#Requires -Version 7.2

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $AssetDirectory,

    [string] $OutputName = 'release-manifest.json'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $AssetDirectory).ProviderPath
$outputPath = Join-Path $root $OutputName
$excluded = @($OutputName, 'SHA256SUMS.txt')

$assets = @(
    Get-ChildItem -LiteralPath $root -File |
        Where-Object { $_.Name -notin $excluded } |
        Sort-Object Name |
        ForEach-Object {
            [ordered]@{
                name       = $_.Name
                size_bytes = [int64] $_.Length
                sha256     = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
)

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
    [ordered]@{ logical_name = 'l24-source-v2.tar'; prefix = 'l24-source-v2.tar.part' },
    [ordered]@{ logical_name = 'halofpx-project-p63-formal-evidence.tar.gz'; prefix = 'halofpx-project-p63-formal-evidence.tar.gz.part' }
)
foreach ($known in $knownPrefixes) {
    $partNames = @($assets.name | Where-Object { $_.StartsWith($known.prefix, [StringComparison]::Ordinal) } | Sort-Object)
    if ($partNames.Count -eq 0) { continue }
    $original = Get-OriginalHashFromParts -PartNames $partNames
    $splitPayloads += [ordered]@{
        logical_name        = $known.logical_name
        original_size_bytes = $original.size_bytes
        original_sha256     = $original.sha256
        reassembly_order     = $partNames
    }
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
[IO.File]::WriteAllText($outputPath, (($manifest | ConvertTo-Json -Depth 8) + "`n"), $utf8)

$sumLines = @(
    foreach ($asset in $assets) {
        '{0}  {1}' -f $asset.sha256, $asset.name
    }
    '{0}  {1}' -f (Get-FileHash -LiteralPath $outputPath -Algorithm SHA256).Hash.ToLowerInvariant(), $OutputName
)
[IO.File]::WriteAllText((Join-Path $root 'SHA256SUMS.txt'), (($sumLines -join "`n") + "`n"), $utf8)
Write-Output "Created $OutputName and SHA256SUMS.txt for $($assets.Count) payload assets."
