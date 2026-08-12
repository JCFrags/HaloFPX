#Requires -Version 7.2

# Historical general-purpose packager. It did not create the
# evidence-2026-08-12 release. Do not use it for a new authoritative package
# until its source capture is bound to an immutable filesystem snapshot (or to
# verified before/after content and ref-set digests); HEAD and branch identity
# alone cannot make a live dirty working tree atomic.

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string] $OriginalDocsRepo,

    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string] $OriginalImplementationRepo,

    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string] $OutputDirectory,

    [switch] $AllowNonAuthoritativeLiveCapture
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

# GitHub release assets must remain below 2 GiB. Decimal 1.9 GB leaves margin
# for client and service interpretations of the limit.
$SplitThresholdBytes = [uint64] 1900000000
$CopyBufferBytes = 8 * 1024 * 1024
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$PathComparison = [System.StringComparison]::OrdinalIgnoreCase

if (-not $AllowNonAuthoritativeLiveCapture) {
    throw 'This historical live-tree packager is non-authoritative and non-atomic. Pass -AllowNonAuthoritativeLiveCapture only for an explicitly labelled diagnostic capture; do not use its output as release evidence.'
}

$ExcludedDirectoryNames = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase
)
@(
    '.git',
    '.cache',
    '.gradle',
    '.mypy_cache',
    '.nox',
    '.pytest_cache',
    '.ruff_cache',
    '.tox',
    '.venv',
    '.vs',
    'CMakeFiles',
    '__pycache__',
    '_deps',
    'build',
    'node_modules',
    'venv'
) | ForEach-Object { [void] $ExcludedDirectoryNames.Add($_) }

# These are immutable milestone-transition payloads, not disposable build trees.
# They are deliberately retained even though some names contain "build".
$ExplicitTransitionArchiveNames = @(
    'l97-build.tar',
    'l97-source.tar',
    'l98-build-corrected.tar',
    'l98-build-final.tar',
    'l98-build.tar',
    'l98-source-corrected.tar',
    'l98-source-final.tar',
    'l98-source.tar'
)

