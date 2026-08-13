[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$incidentRelativePath = "docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident"
$incidentRoot = Join-Path $repoRoot $incidentRelativePath
$powerShellPath = (Get-Command pwsh -CommandType Application -ErrorAction Stop |
    Select-Object -First 1).Source
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "halofpx-oom-incident-validator-tests-{0}" -f [guid]::NewGuid().ToString("N")
)
$script:passed = 0
$script:skipped = 0

function Assert-True {
    param([bool] $Condition, [string] $Message)
    if (-not $Condition) { throw $Message }
}

function New-TestBundle {
    param([Parameter(Mandatory)] [string] $Name)

    $destination = Join-Path $testRoot $Name
    Copy-Item -LiteralPath $incidentRoot -Destination $destination -Recurse -Force
    return $destination
}

function Write-Utf8Text {
    param(
        [Parameter(Mandatory)] [string] $Path,
        [Parameter(Mandatory)] [AllowEmptyString()] [string] $Text
    )

    [System.IO.File]::WriteAllText($Path, $Text, $utf8NoBom)
}

function Update-Checksum {
    param(
        [Parameter(Mandatory)] [string] $Bundle,
        [Parameter(Mandatory)] [string] $RelativePath
    )

    $checksumPath = Join-Path $Bundle "SHA256SUMS"
    $targetPath = Join-Path $Bundle $RelativePath
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $targetPath).Hash.ToLowerInvariant()
    $lines = [System.IO.File]::ReadAllLines($checksumPath)
    $updated = 0
    for ($index = 0; $index -lt $lines.Count; $index++) {
        if ($lines[$index] -match '^([0-9a-f]{64})  (.+)$' -and $Matches[2] -ceq $RelativePath) {
            $lines[$index] = "$hash  $RelativePath"
            $updated++
        }
    }
    Assert-True ($updated -eq 1) "Expected one checksum entry for $RelativePath; found $updated."
    Write-Utf8Text -Path $checksumPath -Text (($lines -join "`n") + "`n")
}

function Add-Checksum {
    param(
        [Parameter(Mandatory)] [string] $Bundle,
        [Parameter(Mandatory)] [string] $RelativePath
    )

    $targetPath = Join-Path $Bundle $RelativePath
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $targetPath).Hash.ToLowerInvariant()
    $checksumPath = Join-Path $Bundle "SHA256SUMS"
    [System.IO.File]::AppendAllText($checksumPath, "$hash  $RelativePath`n", $utf8NoBom)
}

function Remove-Checksum {
    param(
        [Parameter(Mandatory)] [string] $Bundle,
        [Parameter(Mandatory)] [string] $RelativePath
    )

    $checksumPath = Join-Path $Bundle "SHA256SUMS"
    $lines = @([System.IO.File]::ReadAllLines($checksumPath) | Where-Object {
        $_ -notmatch ('^[0-9a-f]{64}  ' + [regex]::Escape($RelativePath) + '$')
    })
    Write-Utf8Text -Path $checksumPath -Text (($lines -join "`n") + "`n")
}

function Invoke-BundleValidator {
    param([Parameter(Mandatory)] [string] $Bundle)

    $validatorPath = Join-Path $Bundle "validate.ps1"
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $powerShellPath
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    [void] $startInfo.ArgumentList.Add("-NoLogo")
    [void] $startInfo.ArgumentList.Add("-NoProfile")
    [void] $startInfo.ArgumentList.Add("-NonInteractive")
    [void] $startInfo.ArgumentList.Add("-File")
    [void] $startInfo.ArgumentList.Add($validatorPath)

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        Assert-True ($process.Start()) "Failed to start incident validator."
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $deadlineExceeded = -not $process.WaitForExit(30000)
        if ($deadlineExceeded) {
            try {
                $process.Kill($true)
            } catch {
                throw "Incident validator exceeded its deadline and process-tree termination failed: $($_.Exception.Message)"
            }
            if (-not $process.WaitForExit(5000)) {
                throw "Incident validator exceeded its deadline and its parent process was not reaped."
            }
        }
        $pipeTasks = [System.Threading.Tasks.Task[]] @($stdoutTask, $stderrTask)
        if (-not [System.Threading.Tasks.Task]::WaitAll($pipeTasks, [TimeSpan]::FromSeconds(5))) {
            $process.StandardOutput.Dispose()
            $process.StandardError.Dispose()
            throw "Incident validator output pipes did not drain within five seconds."
        }
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        if ($deadlineExceeded) {
            throw "Incident validator exceeded its 30-second test deadline."
        }
        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            Output = ($stdout + "`n" + $stderr).Trim()
        }
    } finally {
        $process.Dispose()
    }
}

