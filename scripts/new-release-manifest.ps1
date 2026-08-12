#Requires -Version 7.2

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $AssetDirectory,

    [string] $OutputName = 'release-manifest.json'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$rootItem = Get-Item -LiteralPath (Resolve-Path -LiteralPath $AssetDirectory).ProviderPath -Force
if (-not $rootItem.PSIsContainer) {
    throw 'AssetDirectory must be a directory.'
}
if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'AssetDirectory must not be a reparse point.'
}
$root = $rootItem.FullName
if ($OutputName -cne 'release-manifest.json') {
    throw 'OutputName is fixed to release-manifest.json so a payload cannot be overwritten.'
}
$outputPath = Join-Path $root $OutputName
$checksumOutputPath = Join-Path $root 'SHA256SUMS.txt'
if ((Test-Path -LiteralPath $outputPath) -or (Test-Path -LiteralPath $checksumOutputPath)) {
    throw 'Release control files already exist. This frozen-release generator never replaces a prior control pair; use a new exact asset directory.'
}
$reservedControlNames = [System.Collections.Generic.HashSet[string]]::new(
    [StringComparer]::OrdinalIgnoreCase)
[void] $reservedControlNames.Add($OutputName)
[void] $reservedControlNames.Add('SHA256SUMS.txt')

$provenancePath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\docs\publication\asset-provenance.json'))
$provenance = Get-Content -LiteralPath $provenancePath -Raw | ConvertFrom-Json
if ([int] $provenance.schema_version -ne 1 -or
    [string] $provenance.repository -cne 'JCFrags/HaloFPX' -or
    [string] $provenance.release_tag -cne 'evidence-2026-08-12') {
    throw 'The tracked provenance inventory is not the expected frozen release identity.'
}
$expectedAssetNames = @($provenance.asset_index | ForEach-Object { [string] $_.name } | Sort-Object)
if ($expectedAssetNames.Count -ne 39 -or
    @($expectedAssetNames | Group-Object -CaseSensitive | Where-Object Count -ne 1).Count -ne 0) {
    throw 'The tracked provenance inventory must contain exactly 39 unique, case-sensitive asset names.'
}

$topLevelItems = @(Get-ChildItem -LiteralPath $root -Force)
if (@($topLevelItems | Where-Object { $reservedControlNames.Contains($_.Name) }).Count -gt 0) {
    throw 'A release control filename (including a case variant) already exists.'
}
if (@($topLevelItems | Where-Object PSIsContainer).Count -gt 0) {
    throw 'The frozen release directory must contain payload files only; subdirectories are not allowed.'
}