function Get-NormalizedPath {
    param(
        [Parameter(Mandatory)]
        [string] $Path
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $rootPath = [System.IO.Path]::GetPathRoot($fullPath)
    if ($fullPath.Equals($rootPath, $PathComparison)) {
        return $rootPath
    }

    return $fullPath.TrimEnd([char[]] @(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ))
}

function Resolve-ExistingDirectoryLiteral {
    param(
        [Parameter(Mandatory)]
        [string] $Path
    )

    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    if ($resolved.Provider.Name -ne 'FileSystem') {
        throw 'A required path is not a filesystem path.'
    }

    $item = Get-Item -LiteralPath $resolved.ProviderPath -Force -ErrorAction Stop
    if (-not $item.PSIsContainer) {
        throw 'A required repository or output path is not a directory.'
    }

    return Get-NormalizedPath -Path $item.FullName
}

function Test-IsFilesystemRoot {
    param(
        [Parameter(Mandatory)]
        [string] $Path
    )

    $normalized = Get-NormalizedPath -Path $Path
    $root = Get-NormalizedPath -Path ([System.IO.Path]::GetPathRoot($normalized))
    return $normalized.Equals($root, $PathComparison)
}

function Test-IsWithinPath {
    param(
        [Parameter(Mandatory)]
        [string] $Candidate,

        [Parameter(Mandatory)]
        [string] $Root
    )

    $candidatePath = Get-NormalizedPath -Path $Candidate
    $rootPath = Get-NormalizedPath -Path $Root
    if ($candidatePath.Equals($rootPath, $PathComparison)) {
        return $true
    }

    $rootPrefix = $rootPath
    if (-not $rootPrefix.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $rootPrefix += [System.IO.Path]::DirectorySeparatorChar
    }

    return $candidatePath.StartsWith($rootPrefix, $PathComparison)
}

function Resolve-SafeOutputDirectory {
    param(
        [Parameter(Mandatory)]
        [string] $RequestedPath,

        [Parameter(Mandatory)]
        [string[]] $SourceRoots
    )

    if (Test-Path -LiteralPath $RequestedPath) {
        $outputPath = Resolve-ExistingDirectoryLiteral -Path $RequestedPath
        $outputItem = Get-Item -LiteralPath $outputPath -Force -ErrorAction Stop
        if (($outputItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'The output directory must not be a reparse point.'
        }
    }
    else {
        $candidate = Get-NormalizedPath -Path $RequestedPath
        $parentCandidate = [System.IO.Path]::GetDirectoryName($candidate)
        $leafName = [System.IO.Path]::GetFileName($candidate)
        if ([string]::IsNullOrWhiteSpace($parentCandidate) -or
            [string]::IsNullOrWhiteSpace($leafName) -or
            $leafName -in @('.', '..')) {
            throw 'The requested output directory is not a safe leaf path.'
        }

        $resolvedParent = Resolve-ExistingDirectoryLiteral -Path $parentCandidate
        $outputPath = Get-NormalizedPath -Path (Join-Path -Path $resolvedParent -ChildPath $leafName)
    }

    if (Test-IsFilesystemRoot -Path $outputPath) {
        throw 'The output directory must not be a filesystem root.'
    }

    $userProfile = [Environment]::GetFolderPath([Environment+SpecialFolder]::UserProfile)
    if (-not [string]::IsNullOrWhiteSpace($userProfile)) {
        $resolvedUserProfile = Get-NormalizedPath -Path $userProfile
        if ($outputPath.Equals($resolvedUserProfile, $PathComparison)) {
            throw 'The output directory must not be the user profile.'
        }
    }

    foreach ($sourceRoot in $SourceRoots) {
        if ((Test-IsWithinPath -Candidate $outputPath -Root $sourceRoot) -or
            (Test-IsWithinPath -Candidate $sourceRoot -Root $outputPath)) {
            throw 'The output directory must be separate from every source repository.'
        }
    }

    if (Test-Path -LiteralPath $outputPath) {
        if (@(Get-ChildItem -LiteralPath $outputPath -Force -ErrorAction Stop).Count -ne 0) {
            throw 'The output directory must be empty; existing assets are never overwritten.'
        }
    }
    else {
        [void] [System.IO.Directory]::CreateDirectory($outputPath)
    }

    return Resolve-ExistingDirectoryLiteral -Path $outputPath
}

function Assert-RepositoryRoot {
    param(
        [Parameter(Mandatory)]
        [string] $RepositoryRoot
    )

    if (Test-IsFilesystemRoot -Path $RepositoryRoot) {
        throw 'A source repository must not be a filesystem root.'
    }

    if (-not (Test-Path -LiteralPath (Join-Path $RepositoryRoot '.git'))) {
        throw 'A source directory is not a Git repository.'
    }
}

function Test-ExcludedDirectory {
    param(
        [Parameter(Mandatory)]
        [string] $Name,

        [Parameter(Mandatory)]
        [string] $RelativePath
    )

    if ($ExcludedDirectoryNames.Contains($Name)) {
        return $true
    }

    if ($Name -like 'build-*' -or $Name -like 'cmake-build-*') {
        return $true
    }

    # Preserve formal/out and other evidence directories; prune only the
    # conventional CTest scratch subtree.
    if (($RelativePath -replace '\\', '/') -match '(^|/)Testing/Temporary$') {
        return $true
    }

    # Reject malformed copies of machine-temp canary directories without
    # depending on a specific username or drive letter.
    if ($Name -like 'C*AppData*Local*Temp*halofpx-*') {
        return $true
    }

    return $false
}

function Get-ArchiveEntryList {
    param(
        [Parameter(Mandatory)]
        [string] $SourceRoot
    )

    $entries = [System.Collections.Generic.List[string]]::new()
    $directories = [System.Collections.Generic.Stack[System.IO.DirectoryInfo]]::new()
    $rootItem = Get-Item -LiteralPath $SourceRoot -Force -ErrorAction Stop
    $directories.Push($rootItem)

    while ($directories.Count -gt 0) {
        $current = $directories.Pop()
        foreach ($item in Get-ChildItem -LiteralPath $current.FullName -Force -ErrorAction Stop) {
            $relativePath = [System.IO.Path]::GetRelativePath($SourceRoot, $item.FullName)
            if ([System.IO.Path]::IsPathRooted($relativePath) -or
                $relativePath -eq '..' -or
                $relativePath.StartsWith(('..' + [System.IO.Path]::DirectorySeparatorChar), $PathComparison)) {
                throw 'An archive entry escaped its approved repository root.'
            }

            $archivePath = $relativePath -replace '\\', '/'
            if ($item.Name.Equals('.git', $PathComparison)) {
                continue
            }

            if ($item.PSIsContainer) {
                if (Test-ExcludedDirectory -Name $item.Name -RelativePath $relativePath) {
                    continue
                }

                [void] $entries.Add($archivePath)
                if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0) {
                    $directories.Push($item)
                }
            }
            else {
                [void] $entries.Add($archivePath)
            }
        }
    }

    $orderedEntries = $entries.ToArray()
    [System.Array]::Sort($orderedEntries, [System.StringComparer]::Ordinal)
    if ($orderedEntries.Count -eq 0) {
        throw 'A source repository produced no preservable archive entries.'
    }

    return ,$orderedEntries
}

function Write-NullTerminatedFileList {
    param(
        [Parameter(Mandatory)]
        [string[]] $Entries,

        [Parameter(Mandatory)]
        [string] $DestinationPath
    )

    $stream = [System.IO.File]::Open(
        $DestinationPath,
        [System.IO.FileMode]::CreateNew,
        [System.IO.FileAccess]::Write,
        [System.IO.FileShare]::None
    )
    try {
        foreach ($entry in $Entries) {
            $bytes = $Utf8NoBom.GetBytes($entry)
            $stream.Write($bytes, 0, $bytes.Length)
            $stream.WriteByte(0)
        }
        $stream.Flush($true)
    }
    finally {
        $stream.Dispose()
    }
}

function New-WorkingTreeArchive {
    param(
        [Parameter(Mandatory)]
        [string] $SourceRoot,

        [Parameter(Mandatory)]
        [string] $DestinationPath,

        [Parameter(Mandatory)]
        [string] $ListPath,

        [Parameter(Mandatory)]
        [string] $TarCommand
    )

    if (Test-Path -LiteralPath $DestinationPath) {
        throw 'A generated archive would overwrite an existing file.'
    }

    $entries = Get-ArchiveEntryList -SourceRoot $SourceRoot
    Write-NullTerminatedFileList -Entries $entries -DestinationPath $ListPath

    $arguments = @(
        '-cf', $DestinationPath,
        '--format', 'pax',
        '--no-recursion',
        '-C', $SourceRoot,
        '--null',
        '-T', $ListPath
    )
    & $TarCommand @arguments 1> $null 2> $null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $DestinationPath -PathType Leaf)) {
        throw 'Creation of a working-tree preservation archive failed.'
    }

    return $entries.Count
}

