[CmdletBinding()]
param(
    [string] $OutputRoot = (Join-Path $PSScriptRoot "raw"),
    [switch] $Force
)

$ErrorActionPreference = "Stop"

function Invoke-CapturedCommand {
    param(
        [Parameter(Mandatory)] [string] $Name,
        [Parameter(Mandatory)] [string] $FilePath,
        [Parameter(Mandatory)] [string[]] $ArgumentList,
        [int[]] $AllowedExitCodes = @(0)
    )

    $stdoutPath = Join-Path $OutputRoot ("{0}.stdout.log" -f $Name)
    $stderrPath = Join-Path $OutputRoot ("{0}.stderr.log" -f $Name)
    $process = Start-Process -FilePath $FilePath `
        -ArgumentList $ArgumentList `
        -NoNewWindow `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $stdoutPath `
        -RedirectStandardError $stderrPath

    if ($process.ExitCode -notin $AllowedExitCodes) {
        throw "Read-only capture '$Name' failed with exit code $($process.ExitCode)."
    }
}

if ((Test-Path -LiteralPath $OutputRoot) -and
    (Get-ChildItem -LiteralPath $OutputRoot -Force | Select-Object -First 1) -and
    -not $Force) {
    throw "OutputRoot is not empty. Use a new directory, or pass -Force only for an intentional replacement capture."
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

$sshOptions = @("-o", "BatchMode=yes", "-o", "ConnectTimeout=10")

Invoke-CapturedCommand -Name "nimo-2-kernel-oom-window" -FilePath "ssh" -ArgumentList ($sshOptions + @(
    "nimo-2",
    "journalctl -k --since '2026-08-12 19:10:05.500000' --until '2026-08-12 19:10:15.629000' --no-pager -o short-iso-precise"
))

Invoke-CapturedCommand -Name "nimo-2-worker-restart-window" -FilePath "ssh" -ArgumentList ($sshOptions + @(
    "nimo-2",
    "journalctl -u minimax-m27-rpc-worker.service --since '2026-08-12 19:08:00' --until '2026-08-12 19:18:00' --no-pager -o short-iso-precise"
))

Invoke-CapturedCommand -Name "nimo-1-coordinator-restart-window" -FilePath "ssh" -ArgumentList ($sshOptions + @(
    "nimo-1",
    "journalctl -u minimax-m27-q6-server.service --since '2026-08-12 19:08:00' --until '2026-08-12 19:18:00' --no-pager -o short-iso-precise"
))

Invoke-CapturedCommand -Name "nimo-2-current-authority" -FilePath "ssh" -ArgumentList ($sshOptions + @(
    "nimo-2",
    "free -k; cat /proc/meminfo; systemctl show minimax-m27-rpc-worker.service -p Id -p ActiveState -p SubState -p MainPID -p InvocationID -p NRestarts -p Result -p ExecMainCode -p ExecMainStatus -p ControlGroup"
))

Invoke-CapturedCommand -Name "nimo-1-current-authority" -FilePath "ssh" -ArgumentList ($sshOptions + @(
    "nimo-1",
    "free -k; cat /proc/meminfo; systemctl show minimax-m27-q6-server.service -p Id -p ActiveState -p SubState -p MainPID -p InvocationID -p NRestarts -p Result -p ExecMainCode -p ExecMainStatus -p ControlGroup; curl -fsS http://127.0.0.1:8081/health"
))

Invoke-CapturedCommand -Name "github-issue-41" -FilePath "gh" -ArgumentList @(
    "issue",
    "view",
    "41",
    "--repo",
    "JCFrags/HaloFPX",
    "--json",
    "number,title,state,url,body,labels,comments"
)

Get-ChildItem -LiteralPath $OutputRoot -File |
    Sort-Object Name |
    ForEach-Object {
        $digest = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
        "{0}  raw/{1}" -f $digest, $_.Name
    }