function Assert-ValidationPasses {
    param([Parameter(Mandatory)] [string] $Bundle)

    $result = Invoke-BundleValidator -Bundle $Bundle
    Assert-True ($result.ExitCode -eq 0) "Validator unexpectedly failed:`n$($result.Output)"
    Assert-True ($result.Output.Contains("PASS:")) "Validator returned success without its PASS receipt."
}

function Assert-ValidationFails {
    param(
        [Parameter(Mandatory)] [string] $Bundle,
        [Parameter(Mandatory)] [string] $ExpectedMessage
    )

    $result = Invoke-BundleValidator -Bundle $Bundle
    Assert-True ($result.ExitCode -ne 0) "Validator unexpectedly accepted a corrupt fixture."
    Assert-True ($result.Output.Contains($ExpectedMessage)) (
        "Validator failed for an unexpected reason. Expected '$ExpectedMessage'; output:`n$($result.Output)"
    )
}

function Invoke-Test {
    param(
        [Parameter(Mandatory)] [string] $Name,
        [Parameter(Mandatory)] [scriptblock] $Body
    )

    & $Body
    $script:passed++
    Write-Output "PASS: $Name"
}

function Skip-Test {
    param(
        [Parameter(Mandatory)] [string] $Name,
        [Parameter(Mandatory)] [string] $Reason
    )

    $script:skipped++
    Write-Output "SKIP: ${Name}: $Reason"
}

Assert-True (Test-Path -LiteralPath $incidentRoot -PathType Container) "Incident bundle is missing: $incidentRoot"
New-Item -ItemType Directory -Path $testRoot -ErrorAction Stop | Out-Null

