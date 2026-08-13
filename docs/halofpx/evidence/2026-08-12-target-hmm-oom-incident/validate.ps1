[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = [System.IO.Path]::GetFullPath($PSScriptRoot)
$rootPrefix = $root.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
$strictUtf8 = [System.Text.UTF8Encoding]::new($false, $true)

function Assert-True {
    param([bool] $Condition, [string] $Message)
    if (-not $Condition) { throw $Message }
}

function Read-Evidence {
    param([string] $RelativePath)
    Get-Content -Raw -LiteralPath (Join-Path $root $RelativePath)
}

function Assert-CanonicalRelativePath {
    param(
        [Parameter(Mandatory)] [string] $RelativePath,
        [Parameter(Mandatory)] [string] $Context
    )

    Assert-True (-not [string]::IsNullOrWhiteSpace($RelativePath)) "$Context path is empty."
    Assert-True (-not [System.IO.Path]::IsPathRooted($RelativePath)) "$Context absolute path rejected: $RelativePath"
    Assert-True ($RelativePath -notmatch '\\') "$Context must use portable forward slashes: $RelativePath"
    Assert-True ($RelativePath -notmatch '(^|/)\.\.?(/|$)') "$Context traversal or dot segment rejected: $RelativePath"

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    Assert-True ($fullPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) "$Context escapes bundle root: $RelativePath"
    $normalized = [System.IO.Path]::GetRelativePath($root, $fullPath).Replace('\', '/')
    Assert-True ($normalized -ceq $RelativePath) "$Context path is not canonical: $RelativePath"
}

function New-OrdinalSet {
    return ,([System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal))
}

function New-OrdinalIgnoreCaseSet {
    return ,([System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase))
}

$manifestPath = Join-Path $root "manifest.json"
$checksumPath = Join-Path $root "SHA256SUMS"
$manifestText = Get-Content -Raw -LiteralPath $manifestPath
Add-Type -AssemblyName "Newtonsoft.Json"
$jsonLoadSettings = [Newtonsoft.Json.Linq.JsonLoadSettings]::new()
$jsonLoadSettings.DuplicatePropertyNameHandling = [Newtonsoft.Json.Linq.DuplicatePropertyNameHandling]::Error
try {
    [void] [Newtonsoft.Json.Linq.JToken]::Parse($manifestText, $jsonLoadSettings)
} catch {
    throw "Manifest JSON is invalid or contains duplicate properties: $($_.Exception.Message)"
}
$manifest = $manifestText | ConvertFrom-Json
Assert-True ($manifest.schema -eq "halofpx.target-safety-incident.v1") "Unexpected manifest schema."
Assert-True ($manifest.classification -eq "production-safety-incident") "Incident classification changed."
Assert-True ($manifest.benchmark_valid -eq $false) "Incident must remain an invalid benchmark."
Assert-True ($null -eq $manifest.performance_result) "Incident must not contain a performance result."
Assert-True ($manifest.repository_base -eq "b77f2bce6e7875ab065e09894f45915585c9f156") "Repository base changed."
Assert-True ($null -ne $manifest.evidence -and $manifest.evidence.Count -gt 0) "Manifest evidence list is empty."

$checksumHashes = [System.Collections.Generic.Dictionary[string, string]]::new([System.StringComparer]::Ordinal)
$checksumCasePaths = New-OrdinalIgnoreCaseSet
$sumLines = @(Get-Content -LiteralPath $checksumPath)
Assert-True ($sumLines.Count -gt 0) "SHA256SUMS is empty."
foreach ($line in $sumLines) {
    if ($line -notmatch '^([0-9a-f]{64})  (.+)$') { throw "Malformed SHA256SUMS line: $line" }
    $expected = $Matches[1]
    $relative = $Matches[2]
    Assert-CanonicalRelativePath -RelativePath $relative -Context "Checksum"
    Assert-True ($relative -cne "SHA256SUMS") "SHA256SUMS must not checksum itself."
    Assert-True (-not $checksumHashes.ContainsKey($relative)) "Duplicate checksum path: $relative"
    Assert-True ($checksumCasePaths.Add($relative)) "Case-colliding checksum path: $relative"
    $checksumHashes.Add($relative, $expected)
}

$actualPortablePaths = New-OrdinalSet
$actualPortableCasePaths = New-OrdinalIgnoreCaseSet
$actualDirectoryPaths = New-OrdinalSet
$bundleItems = @(Get-ChildItem -LiteralPath $root -Recurse -Force -ErrorAction Stop)
foreach ($item in $bundleItems) {
    Assert-True (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0) "Bundle item is a reparse point: $($item.FullName)"
}
foreach ($directory in $bundleItems | Where-Object { $_.PSIsContainer }) {
    $relative = [System.IO.Path]::GetRelativePath($root, $directory.FullName).Replace('\', '/')
    Assert-CanonicalRelativePath -RelativePath $relative -Context "Bundle directory"
    Assert-True ($actualDirectoryPaths.Add($relative)) "Duplicate bundle directory: $relative"
}
Assert-True ($actualDirectoryPaths.Count -eq 1 -and $actualDirectoryPaths.Contains("raw")) "Bundle directory set must contain exactly raw/."
foreach ($file in $bundleItems | Where-Object { -not $_.PSIsContainer }) {
    $relative = [System.IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
    Assert-CanonicalRelativePath -RelativePath $relative -Context "Bundle"
    Assert-True ($actualPortablePaths.Add($relative)) "Duplicate bundle path: $relative"
    Assert-True ($actualPortableCasePaths.Add($relative)) "Case-colliding bundle path: $relative"
}
Assert-True ($actualPortablePaths.Contains("SHA256SUMS")) "Bundle SHA256SUMS is missing."

$expectedChecksummedCount = $actualPortablePaths.Count - 1
Assert-True ($checksumHashes.Count -eq $expectedChecksummedCount) "Checksum set cardinality does not match every portable file except SHA256SUMS."
foreach ($relative in $actualPortablePaths) {
    if ($relative -ceq "SHA256SUMS") { continue }
    Assert-True ($checksumHashes.ContainsKey($relative)) "Unchecksummed portable file: $relative"
}
foreach ($relative in $checksumHashes.Keys) {
    Assert-True ($actualPortablePaths.Contains($relative)) "Checksum path is not a portable bundle file: $relative"
    $path = Join-Path $root $relative
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    Assert-True ($actual -eq $checksumHashes[$relative]) "SHA-256 mismatch: $relative"
}

$manifestPaths = New-OrdinalSet
$manifestCasePaths = New-OrdinalIgnoreCaseSet
foreach ($entry in $manifest.evidence) {
    $relative = [string] $entry.path
    Assert-CanonicalRelativePath -RelativePath $relative -Context "Manifest evidence"
    Assert-True ($relative.StartsWith("raw/", [System.StringComparison]::Ordinal)) "Manifest evidence must be under raw/: $relative"
    Assert-True ($manifestPaths.Add($relative)) "Duplicate manifest evidence path: $relative"
    Assert-True ($manifestCasePaths.Add($relative)) "Case-colliding manifest evidence path: $relative"
    Assert-True ($checksumHashes.ContainsKey($relative)) "Manifest evidence is not checksummed: $relative"

    $path = Join-Path $root $relative
    Assert-True (Test-Path -LiteralPath $path -PathType Leaf) "Missing manifest evidence: $relative"
    $file = Get-Item -LiteralPath $path
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    Assert-True ($file.Length -eq $entry.bytes) "Byte length mismatch: $relative"
    Assert-True ($hash -eq $entry.sha256) "Manifest hash mismatch: $relative"
    Assert-True ($hash -eq $checksumHashes[$relative]) "Manifest/checksum hash disagreement: $relative"
}

$requiredPortablePaths = New-OrdinalSet
foreach ($relative in @(
    "README.md",
    "SHA256SUMS",
    "collect-read-only.ps1",
    "manifest.json",
    "validate.ps1"
)) {
    [void] $requiredPortablePaths.Add($relative)
}
foreach ($relative in $manifestPaths) {
    [void] $requiredPortablePaths.Add($relative)
}
foreach ($relative in $requiredPortablePaths) {
    Assert-True ($actualPortablePaths.Contains($relative)) "Required portable artifact is missing: $relative"
}
foreach ($relative in $actualPortablePaths) {
    Assert-True ($requiredPortablePaths.Contains($relative)) "Undeclared portable artifact is present: $relative"
}
Assert-True ($actualPortablePaths.Count -eq $requiredPortablePaths.Count) "Portable file set cardinality does not match the exact incident contract."

$actualRawPaths = New-OrdinalSet
$rawRoot = Join-Path $root "raw"
Assert-True (Test-Path -LiteralPath $rawRoot -PathType Container) "Bundle raw directory is missing."
foreach ($file in Get-ChildItem -LiteralPath $rawRoot -Recurse -File -Force -ErrorAction Stop) {
    $relative = [System.IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
    [void] $actualRawPaths.Add($relative)
}
Assert-True ($manifestPaths.Count -eq $actualRawPaths.Count) "Manifest/raw set cardinality mismatch."
foreach ($relative in $actualRawPaths) {
    Assert-True ($manifestPaths.Contains($relative)) "Raw file is absent from manifest evidence: $relative"
}
foreach ($relative in $manifestPaths) {
    Assert-True ($actualRawPaths.Contains($relative)) "Manifest evidence is absent from raw/: $relative"
}

$kernel = Read-Evidence "raw/nimo-2-kernel-oom-window.stdout.log"
Assert-True (([regex]::Matches($kernel, 'invoked oom-killer')).Count -eq 4) "Expected exactly four global OOM invocations."
Assert-True ($kernel.Contains("gpu_active:114041696kB")) "Missing authoritative HMM accounting."
Assert-True ($kernel.Contains("Killed process 2148915 (ggml-rpc-server)")) "Missing production-worker OOM kill."
Assert-True ($kernel.Contains("2244520") -and $kernel.Contains("2244762") -and $kernel.Contains("2244763")) "Missing active build process tree."

$worker = Read-Evidence "raw/nimo-2-worker-restart-window.stdout.log"
Assert-True ($worker.Contains("Failed with result 'oom-kill'")) "Missing worker oom-kill result."
Assert-True ($worker.Contains("restart counter is at 1")) "Missing worker restart receipt."
Assert-True ($worker.Contains("[2248760]")) "Missing replacement worker PID."

$coordinator = Read-Evidence "raw/nimo-1-coordinator-restart-window.stdout.log"
Assert-True ($coordinator.Contains("Remote RPC server crashed or returned malformed response")) "Missing stale RPC failure."
Assert-True ($coordinator.Contains("Failed with result 'core-dump'")) "Missing coordinator failure result."
Assert-True ($coordinator.Contains("5 tokens") -and $coordinator.Contains("1 tokens")) "Missing real recovery inference receipt."

$nimo1 = Read-Evidence "raw/nimo-1-current-authority.stdout.log"
$nimo2 = Read-Evidence "raw/nimo-2-current-authority.stdout.log"
Assert-True ($nimo1.Contains("MainPID=3113343") -and $nimo1.Contains("InvocationID=0656332b63a140eab7214627baa43253") -and $nimo1.Contains("NRestarts=1")) "nimo-1 recovery authority mismatch."
Assert-True ($nimo1.Contains('{"status":"ok"}')) "nimo-1 health receipt missing."
Assert-True ($nimo2.Contains("MainPID=2248760") -and $nimo2.Contains("InvocationID=d15fe49610274e77bd9a3d84a0b791a5") -and $nimo2.Contains("NRestarts=1")) "nimo-2 recovery authority mismatch."

$cache = Read-Evidence "raw/nimo-2-build-cmake-cache.stdout.log"
foreach ($expected in @(
    "CMAKE_BUILD_TYPE:STRING=Release",
    "CMAKE_HIP_ARCHITECTURES:UNINITIALIZED=gfx1151",
    "GGML_HIP:BOOL=ON",
    "GGML_HIP_FORCE_MMQ:BOOL=ON",
    "GGML_HIP_ROCMFPX_FFN_Q8_REUSE:BOOL=OFF",
    "GGML_HIP_ROCMFPX_MMVQ_SUM_FREE:BOOL=OFF",
    "GGML_CUDA:BOOL=OFF",
    "GGML_VULKAN:BOOL=OFF",
    "LLAMA_BUILD_SERVER:BOOL=OFF",
    "LLAMA_BUILD_TESTS:BOOL=ON",
    "CMAKE_GENERATOR:INTERNAL=Ninja"
)) {
    Assert-True ($cache.Contains($expected)) "Missing build configuration: $expected"
}

$controller = Read-Evidence "raw/controller-observed-facts.txt"
Assert-True ($controller.Contains("It was never configured and never built.")) "Missing ON-build non-result boundary."
Assert-True ($controller.Contains("No completed output, binary, benchmark, or performance result was retained.")) "Missing no-benchmark disposition."

$secretPatterns = [ordered]@{
    private_key   = '-----BEGIN (?:RSA |OPENSSH |EC |DSA |ENCRYPTED )?PRIVATE KEY-----'
    bearer        = '(?i)authorization:\s*bearer\s+[A-Za-z0-9._~+/=-]{8,}'
    credential    = '(?i)["'']?(api[_-]?key|access[_-]?token|client[_-]?secret|password)["'']?\s*[=:]\s*["'']?[^\s"'']{8,}'
    github_token  = '(?i)\b(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,})\b'
    openai_token  = '\bsk-[A-Za-z0-9_-]{20,}\b'
    huggingface   = '\bhf_[A-Za-z0-9]{20,}\b'
    aws_access    = '\b(?:AKIA|ASIA)[A-Z0-9]{16}\b'
    aws_secret    = '(?i)["'']?aws[_-]?secret[_-]?access[_-]?key["'']?\s*[=:]\s*["'']?[A-Za-z0-9/+=]{40}'
    slack_token   = '\bxox[baprs]-[A-Za-z0-9-]{10,}\b'
    basic_auth    = '(?i)authorization:\s*basic\s+[A-Za-z0-9+/=]{8,}'
    jwt            = '\beyJ[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,}\b'
    url_userinfo   = '(?i)\b(?:https?|ssh)://[^\s/@:]+:[^\s/@]+@'
    sensitive_assignment = '(?i)["'']?(?:refresh[_-]?token|private[_-]?token)["'']?\s*[=:]\s*["'']?[A-Za-z0-9._~+/=-]{16,}'
    generic_secret_assignment = '(?i)(?:^|[^A-Za-z0-9_])["'']?(?:token|secret|secret[_-]?key|auth[_-]?token)["'']?\s*[=:]\s*["'']?[A-Za-z0-9._~+/=-]{16,}'
}
foreach ($relative in $actualPortablePaths) {
    $path = Join-Path $root $relative
    $bytes = [System.IO.File]::ReadAllBytes($path)
    try {
        $text = $strictUtf8.GetString($bytes)
    } catch {
        throw "Portable artifact is not strict UTF-8 text: $relative"
    }
    foreach ($name in $secretPatterns.Keys) {
        Assert-True (-not [regex]::IsMatch($text, $secretPatterns[$name])) "Possible $name secret pattern in $relative"
    }
}

"PASS: closed-world bundle/checksum/raw/manifest sets, hashes, OOM timeline, recovery authority, build boundary, and full portable secret scan"
