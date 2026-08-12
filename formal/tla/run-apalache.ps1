param(
    [Parameter(Mandatory = $true)]
    [string] $EvidenceRoot,
    [ValidateRange(1, 20)]
    [int] $Length = 5
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedJarSha256 = '1c2500ec2b014fcf41a7b0bd4c30fc3204b69377028fd689224eea9cf23f66f5'
$specRoot = $PSScriptRoot
$repoRoot = [IO.Path]::GetFullPath((Join-Path $specRoot '../..'))
$spec = Join-Path $specRoot 'HaloFPXPublication.tla'
$config = Join-Path $specRoot 'GenerationChainSafety.cfg'
$resolvedEvidence = [IO.Path]::GetFullPath($EvidenceRoot)
$repositoryBoundary = $repoRoot.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar

if ($resolvedEvidence.Equals($repoRoot, [StringComparison]::OrdinalIgnoreCase) -or
    $resolvedEvidence.StartsWith($repositoryBoundary, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Raw evidence must remain outside the monorepo'
}

function Resolve-JavaRuntime {
    $configured = [Environment]::GetEnvironmentVariable('HALOFPX_JAVA')
    if (-not [string]::IsNullOrWhiteSpace($configured)) {
        $candidate = [Environment]::ExpandEnvironmentVariables($configured)
        if (-not [IO.Path]::IsPathRooted($candidate)) { $candidate = Join-Path $repoRoot $candidate }
        $candidate = [IO.Path]::GetFullPath($candidate)
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            throw "HALOFPX_JAVA does not identify a Java executable: $candidate"
        }
        return $candidate
    }

    $javaHome = [Environment]::GetEnvironmentVariable('JAVA_HOME')
    if (-not [string]::IsNullOrWhiteSpace($javaHome)) {
        foreach ($name in @('java.exe', 'java')) {
            $candidate = Join-Path $javaHome (Join-Path 'bin' $name)
            if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                return [IO.Path]::GetFullPath($candidate)
            }
        }
    }

    $command = Get-Command java -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -ne $command) { return $command.Source }
    throw 'Java runtime not found. Set HALOFPX_JAVA, set JAVA_HOME, or add java to PATH.'
}

function Resolve-PinnedJar {
    $configured = [Environment]::GetEnvironmentVariable('HALOFPX_APALACHE_JAR')
    $candidate = if ([string]::IsNullOrWhiteSpace($configured)) {
        Join-Path $repoRoot 'project/sources/tools/apalache/v0.57.0/extracted/apalache-0.57.0/lib/apalache.jar'
    } else {
        [Environment]::ExpandEnvironmentVariables($configured)
    }
    if (-not [IO.Path]::IsPathRooted($candidate)) { $candidate = Join-Path $repoRoot $candidate }
    $candidate = [IO.Path]::GetFullPath($candidate)
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "Pinned Apalache JAR not found: $candidate. Restore project/sources/tools or set HALOFPX_APALACHE_JAR."
    }
    return $candidate
}

$java = Resolve-JavaRuntime
$jar = Resolve-PinnedJar
$jarSha256 = (Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if ($jarSha256 -ne $expectedJarSha256) { throw "Unexpected Apalache JAR SHA-256: $jarSha256" }
[IO.Directory]::CreateDirectory($resolvedEvidence) | Out-Null

function Invoke-ApalacheRun {
    param([string] $Name, [string[]] $Arguments, [string] $ExpectedText)
    $stdoutPath = Join-Path $resolvedEvidence ($Name + '__stdout.txt')
    $stderrPath = Join-Path $resolvedEvidence ($Name + '__stderr.txt')
    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $java
    $psi.WorkingDirectory = $specRoot
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $psi.ArgumentList.Add($argument) }
    $started = [DateTimeOffset]::UtcNow
    $process = [Diagnostics.Process]::Start($psi)
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    $finished = [DateTimeOffset]::UtcNow
    [IO.File]::WriteAllText($stdoutPath, $stdout, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText($stderrPath, $stderr, [Text.UTF8Encoding]::new($false))
    $passed = $process.ExitCode -eq 0 -and ($stdout + $stderr).Contains($ExpectedText)
    if (-not $passed) { throw "Apalache contract failed for $Name; inspect $stdoutPath and $stderrPath" }
    $command = '"' + $java + '" ' + (($Arguments | ForEach-Object {
        if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
    }) -join ' ')
    [pscustomobject]@{
        run = $Name; command = $command; arguments = $Arguments
        expected_text = $ExpectedText; passed = $passed; exit = $process.ExitCode
        started_utc = $started.ToString('o'); finished_utc = $finished.ToString('o')
        elapsed_ms = [int64]($finished - $started).TotalMilliseconds
        stdout = [IO.Path]::GetFileName($stdoutPath); stderr = [IO.Path]::GetFileName($stderrPath)
    }
}

$typeOutput = Join-Path $resolvedEvidence 'typecheck-output'
$checkOutput = Join-Path $resolvedEvidence 'bounded-check-output'
$records = @(
    Invoke-ApalacheRun -Name '01__typecheck' -ExpectedText 'Type checker [OK]' -Arguments @(
        '-jar', $jar, ('--out-dir=' + $typeOutput), 'typecheck', 'HaloFPXPublication.tla')
    Invoke-ApalacheRun -Name '02__bounded-safety' -ExpectedText "Checker reports no error up to computation length $Length" -Arguments @(
        '-jar', $jar, ('--out-dir=' + $checkOutput), 'check',
        '--config=GenerationChainSafety.cfg', '--inv=Safety', ('--length=' + $Length),
        'HaloFPXPublication.tla')
)

$artifactPaths = @(Get-ChildItem -LiteralPath $resolvedEvidence -File -Recurse | Sort-Object FullName)
$artifacts = @($artifactPaths | ForEach-Object {
    [pscustomobject]@{
        path = [IO.Path]::GetRelativePath($resolvedEvidence, $_.FullName).Replace('\', '/')
        bytes = $_.Length
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
})
$manifest = [ordered]@{
    schema = 'halofpx.apalache-run-manifest.v1'
    created_utc = [DateTimeOffset]::UtcNow.ToString('o')
    host = [Environment]::MachineName
    java = $java
    java_version = (& $java -version 2>&1 | Out-String).Trim()
    apalache_jar = $jar
    apalache_jar_sha256 = $jarSha256
    spec_sha256 = (Get-FileHash -LiteralPath $spec -Algorithm SHA256).Hash.ToLowerInvariant()
    config_sha256 = (Get-FileHash -LiteralPath $config -Algorithm SHA256).Hash.ToLowerInvariant()
    git_head = (git -C $repoRoot rev-parse HEAD).Trim()
    git_status = @((git -C $repoRoot status --short))
    runs = $records
    artifacts = $artifacts
}
$manifestPath = Join-Path $resolvedEvidence 'run-manifest.json'
[IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json -Depth 10), [Text.UTF8Encoding]::new($false))
$records | Format-Table run, passed, exit, elapsed_ms -AutoSize
Write-Output "Evidence manifest: $manifestPath"