try {
    Invoke-Test "baseline bundle passes" {
        Assert-ValidationPasses -Bundle $incidentRoot
    }

    Invoke-Test "manifest-declared empty evidence files remain valid" {
        $manifest = Get-Content -Raw -LiteralPath (Join-Path $incidentRoot "manifest.json") | ConvertFrom-Json
        $emptyEntries = @($manifest.evidence | Where-Object { $_.bytes -eq 0 })
        Assert-True ($emptyEntries.Count -gt 0) "Baseline no longer exercises empty evidence files."
        foreach ($entry in $emptyEntries) {
            $file = Get-Item -LiteralPath (Join-Path $incidentRoot ([string] $entry.path))
            Assert-True ($file.Length -eq 0) "Manifest-declared empty fixture is not empty: $($entry.path)"
        }
        Assert-ValidationPasses -Bundle $incidentRoot
    }

    Invoke-Test "duplicate SHA256SUMS path is rejected" {
        $bundle = New-TestBundle "duplicate-checksum"
        $checksumPath = Join-Path $bundle "SHA256SUMS"
        $firstLine = [System.IO.File]::ReadAllLines($checksumPath)[0]
        [System.IO.File]::AppendAllText($checksumPath, "$firstLine`n", $utf8NoBom)
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Duplicate checksum path:"
    }

    Invoke-Test "duplicate manifest evidence path is rejected" {
        $bundle = New-TestBundle "duplicate-manifest-evidence"
        $manifestPath = Join-Path $bundle "manifest.json"
        $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
        $manifest.evidence = @($manifest.evidence) + @($manifest.evidence[0])
        Write-Utf8Text -Path $manifestPath -Text (($manifest | ConvertTo-Json -Depth 100) + "`n")
        Update-Checksum -Bundle $bundle -RelativePath "manifest.json"
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Duplicate manifest evidence path:"
    }

    Invoke-Test "duplicate JSON property is rejected" {
        $bundle = New-TestBundle "duplicate-json-property"
        $manifestPath = Join-Path $bundle "manifest.json"
        $manifestText = [System.IO.File]::ReadAllText($manifestPath)
        $duplicatePrefix = "{`n  " + '"schema": "duplicate-property-fixture",'
        $mutated = $manifestText -replace '^\{', $duplicatePrefix
        Write-Utf8Text -Path $manifestPath -Text $mutated
        Update-Checksum -Bundle $bundle -RelativePath "manifest.json"
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "contains duplicate properties"
    }

    Invoke-Test "hidden extra top-level file is rejected" {
        $bundle = New-TestBundle "hidden-extra-file"
        $hiddenPath = Join-Path $bundle ".undeclared-fixture"
        Write-Utf8Text -Path $hiddenPath -Text "closed-world fixture`n"
        if ($IsWindows) {
            (Get-Item -LiteralPath $hiddenPath).Attributes = (
                (Get-Item -LiteralPath $hiddenPath).Attributes -bor [System.IO.FileAttributes]::Hidden
            )
        }
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Checksum set cardinality"
    }

    Invoke-Test "undeclared empty directory is rejected" {
        $bundle = New-TestBundle "empty-directory"
        New-Item -ItemType Directory -Path (Join-Path $bundle "undeclared-empty") -ErrorAction Stop | Out-Null
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Bundle directory set must contain exactly raw/."
    }

    Invoke-Test "checksummed but unmanifested raw file is rejected" {
        $bundle = New-TestBundle "unmanifested-raw"
        $relative = "raw/unmanifested-fixture.txt"
        Write-Utf8Text -Path (Join-Path $bundle $relative) -Text "unmanifested raw fixture`n"
        Add-Checksum -Bundle $bundle -RelativePath $relative
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Undeclared portable artifact is present: $relative"
    }

    Invoke-Test "arbitrary checksummed non-raw file is rejected" {
        $bundle = New-TestBundle "checksummed-nonraw-extra"
        $relative = "checksummed-extra.txt"
        Write-Utf8Text -Path (Join-Path $bundle $relative) -Text "undeclared portable fixture`n"
        Add-Checksum -Bundle $bundle -RelativePath $relative
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Undeclared portable artifact is present: $relative"
    }

    Invoke-Test "required portable file cannot be removed with its checksum" {
        $bundle = New-TestBundle "removed-required-portable"
        $relative = "collect-read-only.ps1"
        Remove-Item -LiteralPath (Join-Path $bundle $relative) -Force
        Remove-Checksum -Bundle $bundle -RelativePath $relative
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Required portable artifact is missing: $relative"
    }

    $symlinkName = "raw symlink escape is rejected"
    $symlinkBundle = New-TestBundle "raw-symlink-escape"
    $outsidePath = Join-Path $testRoot "outside-raw-evidence.txt"
    Write-Utf8Text -Path $outsidePath -Text "outside bundle`n"
    $symlinkCreated = $false
    try {
        New-Item -ItemType SymbolicLink -Path (Join-Path $symlinkBundle "raw/escape-link") -Target $outsidePath -ErrorAction Stop | Out-Null
        $symlinkCreated = $true
    } catch {
        if (-not $IsWindows) {
            throw "Non-Windows CI host could not create the required symbolic-link fixture: $($_.Exception.Message)"
        }
        Skip-Test -Name $symlinkName -Reason "Windows host cannot create a symbolic link ($($_.Exception.Message))"
    }
    if ($symlinkCreated) {
        Invoke-Test $symlinkName {
            Assert-ValidationFails -Bundle $symlinkBundle -ExpectedMessage "Bundle item is a reparse point:"
        }
    }

    Invoke-Test "checksum traversal path is rejected" {
        $bundle = New-TestBundle "checksum-traversal"
        $checksumPath = Join-Path $bundle "SHA256SUMS"
        [System.IO.File]::AppendAllText($checksumPath, ("0" * 64) + "  ../escape`n", $utf8NoBom)
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "traversal or dot segment rejected"
    }

    Invoke-Test "checksum backslash path is rejected" {
        $bundle = New-TestBundle "checksum-backslash"
        $checksumPath = Join-Path $bundle "SHA256SUMS"
        [System.IO.File]::AppendAllText(
            $checksumPath,
            ("0" * 64) + "  raw\controller-observed-facts.txt`n",
            $utf8NoBom
        )
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "must use portable forward slashes"
    }

    Invoke-Test "case-colliding checksum path is rejected" {
        $bundle = New-TestBundle "checksum-case-collision"
        $checksumPath = Join-Path $bundle "SHA256SUMS"
        $sourceLine = [System.IO.File]::ReadAllLines($checksumPath) |
            Where-Object { $_ -match '  raw/controller-observed-facts\.txt$' } |
            Select-Object -First 1
        Assert-True ($null -ne $sourceLine) "Could not locate checksum collision fixture source."
        $collisionLine = $sourceLine.Replace(
            "raw/controller-observed-facts.txt",
            "RAW/controller-observed-facts.txt"
        )
        [System.IO.File]::AppendAllText($checksumPath, "$collisionLine`n", $utf8NoBom)
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Case-colliding checksum path:"
    }

    Invoke-Test "secret pattern in checksummed non-raw file is rejected" {
        $bundle = New-TestBundle "nonraw-secret"
        $readmePath = Join-Path $bundle "README.md"
        [System.IO.File]::AppendAllText(
            $readmePath,
            "`napi_key=HALOFPX_TEST_SECRET_1234567890`n",
            $utf8NoBom
        )
        Update-Checksum -Bundle $bundle -RelativePath "README.md"
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Possible credential secret pattern in README.md"
    }

    Invoke-Test "AWS secret assignment in checksummed non-raw file is rejected" {
        $bundle = New-TestBundle "nonraw-aws-secret"
        $readmePath = Join-Path $bundle "README.md"
        [System.IO.File]::AppendAllText(
            $readmePath,
            "`nAWS_SECRET_ACCESS_KEY=abcdefghijklmnopqrstuvwxyz1234567890ABCD`n",
            $utf8NoBom
        )
        Update-Checksum -Bundle $bundle -RelativePath "README.md"
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Possible aws_secret secret pattern in README.md"
    }

    Invoke-Test "quoted JSON AWS secret in checksummed non-raw file is rejected" {
        $bundle = New-TestBundle "nonraw-json-aws-secret"
        $readmePath = Join-Path $bundle "README.md"
        [System.IO.File]::AppendAllText(
            $readmePath,
            '`n{"AWS_SECRET_ACCESS_KEY":"abcdefghijklmnopqrstuvwxyz1234567890ABCD"}`n',
            $utf8NoBom
        )
        Update-Checksum -Bundle $bundle -RelativePath "README.md"
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Possible aws_secret secret pattern in README.md"
    }

    Invoke-Test "generic token assignment in checksummed non-raw file is rejected" {
        $bundle = New-TestBundle "nonraw-generic-token"
        $readmePath = Join-Path $bundle "README.md"
        [System.IO.File]::AppendAllText(
            $readmePath,
            "`nTOKEN=HALOFPX_GENERIC_TOKEN_FIXTURE_123456`n",
            $utf8NoBom
        )
        Update-Checksum -Bundle $bundle -RelativePath "README.md"
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Possible generic_secret_assignment secret pattern in README.md"
    }

    Invoke-Test "invalid UTF-8 in checksummed non-raw file is rejected" {
        $bundle = New-TestBundle "invalid-utf8"
        $relative = "collect-read-only.ps1"
        [System.IO.File]::WriteAllBytes((Join-Path $bundle $relative), [byte[]] @(0x66, 0x6f, 0x80, 0x6f))
        Update-Checksum -Bundle $bundle -RelativePath $relative
        Assert-ValidationFails -Bundle $bundle -ExpectedMessage "Portable artifact is not strict UTF-8 text: $relative"
    }

    Write-Output "PASS: $script:passed incident validator contract tests; SKIP: $script:skipped"
} finally {
    $resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
    $resolvedTempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
    $safePrefix = $resolvedTempRoot.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    if ($resolvedTestRoot.StartsWith($safePrefix, [System.StringComparison]::OrdinalIgnoreCase) -and
        ([System.IO.Path]::GetFileName($resolvedTestRoot)).StartsWith(
            "halofpx-oom-incident-validator-tests-",
            [System.StringComparison]::Ordinal
        )) {
        Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
