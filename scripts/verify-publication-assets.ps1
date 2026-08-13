#Requires -Version 7.2

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $AssetDirectory,

    [Parameter(Mandatory)]
    [string] $ManifestPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-LowerSha256 {
    param([Parameter(Mandatory)][string] $LiteralPath)
    return (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Assert-LeafName {
    param([Parameter(Mandatory)][string] $Name)
    if ([string]::IsNullOrWhiteSpace($Name) -or
        $Name -in @('.', '..') -or
        [IO.Path]::GetFileName($Name) -cne $Name -or
        $Name.Contains('/') -or
        $Name.Contains('\')) {
        throw "Manifest contains an unsafe non-leaf asset name: $Name"
    }
}

function Resolve-ContainedLeaf {
    param(
        [Parameter(Mandatory)][string] $Root,
        [Parameter(Mandatory)][string] $Name
    )
    Assert-LeafName -Name $Name
    $candidate = [IO.Path]::GetFullPath((Join-Path $Root $Name))
    $rootPrefix = $Root.TrimEnd([IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $candidate.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Asset resolves outside the asset directory: $Name"
    }
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "Missing asset: $Name"
    }
    $item = Get-Item -LiteralPath $candidate -Force
    if ($item.Name -cne $Name) {
        throw "Asset filename case does not exactly match the manifest: expected $Name, found $($item.Name)"
    }
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Asset must not be a reparse point: $Name"
    }
    return $candidate
}

$assetRootItem = Get-Item -LiteralPath (Resolve-Path -LiteralPath $AssetDirectory).ProviderPath -Force
if (-not $assetRootItem.PSIsContainer) {
    throw 'AssetDirectory must be a directory.'
}
if (($assetRootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'AssetDirectory must not be a reparse point.'
}
$assetRoot = $assetRootItem.FullName
$manifestFile = (Resolve-Path -LiteralPath $ManifestPath).ProviderPath

$expectedTrackedManifestDigest = '0b888309cb8318a651389e7b0020dadfb325959413f441adbdb9fd4c2de1c488'
$expectedReleasedManifestDigest = '317390d10c9194bb44cf769a596d0f5257772f6328d7118d0d1167c7461f0950'
$expectedChecksumDigest = 'cbeb29fb2e6cf6b45043bd17db2e925c1e0b4dcd5cff0c3d6f3250745708d827'
if ((Get-LowerSha256 -LiteralPath $manifestFile) -ne $expectedTrackedManifestDigest) {
    throw 'The supplied manifest does not match the trusted tracked manifest digest.'
}

$releaseManifestPath = Resolve-ContainedLeaf -Root $assetRoot -Name 'release-manifest.json'
$checksumPath = Resolve-ContainedLeaf -Root $assetRoot -Name 'SHA256SUMS.txt'
if ((Get-LowerSha256 -LiteralPath $releaseManifestPath) -ne $expectedReleasedManifestDigest) {
    throw 'The downloaded release-manifest.json does not match the trusted immutable release manifest.'
}
if ((Get-LowerSha256 -LiteralPath $checksumPath) -ne $expectedChecksumDigest) {
    throw 'The downloaded SHA256SUMS.txt does not match the trusted checksum-file digest.'
}

$manifest = Get-Content -LiteralPath $releaseManifestPath -Raw | ConvertFrom-Json
if ([string] $manifest.schema_version -cne '1.0' -or
    [string] $manifest.repository -cne 'JCFrags/HaloFPX' -or
    [string] $manifest.visibility -cne 'private' -or
    [string] $manifest.release_tag -cne 'evidence-2026-08-12') {
    throw 'Manifest schema, repository, visibility, or tag identity is not the trusted release identity.'
}

$assetRecords = @($manifest.assets)
if ($assetRecords.Count -ne 39) {
    throw "Expected exactly 39 payload asset records; found $($assetRecords.Count)."
}
$assetByName = [System.Collections.Generic.Dictionary[string, object]]::new(
    [StringComparer]::Ordinal)
foreach ($asset in $assetRecords) {
    $name = [string] $asset.name
    Assert-LeafName -Name $name
    if ($assetByName.ContainsKey($name)) {
        throw "Duplicate asset record: $name"
    }
    if ([int64] $asset.size_bytes -lt 0 -or [string] $asset.sha256 -notmatch '^[0-9a-fA-F]{64}$') {
        throw "Invalid size or SHA-256 record: $name"
    }
    $assetByName[$name] = $asset
}

$expectedDirectoryNames = @($assetByName.Keys) + @('release-manifest.json', 'SHA256SUMS.txt')
$actualItems = @(Get-ChildItem -LiteralPath $assetRoot -Force)
$actualDirectoryNames = @($actualItems | ForEach-Object Name)
$missingNames = @($expectedDirectoryNames | Where-Object { $_ -cnotin $actualDirectoryNames })
$unexpectedNames = @($actualDirectoryNames | Where-Object { $_ -cnotin $expectedDirectoryNames })
if ($missingNames.Count -gt 0 -or $unexpectedNames.Count -gt 0) {
    throw "Asset directory set mismatch. Missing=[$($missingNames -join ', ')] Unexpected=[$($unexpectedNames -join ', ')]"
}

foreach ($asset in $assetRecords) {
    $assetPath = Resolve-ContainedLeaf -Root $assetRoot -Name ([string] $asset.name)
    $item = Get-Item -LiteralPath $assetPath -Force
    if ([int64] $item.Length -ne [int64] $asset.size_bytes) {
        throw "Size mismatch: $($asset.name)"
    }
    if ((Get-LowerSha256 -LiteralPath $assetPath) -ne ([string] $asset.sha256).ToLowerInvariant()) {
        throw "SHA-256 mismatch: $($asset.name)"
    }
}

$knownSplits = @{
    'l24-source-v2.tar' = @{
        count = 10
        prefix = 'l24-source-v2.tar.part'
        size = [int64] 17101714432
        sha256 = '5920dbdb2f1d29eac0be84c82611a9869318fae2ec5b3fe1392fd2ef9abef3cf'
    }
    'halofpx-project-p63-formal-evidence.tar.gz' = @{
        count = 2
        prefix = 'halofpx-project-p63-formal-evidence.tar.gz.part'
        size = [int64] 2516292772
        sha256 = '412dc86ea616b91e77b8618ffae3e4cadf9597c30a32fb91b5a2d3df41a98892'
    }
}
$payloadRecords = @($manifest.split_payloads)
if ($payloadRecords.Count -ne $knownSplits.Count) {
    throw "Expected exactly $($knownSplits.Count) split payload records; found $($payloadRecords.Count)."
}
$usedParts = [System.Collections.Generic.Dictionary[string, string]]::new(
    [StringComparer]::Ordinal)
foreach ($payload in $payloadRecords) {
    $logicalName = [string] $payload.logical_name
    if (-not $knownSplits.ContainsKey($logicalName)) {
        throw "Unexpected split payload: $logicalName"
    }
    $expected = $knownSplits[$logicalName]
    $partNames = @($payload.reassembly_order | ForEach-Object { [string] $_ })
    if ($partNames.Count -ne $expected.count) {
        throw "Wrong part count for $logicalName."
    }
    if ([int64] $payload.original_size_bytes -ne $expected.size -or
        ([string] $payload.original_sha256).ToLowerInvariant() -ne $expected.sha256) {
        throw "Pinned reconstructed identity mismatch for $logicalName."
    }
    for ($index = 1; $index -le $expected.count; $index++) {
        $expectedName = '{0}{1:d4}' -f $expected.prefix, $index
        if ($partNames[$index - 1] -cne $expectedName) {
            throw "Noncontiguous or out-of-order split part for ${logicalName}: expected $expectedName."
        }
        if (-not $assetByName.ContainsKey($expectedName) -or $usedParts.ContainsKey($expectedName)) {
            throw "Missing, unknown, or reused split part: $expectedName"
        }
        $usedParts[$expectedName] = $logicalName
    }

    $hasher = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256)
    [int64] $total = 0
    try {
        foreach ($partName in $partNames) {
            $partPath = Resolve-ContainedLeaf -Root $assetRoot -Name $partName
            $stream = [IO.File]::OpenRead($partPath)
            try {
                $buffer = [byte[]]::new(8MB)
                while (($read = $stream.Read($buffer, 0, $buffer.Length)) -gt 0) {
                    $hasher.AppendData($buffer, 0, $read)
                    $total += $read
                }
            }
            finally { $stream.Dispose() }
        }
        $wholeDigest = [Convert]::ToHexString($hasher.GetHashAndReset()).ToLowerInvariant()
    }
    finally { $hasher.Dispose() }

    if ($total -ne $expected.size -or $wholeDigest -ne $expected.sha256) {
        throw "Reassembled identity mismatch: $logicalName"
    }
}

$allPartAssets = @($assetByName.Keys | Where-Object { $_ -match '\.part\d{4}$' })
if ($usedParts.Count -ne $allPartAssets.Count -or
    @($allPartAssets | Where-Object { -not $usedParts.ContainsKey($_) }).Count -gt 0) {
    throw 'Every split-looking asset must be referenced exactly once by a known split payload.'
}

$checksumRecords = [System.Collections.Generic.Dictionary[string, string]]::new(
    [StringComparer]::Ordinal)
foreach ($line in Get-Content -LiteralPath $checksumPath) {
    if ($line -notmatch '^([0-9a-f]{64})  ([^/\\]+)$') {
        throw "Malformed SHA256SUMS.txt line: $line"
    }
    $digest = $Matches[1]
    $name = $Matches[2]
    Assert-LeafName -Name $name
    if ($checksumRecords.ContainsKey($name)) {
        throw "Duplicate checksum record: $name"
    }
    $checksumRecords[$name] = $digest
}
$expectedChecksumNames = @($assetByName.Keys) + @('release-manifest.json')
if ($checksumRecords.Count -ne $expectedChecksumNames.Count -or
    @($expectedChecksumNames | Where-Object { -not $checksumRecords.ContainsKey($_) }).Count -gt 0) {
    throw 'SHA256SUMS.txt does not cover exactly the 39 payloads and release-manifest.json.'
}
foreach ($asset in $assetRecords) {
    if ($checksumRecords[[string] $asset.name] -ne ([string] $asset.sha256).ToLowerInvariant()) {
        throw "Checksum-list disagreement: $($asset.name)"
    }
}
if ($checksumRecords['release-manifest.json'] -ne $expectedReleasedManifestDigest) {
    throw 'Checksum-list disagreement: release-manifest.json'
}

Write-Output 'Verified 39 assets, 2 split payloads, exact directory membership, and both trusted control files.'
