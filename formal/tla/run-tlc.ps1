param(
    [Parameter(Mandatory = $true)]
    [string] $EvidenceRoot,
    [int] $SafetyRepetitions = 3
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$java = 'C:\Program Files\Stirling-PDF\runtime\jre\bin\java.exe'
$jar = 'C:\Users\britt\Documents\Custom_Inference_Project\sources\tools\tlaplus\v1.7.4\tla2tools.jar'
$specRoot = $PSScriptRoot
$spec = Join-Path $specRoot 'HaloFPXPublication.tla'

if (-not (Test-Path -LiteralPath $java -PathType Leaf)) { throw "Java runtime not found: $java" }
if (-not (Test-Path -LiteralPath $jar -PathType Leaf)) { throw "Pinned TLC JAR not found: $jar" }
if ($SafetyRepetitions -lt 1 -or $SafetyRepetitions -gt 20) { throw 'SafetyRepetitions must be 1..20' }

$resolvedEvidence = [IO.Path]::GetFullPath($EvidenceRoot)
$forbidden = [IO.Path]::GetFullPath((Join-Path $specRoot '..\..'))
if ($resolvedEvidence.StartsWith($forbidden, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Raw evidence must remain outside the implementation repository'
}
[IO.Directory]::CreateDirectory($resolvedEvidence) | Out-Null

$jarSha256 = (Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if ($jarSha256 -ne '936a262061c914694dfd669a543be24573c45d5aa0ff20a8b96b23d01e050e88') {
    throw "Unexpected TLC JAR SHA-256: $jarSha256"
}

$matrix = @(
    [pscustomobject]@{ Config='PublicationCrashSafety.cfg'; Workers=8; ExpectedExit=0; ExpectedText='No error has been found'; Repetitions=$SafetyRepetitions },
    [pscustomobject]@{ Config='GenerationChainSafety.cfg'; Workers=4; ExpectedExit=0; ExpectedText='No error has been found'; Repetitions=$SafetyRepetitions },
    [pscustomobject]@{ Config='TwoLineageIsolation.cfg'; Workers=8; ExpectedExit=0; ExpectedText='No error has been found'; Repetitions=$SafetyRepetitions },
    [pscustomobject]@{ Config='PublicationProgress.cfg'; Workers=1; ExpectedExit=0; ExpectedText='Finished checking temporal properties'; Repetitions=$SafetyRepetitions },
    [pscustomobject]@{ Config='NegativeAckEarly.cfg'; Workers=1; ExpectedExit=-1; ExpectedText='Invariant NoAckBeforeVisibility is violated'; Repetitions=1 },
    [pscustomobject]@{ Config='NegativeMixedRecovery.cfg'; Workers=1; ExpectedExit=-1; ExpectedText='Invariant NoMixedGenerationRecovery is violated'; Repetitions=1 },
    [pscustomobject]@{ Config='NegativeRecoverNewest.cfg'; Workers=1; ExpectedExit=-1; ExpectedText='Invariant RecoveryUsesExactAnchor is violated'; Repetitions=1 },
    [pscustomobject]@{ Config='NegativeReplayAnchor.cfg'; Workers=1; ExpectedExit=-1; ExpectedText='Invariant RecoveryUsesExactAnchor is violated'; Repetitions=1 },
    [pscustomobject]@{ Config='NegativeCrossLineageAnchor.cfg'; Workers=1; ExpectedExit=-1; ExpectedText='Invariant RecoveryUsesExactAnchor is violated'; Repetitions=1 }
)

$records = [Collections.Generic.List[object]]::new()
$sequence = 0
foreach ($entry in $matrix) {
    $configPath = Join-Path $specRoot $entry.Config
    $configSha256 = (Get-FileHash -LiteralPath $configPath -Algorithm SHA256).Hash.ToLowerInvariant()
    for ($repetition = 1; $repetition -le $entry.Repetitions; ++$repetition) {
        ++$sequence
        $seed = 1000003 + ($sequence * 7919)
        $name = '{0:d2}__{1}__r{2}' -f $sequence, [IO.Path]::GetFileNameWithoutExtension($entry.Config), $repetition
        $stdoutPath = Join-Path $resolvedEvidence ($name + '__stdout.txt')
        $stderrPath = Join-Path $resolvedEvidence ($name + '__stderr.txt')
        $metaDirectory = Join-Path $resolvedEvidence ($name + '__tlc-state')

        $arguments = @(
            '-XX:+UseParallelGC', '-jar', $jar,
            '-workers', [string]$entry.Workers,
            '-seed', [string]$seed,
            '-coverage', '1',
            '-metadir', $metaDirectory,
            '-config', $entry.Config,
            'HaloFPXPublication.tla'
        )
        $psi = [Diagnostics.ProcessStartInfo]::new()
        $psi.FileName = $java
        $psi.WorkingDirectory = $specRoot
        $psi.UseShellExecute = $false
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        foreach ($argument in $arguments) { $psi.ArgumentList.Add($argument) }
        $command = '"' + $java + '" ' + (($arguments | ForEach-Object {
            if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
        }) -join ' ')

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

        $combined = $stdout + $stderr
        $exitMatches = if ($entry.ExpectedExit -eq -1) {
            $process.ExitCode -ne 0
        } else {
            $process.ExitCode -eq $entry.ExpectedExit
        }
        $passed = $exitMatches -and $combined.Contains($entry.ExpectedText)
        $statesGenerated = $null
        $distinctStates = $null
        $depth = $null
        $matches = [regex]::Matches($combined, '(?m)^(\d+) states generated, (\d+) distinct states found')
        if ($matches.Count -gt 0) {
            $last = $matches[$matches.Count - 1]
            $statesGenerated = [uint64]$last.Groups[1].Value
            $distinctStates = [uint64]$last.Groups[2].Value
        }
        $depthMatch = [regex]::Match($combined, 'The depth of the complete state graph search is (\d+)')
        if ($depthMatch.Success) { $depth = [uint64]$depthMatch.Groups[1].Value }

        $artifactPaths = @($stdoutPath, $stderrPath)
        if (Test-Path -LiteralPath $metaDirectory -PathType Container) {
            $artifactPaths += @(Get-ChildItem -LiteralPath $metaDirectory -File -Recurse |
                Sort-Object FullName | ForEach-Object { $_.FullName })
        }
        $artifacts = @($artifactPaths | ForEach-Object {
            $artifact = Get-Item -LiteralPath $_
            [pscustomobject]@{
                path = [IO.Path]::GetRelativePath($resolvedEvidence, $artifact.FullName).Replace('\', '/')
                bytes = $artifact.Length
                sha256 = (Get-FileHash -LiteralPath $artifact.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        })

        $records.Add([pscustomobject]@{
            run = $name
            config = $entry.Config
            repetition = $repetition
            workers = $entry.Workers
            seed = $seed
            command = $command
            arguments = $arguments
            expected_exit = $entry.ExpectedExit
            exit = $process.ExitCode
            expected_text = $entry.ExpectedText
            passed = $passed
            started_utc = $started.ToString('o')
            finished_utc = $finished.ToString('o')
            elapsed_ms = [int64]($finished - $started).TotalMilliseconds
            states_generated = $statesGenerated
            distinct_states = $distinctStates
            depth = $depth
            config_sha256 = $configSha256
            stdout_sha256 = (Get-FileHash -LiteralPath $stdoutPath -Algorithm SHA256).Hash.ToLowerInvariant()
            stderr_sha256 = (Get-FileHash -LiteralPath $stderrPath -Algorithm SHA256).Hash.ToLowerInvariant()
            artifacts = $artifacts
        })
        if (-not $passed) { throw "TLC contract failed for $name; inspect $stdoutPath and $stderrPath" }
    }
}

$javaVersion = (& $java -version 2>&1 | Out-String).Trim()
$manifest = [ordered]@{
    schema = 'halofpx.tlc-run-manifest.v2'
    created_utc = [DateTimeOffset]::UtcNow.ToString('o')
    host = [Environment]::MachineName
    java = $java
    java_sha256 = (Get-FileHash -LiteralPath $java -Algorithm SHA256).Hash.ToLowerInvariant()
    java_version = $javaVersion
    tla2tools_jar = $jar
    tla2tools_sha256 = $jarSha256
    spec = $spec
    spec_sha256 = (Get-FileHash -LiteralPath $spec -Algorithm SHA256).Hash.ToLowerInvariant()
    git_head = (git -C (Join-Path $specRoot '..\..') rev-parse HEAD).Trim()
    git_status = @((git -C (Join-Path $specRoot '..\..') status --short))
    runs = $records
}
$manifestPath = Join-Path $resolvedEvidence 'run-manifest.json'
[IO.File]::WriteAllText(
    $manifestPath,
    ($manifest | ConvertTo-Json -Depth 12),
    [Text.UTF8Encoding]::new($false))

$records | Format-Table run, passed, exit, states_generated, distinct_states, depth, elapsed_ms -AutoSize
Write-Output "Evidence manifest: $manifestPath"
