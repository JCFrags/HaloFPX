#Requires -Version 7.2

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$verifierPath = Join-Path $repoRoot 'scripts/verify-publication-assets.ps1'
$trackedManifestPath = Join-Path $repoRoot 'docs/publication/release-manifest.json'
$checksumPath = Join-Path $repoRoot 'docs/publication/SHA256SUMS.txt'
$pwshPath = (Get-Command pwsh -ErrorAction Stop).Source

$expectedTrackedDigest = '0b888309cb8318a651389e7b0020dadfb325959413f441adbdb9fd4c2de1c488'
$expectedReleasedDigest = '317390d10c9194bb44cf769a596d0f5257772f6328d7118d0d1167c7461f0950'

function Get-LowerSha256 {
    param([Parameter(Mandatory)][string] $LiteralPath)
    return (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Invoke-VerifierFailure {
    param(
        [Parameter(Mandatory)][string] $AssetDirectory,
        [Parameter(Mandatory)][string] $ManifestPath,
        [Parameter(Mandatory)][string] $ExpectedMessage
    )

    $output = & $pwshPath -NoProfile -NonInteractive -File $verifierPath `
        -AssetDirectory $AssetDirectory -ManifestPath $ManifestPath 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    if ($exitCode -eq 0) {
        throw "Verifier unexpectedly succeeded; expected failure containing: $ExpectedMessage"
    }
    if ($output -notlike "*$ExpectedMessage*") {
        throw "Verifier failed for the wrong reason. Expected=[$ExpectedMessage] Output=[$output]"
    }
}

$scratchRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ('halofpx-fresh-clone-manifest-identity-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $scratchRoot | Out-Null

try {
    if ((Get-LowerSha256 -LiteralPath $trackedManifestPath) -ne $expectedTrackedDigest) {
        throw 'The checked-out tracked manifest is not the pinned LF identity used by this regression.'
    }

    $trackedBytes = [IO.File]::ReadAllBytes($trackedManifestPath)
    if ($trackedBytes.Length -eq 0 -or $trackedBytes[$trackedBytes.Length - 1] -ne 0x0a) {
        throw 'The tracked manifest must end with LF for the released-byte fixture reconstruction.'
    }

    $releasedBytes = [Collections.Generic.List[byte]]::new($trackedBytes.Length + 256)
    for ($index = 0; $index -lt $trackedBytes.Length; $index++) {
        if ($trackedBytes[$index] -eq 0x0a -and $index -lt $trackedBytes.Length - 1) {
            $releasedBytes.Add(0x0d)
        }
        $releasedBytes.Add($trackedBytes[$index])
    }

    $cleanAssetDirectory = Join-Path $scratchRoot 'clean-release-controls'
    New-Item -ItemType Directory -Path $cleanAssetDirectory | Out-Null
    $releasedManifestPath = Join-Path $cleanAssetDirectory 'release-manifest.json'
    [IO.File]::WriteAllBytes($releasedManifestPath, $releasedBytes.ToArray())
    Copy-Item -LiteralPath $checksumPath -Destination (Join-Path $cleanAssetDirectory 'SHA256SUMS.txt')
    if ((Get-LowerSha256 -LiteralPath $releasedManifestPath) -ne $expectedReleasedDigest) {
        throw 'The reconstructed immutable release-manifest.json bytes do not match the pinned release identity.'
    }

    Invoke-VerifierFailure -AssetDirectory $cleanAssetDirectory `
        -ManifestPath $trackedManifestPath `
        -ExpectedMessage 'Asset directory set mismatch.'

    $tamperedTrackedManifestPath = Join-Path $scratchRoot 'tampered-tracked-manifest.json'
    $tamperedBytes = [byte[]]::new($trackedBytes.Length + 1)
    [Array]::Copy($trackedBytes, $tamperedBytes, $trackedBytes.Length)
    $tamperedBytes[$tamperedBytes.Length - 1] = 0x20
    [IO.File]::WriteAllBytes($tamperedTrackedManifestPath, $tamperedBytes)
    Invoke-VerifierFailure -AssetDirectory $cleanAssetDirectory `
        -ManifestPath $tamperedTrackedManifestPath `
        -ExpectedMessage 'The supplied manifest does not match the trusted tracked manifest digest.'

    $normalizedAssetDirectory = Join-Path $scratchRoot 'lf-normalized-release-controls'
    New-Item -ItemType Directory -Path $normalizedAssetDirectory | Out-Null
    Copy-Item -LiteralPath $trackedManifestPath `
        -Destination (Join-Path $normalizedAssetDirectory 'release-manifest.json')
    Copy-Item -LiteralPath $checksumPath `
        -Destination (Join-Path $normalizedAssetDirectory 'SHA256SUMS.txt')
    Invoke-VerifierFailure -AssetDirectory $normalizedAssetDirectory `
        -ManifestPath $trackedManifestPath `
        -ExpectedMessage 'The downloaded release-manifest.json does not match the trusted immutable release manifest.'

    Write-Output 'PASS: fresh-clone tracked/released manifest identities remain distinct and release checks reach membership.'
}
finally {
    if (Test-Path -LiteralPath $scratchRoot) {
        Remove-Item -LiteralPath $scratchRoot -Recurse -Force
    }
}
