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

$assetRoot = (Resolve-Path -LiteralPath $AssetDirectory).ProviderPath
$manifestFile = (Resolve-Path -LiteralPath $ManifestPath).ProviderPath
$manifest = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json

foreach ($asset in $manifest.assets) {
    $assetPath = Join-Path $assetRoot $asset.name
    if (-not (Test-Path -LiteralPath $assetPath -PathType Leaf)) {
        throw "Missing asset: $($asset.name)"
    }

    $item = Get-Item -LiteralPath $assetPath
    if ([int64] $item.Length -ne [int64] $asset.size_bytes) {
        throw "Size mismatch: $($asset.name)"
    }

    $digest = (Get-FileHash -LiteralPath $assetPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($digest -ne ([string] $asset.sha256).ToLowerInvariant()) {
        throw "SHA-256 mismatch: $($asset.name)"
    }
}

foreach ($payload in $manifest.split_payloads) {
    $hasher = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256)
    [int64] $total = 0
    try {
        foreach ($partName in $payload.reassembly_order) {
            $partPath = Join-Path $assetRoot $partName
            $stream = [IO.File]::OpenRead($partPath)
            try {
                $buffer = [byte[]]::new(8MB)
                while (($read = $stream.Read($buffer, 0, $buffer.Length)) -gt 0) {
                    $hasher.AppendData($buffer, 0, $read)
                    $total += $read
                }
            }
            finally {
                $stream.Dispose()
            }
        }
        $wholeDigest = [Convert]::ToHexString($hasher.GetHashAndReset()).ToLowerInvariant()
    }
    finally {
        $hasher.Dispose()
    }

    if ($total -ne [int64] $payload.original_size_bytes) {
        throw "Reassembled size mismatch: $($payload.logical_name)"
    }
    if ($wholeDigest -ne ([string] $payload.original_sha256).ToLowerInvariant()) {
        throw "Reassembled SHA-256 mismatch: $($payload.logical_name)"
    }
}

Write-Output "Verified $($manifest.assets.Count) assets and $($manifest.split_payloads.Count) split payloads."