$assets = @(
    $topLevelItems |
        Where-Object { -not $_.PSIsContainer } |
        Sort-Object Name |
        ForEach-Object {
            if (($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Payload must not be a reparse point: $($_.Name)"
            }
            [ordered]@{
                name       = $_.Name
                size_bytes = [int64] $_.Length
                sha256     = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
)
$observedAssetNames = @($assets | ForEach-Object { [string] $_.name } | Sort-Object)
if ($observedAssetNames.Count -ne $expectedAssetNames.Count -or
    (Compare-Object -ReferenceObject $expectedAssetNames -DifferenceObject $observedAssetNames -CaseSensitive)) {
    throw 'Payload membership does not exactly match the frozen 39-name provenance inventory.'
}

function Get-OriginalHashFromParts {
    param([string[]] $PartNames)
    $hasher = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256)
    [int64] $total = 0
    try {
        foreach ($partName in $PartNames) {
            $stream = [IO.File]::OpenRead((Join-Path $root $partName))
            try {
                $buffer = [byte[]]::new(8MB)
                while (($read = $stream.Read($buffer, 0, $buffer.Length)) -gt 0) {
                    $hasher.AppendData($buffer, 0, $read)
                    $total += $read
                }
            }
            finally { $stream.Dispose() }
        }
        $digest = [Convert]::ToHexString($hasher.GetHashAndReset()).ToLowerInvariant()
    }
    finally { $hasher.Dispose() }
    return [ordered]@{ size_bytes = $total; sha256 = $digest }
}

$splitPayloads = @()
$knownPrefixes = @(
    [ordered]@{
        logical_name = 'l24-source-v2.tar'
        prefix = 'l24-source-v2.tar.part'
        part_count = 10
        original_size_bytes = [int64] 17101714432
        original_sha256 = '5920dbdb2f1d29eac0be84c82611a9869318fae2ec5b3fe1392fd2ef9abef3cf'
    },
    [ordered]@{
        logical_name = 'halofpx-project-p63-formal-evidence.tar.gz'
        prefix = 'halofpx-project-p63-formal-evidence.tar.gz.part'
        part_count = 2
        original_size_bytes = [int64] 2516292772
        original_sha256 = '412dc86ea616b91e77b8618ffae3e4cadf9597c30a32fb91b5a2d3df41a98892'
    }
)
$recognizedPartNames = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($known in $knownPrefixes) {
    $partNames = @(
        for ($index = 1; $index -le $known.part_count; $index++) {
            '{0}{1:d4}' -f $known.prefix, $index
        }
    )
    $observedForPrefix = @(
        $assets.name |
            Where-Object { $_.StartsWith($known.prefix, [StringComparison]::Ordinal) } |
            Sort-Object
    )
    if ($observedForPrefix.Count -ne $partNames.Count -or
        (Compare-Object -ReferenceObject $partNames -DifferenceObject $observedForPrefix -CaseSensitive)) {
        throw "Split payload $($known.logical_name) is missing, duplicated, malformed, or noncontiguous."
    }
    foreach ($partName in $partNames) { [void] $recognizedPartNames.Add($partName) }
    $original = Get-OriginalHashFromParts -PartNames $partNames
    if ([int64] $original.size_bytes -ne [int64] $known.original_size_bytes -or
        [string] $original.sha256 -cne [string] $known.original_sha256) {
        throw "Split payload $($known.logical_name) does not reconstruct the pinned original identity."
    }
    $splitPayloads += [ordered]@{
        logical_name        = $known.logical_name
        original_size_bytes = $original.size_bytes
        original_sha256     = $original.sha256
        reassembly_order     = $partNames
    }
}
$unrecognizedParts = @(
    $assets.name |
        Where-Object { $_ -match '\.part\d+$' -and -not $recognizedPartNames.Contains($_) }
)
if ($unrecognizedParts.Count -gt 0) {
    throw "Unrecognized split-looking asset(s): $($unrecognizedParts -join ', ')"
}

$manifest = [ordered]@{
    schema_version = '1.0'
    repository     = 'JCFrags/HaloFPX'
    visibility     = 'private'
    release_tag    = 'evidence-2026-08-12'
    generated_utc  = [DateTimeOffset]::UtcNow.ToString('o')
    assets         = $assets
    split_payloads = $splitPayloads
}

$utf8 = [Text.UTF8Encoding]::new($false)
$temporarySuffix = '.tmp-' + [guid]::NewGuid().ToString('N')
$temporaryManifestPath = $outputPath + $temporarySuffix
$temporaryChecksumPath = $checksumOutputPath + $temporarySuffix
try {
    [IO.File]::WriteAllText(
        $temporaryManifestPath,
        (($manifest | ConvertTo-Json -Depth 8) + "`n"),
        $utf8)

    $sumLines = @(
        foreach ($asset in $assets) {
            '{0}  {1}' -f $asset.sha256, $asset.name
        }
        '{0}  {1}' -f (Get-FileHash -LiteralPath $temporaryManifestPath -Algorithm SHA256).Hash.ToLowerInvariant(), $OutputName
    )
    [IO.File]::WriteAllText(
        $temporaryChecksumPath,
        (($sumLines -join "`n") + "`n"),
        $utf8)

    # The manifest is the completion marker. The script refuses replacement,
    # promotes the checksum first, and removes any promoted output on an
    # ordinary failure so it cannot destroy or silently mismatch a valid pair.
    $checksumPromoted = $false
    $manifestPromoted = $false
    try {
        [IO.File]::Move($temporaryChecksumPath, $checksumOutputPath, $false)
        $checksumPromoted = $true
        [IO.File]::Move($temporaryManifestPath, $outputPath, $false)
        $manifestPromoted = $true
    }
    catch {
        if ($manifestPromoted -and (Test-Path -LiteralPath $outputPath)) {
            Remove-Item -LiteralPath $outputPath -Force
        }
        if ($checksumPromoted -and (Test-Path -LiteralPath $checksumOutputPath)) {
            Remove-Item -LiteralPath $checksumOutputPath -Force
        }
        throw
    }
}
finally {
    foreach ($temporaryPath in @($temporaryManifestPath, $temporaryChecksumPath)) {
        if (Test-Path -LiteralPath $temporaryPath) {
            Remove-Item -LiteralPath $temporaryPath -Force
        }
    }
}
Write-Output "Created $OutputName and SHA256SUMS.txt for $($assets.Count) payload assets."
