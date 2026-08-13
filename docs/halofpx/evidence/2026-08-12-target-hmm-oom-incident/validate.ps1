[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

function Assert-True {
    param([bool] $Condition, [string] $Message)
    if (-not $Condition) { throw $Message }
}

function Read-Evidence {
    param([string] $RelativePath)
    Get-Content -Raw -LiteralPath (Join-Path $root $RelativePath)
}

$manifest = Get-Content -Raw -LiteralPath (Join-Path $root "manifest.json") | ConvertFrom-Json
Assert-True ($manifest.schema -eq "halofpx.target-safety-incident.v1") "Unexpected manifest schema."
Assert-True ($manifest.classification -eq "production-safety-incident") "Incident classification changed."
Assert-True ($manifest.benchmark_valid -eq $false) "Incident must remain an invalid benchmark."
Assert-True ($null -eq $manifest.performance_result) "Incident must not contain a performance result."
Assert-True ($manifest.repository_base -eq "b77f2bce6e7875ab065e09894f45915585c9f156") "Repository base changed."

$sumLines = Get-Content -LiteralPath (Join-Path $root "SHA256SUMS")
foreach ($line in $sumLines) {
    if ($line -notmatch '^([0-9a-f]{64})  (.+)$') { throw "Malformed SHA256SUMS line: $line" }
    $expected = $Matches[1]
    $relative = $Matches[2]
    Assert-True (-not [System.IO.Path]::IsPathRooted($relative)) "Absolute checksum path rejected: $relative"
    Assert-True ($relative -notmatch '(^|[\\/])\.\.([\\/]|$)') "Parent traversal rejected: $relative"
    $path = Join-Path $root $relative
    Assert-True (Test-Path -LiteralPath $path -PathType Leaf) "Missing checksummed file: $relative"
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    Assert-True ($actual -eq $expected) "SHA-256 mismatch: $relative"
}

foreach ($entry in $manifest.evidence) {
    $path = Join-Path $root $entry.path
    Assert-True (Test-Path -LiteralPath $path -PathType Leaf) "Missing manifest evidence: $($entry.path)"
    $file = Get-Item -LiteralPath $path
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    Assert-True ($file.Length -eq $entry.bytes) "Byte length mismatch: $($entry.path)"
    Assert-True ($hash -eq $entry.sha256) "Manifest hash mismatch: $($entry.path)"
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

$secretPatterns = @(
    '-----BEGIN (RSA |OPENSSH |EC )?PRIVATE KEY-----',
    '(?i)authorization:\s*bearer\s+',
    '(?i)(api[_-]?key|access[_-]?token|client[_-]?secret)\s*[=:]\s*[^\s\"]+'
)
foreach ($entry in $manifest.evidence) {
    if ($entry.bytes -eq 0) { continue }
    $text = Read-Evidence $entry.path
    foreach ($pattern in $secretPatterns) {
        Assert-True (-not [regex]::IsMatch($text, $pattern)) "Possible secret pattern in $($entry.path)"
    }
}

"PASS: hashes, manifest, OOM timeline, recovery authority, build boundary, and secret-pattern scan"