function Get-GitIdentity {
    param(
        [Parameter(Mandatory)]
        [string] $RepositoryRoot,

        [Parameter(Mandatory)]
        [string] $GitCommand
    )

    $headOutput = & $GitCommand -C $RepositoryRoot rev-parse --verify HEAD 2> $null
    if ($LASTEXITCODE -ne 0) {
        throw 'A source repository has no resolvable HEAD commit.'
    }
    $head = ([string] $headOutput).Trim()
    if ($head -notmatch '^[0-9a-fA-F]{40,64}$') {
        throw 'A source repository returned an invalid HEAD object identifier.'
    }

    $branchOutput = & $GitCommand -C $RepositoryRoot branch --show-current 2> $null
    if ($LASTEXITCODE -ne 0) {
        throw 'A source repository branch could not be inspected.'
    }
    $branch = ([string] $branchOutput).Trim()
    if ([string]::IsNullOrWhiteSpace($branch)) {
        $branch = '(detached)'
    }

    return [pscustomobject] [ordered] @{
        head   = $head.ToLowerInvariant()
        branch = $branch
    }
}

function New-GitHistoryBundle {
    param(
        [Parameter(Mandatory)]
        [string] $RepositoryRoot,

        [Parameter(Mandatory)]
        [string] $DestinationPath,

        [Parameter(Mandatory)]
        [string] $GitCommand
    )

    if (Test-Path -LiteralPath $DestinationPath) {
        throw 'A generated Git bundle would overwrite an existing file.'
    }

    # Branches, tags, and the active HEAD are canonical. Internal refs such as
    # refs/codex/checkpoints are intentionally not promoted into publication.
    & $GitCommand -C $RepositoryRoot bundle create $DestinationPath HEAD --branches --tags 1> $null 2> $null
    if ($LASTEXITCODE -ne 0) {
        throw 'Creation of a Git history bundle failed.'
    }

    & $GitCommand -C $RepositoryRoot bundle verify $DestinationPath 1> $null 2> $null
    if ($LASTEXITCODE -ne 0) {
        throw 'Verification of a Git history bundle failed.'
    }
}

function Convert-HashToLowerHex {
    param(
        [Parameter(Mandatory)]
        [byte[]] $Bytes
    )

    return [System.Convert]::ToHexString($Bytes).ToLowerInvariant()
}

function Complete-PublicationAsset {
    param(
        [Parameter(Mandatory)]
        [string] $AssetPath,

        [Parameter(Mandatory)]
        [string] $ApprovedOutputRoot,

        [Parameter(Mandatory)]
        [string] $LogicalName,

        [Parameter(Mandatory)]
        [string] $SourceLabel,

        [Parameter(Mandatory)]
        [string] $MediaType,

        [Parameter(Mandatory)]
        [uint64] $EntryCount
    )

    $assetItem = Get-Item -LiteralPath $AssetPath -Force -ErrorAction Stop
    $assetParent = Get-NormalizedPath -Path $assetItem.DirectoryName
    $approvedRoot = Get-NormalizedPath -Path $ApprovedOutputRoot
    if (-not $assetParent.Equals($approvedRoot, $PathComparison)) {
        throw 'A generated asset is outside the approved output directory.'
    }

    $originalSize = [uint64] $assetItem.Length
    $partRecords = [System.Collections.Generic.List[object]]::new()

    if ($originalSize -le $SplitThresholdBytes) {
        $sha256 = (Get-FileHash -LiteralPath $AssetPath -Algorithm SHA256).Hash.ToLowerInvariant()
        [void] $partRecords.Add([pscustomobject] [ordered] @{
            index        = 1
            file         = $assetItem.Name
            size_bytes   = $originalSize
            sha256       = $sha256
        })
        $originalHash = $sha256
    }
    else {
        $sourceStream = [System.IO.File]::Open(
            $AssetPath,
            [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::Read,
            [System.IO.FileShare]::Read
        )
        $overallHasher = [System.Security.Cryptography.IncrementalHash]::CreateHash(
            [System.Security.Cryptography.HashAlgorithmName]::SHA256
        )
        $buffer = [byte[]]::new($CopyBufferBytes)
        $partIndex = 0

        try {
            while ($sourceStream.Position -lt $sourceStream.Length) {
                $partIndex++
                $partName = '{0}.part{1:D4}' -f $assetItem.Name, $partIndex
                $partPath = Join-Path -Path $assetItem.DirectoryName -ChildPath $partName
                $partialPath = $partPath + '.partial'
                if ((Test-Path -LiteralPath $partPath) -or (Test-Path -LiteralPath $partialPath)) {
                    throw 'A split part would overwrite an existing file.'
                }

                $partHasher = [System.Security.Cryptography.IncrementalHash]::CreateHash(
                    [System.Security.Cryptography.HashAlgorithmName]::SHA256
                )
                try {
                    $partStream = [System.IO.File]::Open(
                        $partialPath,
                        [System.IO.FileMode]::CreateNew,
                        [System.IO.FileAccess]::Write,
                        [System.IO.FileShare]::None
                    )
                    [uint64] $partSize = 0
                    try {
                        while ($partSize -lt $SplitThresholdBytes -and
                            $sourceStream.Position -lt $sourceStream.Length) {
                            $remainingInPart = [int64] ($SplitThresholdBytes - $partSize)
                            $toRead = [int] [System.Math]::Min([int64] $buffer.Length, $remainingInPart)
                            $read = $sourceStream.Read($buffer, 0, $toRead)
                            if ($read -le 0) {
                                break
                            }

                            $partStream.Write($buffer, 0, $read)
                            $partHasher.AppendData($buffer, 0, $read)
                            $overallHasher.AppendData($buffer, 0, $read)
                            $partSize += [uint64] $read
                        }
                        $partStream.Flush($true)
                    }
                    finally {
                        $partStream.Dispose()
                    }

                    Move-Item -LiteralPath $partialPath -Destination $partPath -ErrorAction Stop
                    $partHash = Convert-HashToLowerHex -Bytes $partHasher.GetHashAndReset()
                }
                finally {
                    $partHasher.Dispose()
                }

                [void] $partRecords.Add([pscustomobject] [ordered] @{
                    index      = $partIndex
                    file       = $partName
                    size_bytes = $partSize
                    sha256     = $partHash
                })
            }

            $originalHash = Convert-HashToLowerHex -Bytes $overallHasher.GetHashAndReset()
        }
        finally {
            $sourceStream.Dispose()
            $overallHasher.Dispose()
        }

        $splitTotal = [uint64] 0
        foreach ($partRecord in $partRecords) {
            $splitTotal += [uint64] $partRecord.size_bytes
        }
        if ($splitTotal -ne $originalSize) {
            throw 'The split-part byte count does not match the source asset.'
        }

        # This removes only the newly generated, verified oversize asset. The
        # canonical source repositories are never modified.
        Remove-Item -LiteralPath $AssetPath -Force -ErrorAction Stop
    }

    return [pscustomobject] [ordered] @{
        logical_name          = $LogicalName
        source_label          = $SourceLabel
        media_type            = $MediaType
        entry_count           = $EntryCount
        original_file         = $assetItem.Name
        original_size_bytes   = $originalSize
        original_sha256       = $originalHash
        split                 = ($originalSize -gt $SplitThresholdBytes)
        part_count            = $partRecords.Count
        reassembly_order      = @($partRecords | ForEach-Object { $_.file })
        parts                 = @($partRecords)
    }
}

function Remove-PrivateWorkDirectory {
    param(
        [Parameter(Mandatory)]
        [string] $WorkDirectory,

        [Parameter(Mandatory)]
        [string] $ApprovedOutputRoot
    )

    if (-not (Test-Path -LiteralPath $WorkDirectory)) {
        return
    }

    $resolvedWork = Resolve-ExistingDirectoryLiteral -Path $WorkDirectory
    $resolvedOutput = Resolve-ExistingDirectoryLiteral -Path $ApprovedOutputRoot
    if (-not (Test-IsWithinPath -Candidate $resolvedWork -Root $resolvedOutput) -or
        $resolvedWork.Equals($resolvedOutput, $PathComparison) -or
        -not ([System.IO.Path]::GetFileName($resolvedWork).StartsWith('.prepare-publication-assets-', $PathComparison))) {
        throw 'The temporary-work cleanup target failed its path guard.'
    }

    Remove-Item -LiteralPath $resolvedWork -Recurse -Force -ErrorAction Stop
}

$privateWorkDirectory = $null
$safeOutputRoot = $null
try {
    $docsRoot = Resolve-ExistingDirectoryLiteral -Path $OriginalDocsRepo
    $implementationRoot = Resolve-ExistingDirectoryLiteral -Path $OriginalImplementationRepo
    if ($docsRoot.Equals($implementationRoot, $PathComparison)) {
        throw 'The documentation and implementation repositories must be distinct.'
    }
    Assert-RepositoryRoot -RepositoryRoot $docsRoot
    Assert-RepositoryRoot -RepositoryRoot $implementationRoot

    $safeOutputRoot = Resolve-SafeOutputDirectory -RequestedPath $OutputDirectory -SourceRoots @(
        $docsRoot,
        $implementationRoot
    )

    $gitCommandInfo = Get-Command git -CommandType Application -ErrorAction Stop
    $tarCommandInfo = Get-Command tar -CommandType Application -ErrorAction Stop
    $gitCommand = $gitCommandInfo.Source
    $tarCommand = $tarCommandInfo.Source

    $privateWorkDirectory = Join-Path -Path $safeOutputRoot -ChildPath (
        '.prepare-publication-assets-' + [guid]::NewGuid().ToString('N')
    )
    [void] [System.IO.Directory]::CreateDirectory($privateWorkDirectory)

    $docsIdentity = Get-GitIdentity -RepositoryRoot $docsRoot -GitCommand $gitCommand
    $implementationIdentity = Get-GitIdentity -RepositoryRoot $implementationRoot -GitCommand $gitCommand
    $assetRecords = [System.Collections.Generic.List[object]]::new()

    $docsArchivePath = Join-Path $safeOutputRoot 'halofpx-project-working-tree.tar'
    $docsListPath = Join-Path $privateWorkDirectory 'project-files.null'
    $docsEntryCount = New-WorkingTreeArchive `
        -SourceRoot $docsRoot `
        -DestinationPath $docsArchivePath `
        -ListPath $docsListPath `
        -TarCommand $tarCommand
    [void] $assetRecords.Add((Complete-PublicationAsset `
        -AssetPath $docsArchivePath `
        -ApprovedOutputRoot $safeOutputRoot `
        -LogicalName 'project-working-tree' `
        -SourceLabel 'halofpx-project-wiki' `
        -MediaType 'application/x-tar' `
        -EntryCount $docsEntryCount))

    $implementationArchivePath = Join-Path $safeOutputRoot 'halofpx-implementation-working-tree.tar'
    $implementationListPath = Join-Path $privateWorkDirectory 'implementation-files.null'
    $implementationEntryCount = New-WorkingTreeArchive `
        -SourceRoot $implementationRoot `
        -DestinationPath $implementationArchivePath `
        -ListPath $implementationListPath `
        -TarCommand $tarCommand
    [void] $assetRecords.Add((Complete-PublicationAsset `
        -AssetPath $implementationArchivePath `
        -ApprovedOutputRoot $safeOutputRoot `
        -LogicalName 'implementation-working-tree' `
        -SourceLabel 'halofpx-implementation' `
        -MediaType 'application/x-tar' `
        -EntryCount $implementationEntryCount))

    $docsBundlePath = Join-Path $safeOutputRoot 'halofpx-project-git-history.bundle'
    New-GitHistoryBundle -RepositoryRoot $docsRoot -DestinationPath $docsBundlePath -GitCommand $gitCommand
    [void] $assetRecords.Add((Complete-PublicationAsset `
        -AssetPath $docsBundlePath `
        -ApprovedOutputRoot $safeOutputRoot `
        -LogicalName 'project-git-history' `
        -SourceLabel 'halofpx-project-wiki' `
        -MediaType 'application/x-git-bundle' `
        -EntryCount 0))

    $implementationBundlePath = Join-Path $safeOutputRoot 'halofpx-implementation-git-history.bundle'
    New-GitHistoryBundle `
        -RepositoryRoot $implementationRoot `
        -DestinationPath $implementationBundlePath `
        -GitCommand $gitCommand
    [void] $assetRecords.Add((Complete-PublicationAsset `
        -AssetPath $implementationBundlePath `
        -ApprovedOutputRoot $safeOutputRoot `
        -LogicalName 'implementation-git-history' `
        -SourceLabel 'halofpx-implementation' `
        -MediaType 'application/x-git-bundle' `
        -EntryCount 0))

    $manifest = [ordered] @{
        schema_version        = '1.0'
        authoritative         = $false
        source_capture_atomic = $false
        warning               = 'Non-authoritative live working-tree capture; files may have changed while packaging.'
        created_utc           = [DateTimeOffset]::UtcNow.ToString('o')
        generator             = 'scripts/prepare-publication-assets.ps1'
        split_threshold_bytes = $SplitThresholdBytes
        sources               = @(
            [ordered] @{
                label      = 'halofpx-project-wiki'
                root_leaf  = [System.IO.Path]::GetFileName($docsRoot)
                git_head   = $docsIdentity.head
                git_branch = $docsIdentity.branch
            },
            [ordered] @{
                label      = 'halofpx-implementation'
                root_leaf  = [System.IO.Path]::GetFileName($implementationRoot)
                git_head   = $implementationIdentity.head
                git_branch = $implementationIdentity.branch
            }
        )
        preservation_policy   = [ordered] @{
            included = @(
                'All regular files and non-cache directories in both working trees, including untracked and ignored evidence.',
                'Canonical branches, tags, and active HEAD commits in verified Git bundles.',
                'Explicit L97/L98 transition archives when present at repository scope.'
            )
            excluded = @(
                '.git working metadata (history is preserved separately in bundles).',
                'Reproducible build, dependency, test-temporary, bytecode, virtual-environment, and package-manager cache directories.',
                'Internal refs outside branches, tags, and active HEAD, including refs/codex/checkpoints.'
            )
            explicit_transition_archive_names = $ExplicitTransitionArchiveNames
            symlink_policy = 'Archive the link entry; do not traverse reparse-point directories.'
        }
        reassembly            = [ordered] @{
            algorithm = 'Concatenate each artifact reassembly_order byte-for-byte; verify original_size_bytes and original_sha256.'
            powershell_example = '[IO.File]::WriteAllBytes is unsuitable for large assets; stream each ordered part into one FileStream.'
        }
        artifacts             = @($assetRecords)
    }

    $manifestPath = Join-Path $safeOutputRoot 'publication-assets.json'
    $manifestJson = $manifest | ConvertTo-Json -Depth 12
    [System.IO.File]::WriteAllText($manifestPath, ($manifestJson + [Environment]::NewLine), $Utf8NoBom)

    $checksumRecords = [System.Collections.Generic.List[object]]::new()
    foreach ($asset in $assetRecords) {
        foreach ($part in $asset.parts) {
            [void] $checksumRecords.Add([pscustomobject] @{
                file   = [string] $part.file
                sha256 = [string] $part.sha256
            })
        }
    }
    [void] $checksumRecords.Add([pscustomobject] @{
        file   = 'publication-assets.json'
        sha256 = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    })

    $orderedChecksumRecords = @($checksumRecords | Sort-Object -Property file)
    $checksumLines = foreach ($record in $orderedChecksumRecords) {
        '{0}  {1}' -f $record.sha256, $record.file
    }
    $checksumPath = Join-Path $safeOutputRoot 'SHA256SUMS'
    [System.IO.File]::WriteAllText(
        $checksumPath,
        (($checksumLines -join "`n") + "`n"),
        $Utf8NoBom
    )

    Write-Warning 'NON-AUTHORITATIVE live-tree capture completed. It is not atomic release evidence.'
    Write-Output 'Diagnostic asset preparation completed. Verify the JSON and SHA-256 manifests before any restricted internal use.'
}
catch {
    throw 'Publication asset preparation failed without printing source content. Inspect locally before retrying into a new empty output directory.'
}
finally {
    if ($null -ne $privateWorkDirectory -and $null -ne $safeOutputRoot) {
        Remove-PrivateWorkDirectory `
            -WorkDirectory $privateWorkDirectory `
            -ApprovedOutputRoot $safeOutputRoot
    }
}
