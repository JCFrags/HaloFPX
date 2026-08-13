[CmdletBinding()]
param(
    [Parameter(ParameterSetName = "Collect", Mandatory, Position = 0)]
    [ValidateNotNullOrEmpty()]
    [string] $OutputRoot,

    [Parameter(ParameterSetName = "Collect", Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string] $KnownHostsFile,

    [Parameter(ParameterSetName = "Collect", Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string] $SshConfigFile,

    [Parameter(ParameterSetName = "Collect")]
    [ValidateRange(1, 600)]
    [int] $CommandTimeoutSeconds = 60,

    [Parameter(ParameterSetName = "Collect")]
    [ValidateRange(1024, 67108864)]
    [int] $MaximumOutputBytes = 16777216,

    [Parameter(ParameterSetName = "SelfTest", Mandatory)]
    [switch] $SelfTest,

    [Parameter(ParameterSetName = "SelfTest")]
    [ValidateRange(1, 30)]
    [int] $SelfTestTimeoutSeconds = 1,

    [Parameter(ParameterSetName = "SelfTest")]
    [ValidateRange(1024, 67108864)]
    [int] $SelfTestMaximumOutputBytes = 1048576
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $IsWindows) {
    throw "This evidence collector requires a Windows control host so every command can be contained in a kill-on-close Job Object."
}

$bundleRoot = [System.IO.Path]::GetFullPath($PSScriptRoot)
$selfTestRoot = $null
if ($SelfTest) {
    $selfTestRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("halofpx-incident-collector-selftest-{0}" -f [guid]::NewGuid().ToString("N"))
    $OutputRoot = Join-Path $selfTestRoot "capture"
    $KnownHostsFile = Join-Path $selfTestRoot "known_hosts"
    $SshConfigFile = Join-Path $selfTestRoot "ssh_config"
    $CommandTimeoutSeconds = $SelfTestTimeoutSeconds
    $MaximumOutputBytes = $SelfTestMaximumOutputBytes
    New-Item -ItemType Directory -Path $selfTestRoot | Out-Null
    [System.IO.File]::WriteAllText($KnownHostsFile, "self-test-only", [System.Text.UTF8Encoding]::new($false))
    [System.IO.File]::WriteAllText($SshConfigFile, "Host nimo-*`n", [System.Text.UTF8Encoding]::new($false))
}

if (-not [System.IO.Path]::IsPathFullyQualified($OutputRoot)) {
    throw "OutputRoot must be a fully qualified path."
}
$resolvedOutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
$bundlePrefix = $bundleRoot.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar

if ($resolvedOutputRoot.Equals($bundleRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
    $resolvedOutputRoot.StartsWith($bundlePrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must be outside the immutable incident bundle: $bundleRoot"
}

if (Test-Path -LiteralPath $resolvedOutputRoot) {
    throw "OutputRoot must be a new path that does not already exist: $resolvedOutputRoot"
}

if (-not [System.IO.Path]::IsPathFullyQualified($KnownHostsFile)) {
    throw "KnownHostsFile must be a fully qualified path."
}
$resolvedKnownHostsFile = [System.IO.Path]::GetFullPath($KnownHostsFile)
if (-not (Test-Path -LiteralPath $resolvedKnownHostsFile -PathType Leaf)) {
    throw "KnownHostsFile does not exist: $resolvedKnownHostsFile"
}
$knownHostsItem = Get-Item -LiteralPath $resolvedKnownHostsFile -Force
if ($knownHostsItem.Length -eq 0) {
    throw "KnownHostsFile must be nonempty: $resolvedKnownHostsFile"
}

if (-not [System.IO.Path]::IsPathFullyQualified($SshConfigFile)) {
    throw "SshConfigFile must be a fully qualified path."
}
$resolvedSshConfigFile = [System.IO.Path]::GetFullPath($SshConfigFile)
if (-not (Test-Path -LiteralPath $resolvedSshConfigFile -PathType Leaf)) {
    throw "SshConfigFile does not exist: $resolvedSshConfigFile"
}
$sshConfigItem = Get-Item -LiteralPath $resolvedSshConfigFile -Force
if ($sshConfigItem.Length -eq 0) {
    throw "SshConfigFile must be nonempty: $resolvedSshConfigFile"
}

function Assert-NoReparsePoint {
    param([Parameter(Mandatory)] [string] $Path, [Parameter(Mandatory)] [string] $Context)

    $cursor = [System.IO.Path]::GetFullPath($Path)
    while ($cursor) {
        if (Test-Path -LiteralPath $cursor) {
            $item = Get-Item -LiteralPath $cursor -Force
            if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "$Context traverses a reparse point: $cursor"
            }
        }
        $parent = [System.IO.Directory]::GetParent($cursor)
        if ($null -eq $parent) { break }
        $cursor = $parent.FullName
    }
}

$outputParent = [System.IO.Directory]::GetParent($resolvedOutputRoot)
if ($null -eq $outputParent -or -not (Test-Path -LiteralPath $outputParent.FullName -PathType Container)) {
    throw "OutputRoot parent directory must already exist: $resolvedOutputRoot"
}
Assert-NoReparsePoint -Path $outputParent.FullName -Context "OutputRoot"
Assert-NoReparsePoint -Path $resolvedKnownHostsFile -Context "KnownHostsFile"
Assert-NoReparsePoint -Path $resolvedSshConfigFile -Context "SshConfigFile"

$pwshExecutable = $null
$sshExecutable = $null
$ghExecutable = $null
if ($SelfTest) {
    $pwshExecutable = @(Get-Command pwsh -CommandType Application -ErrorAction Stop)[0].Source
} else {
    $sshExecutable = @(Get-Command ssh -CommandType Application -ErrorAction Stop)[0].Source
    $ghExecutable = @(Get-Command gh -CommandType Application -ErrorAction Stop)[0].Source
}

if (-not ("HaloFpx.IncidentJob" -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace HaloFpx
{
    public sealed class NativeCommandResult
    {
        public bool JobCreated { get; internal set; }
        public bool JobKillOnCloseConfigured { get; internal set; }
        public bool BreakawayDisabled { get; internal set; }
        public bool RestrictedHandleListConfigured { get; internal set; }
        public bool AtomicJobListConfigured { get; internal set; }
        public bool JobMembershipVerified { get; internal set; }
        public bool PrimaryThreadResumed { get; internal set; }
        public bool Started { get; internal set; }
        public int? ProcessId { get; internal set; }
        public uint? ExitCode { get; internal set; }
        public bool RootHandleReaped { get; internal set; }
        public bool RootExitedBeforeDeadline { get; internal set; }
        public bool TimedOut { get; internal set; }
        public bool OutputLimitExceeded { get; internal set; }
        public bool TreeTerminationRequested { get; internal set; }
        public bool TreeTerminationApiSucceeded { get; internal set; }
        public bool? TreeCleanupProven { get; internal set; }
        public uint? ActiveProcessesFinal { get; internal set; }
        public uint TotalProcessesObserved { get; internal set; }
        public long StdoutObservedBytes { get; internal set; }
        public long StderrObservedBytes { get; internal set; }
        public long StdoutStoredBytes { get; internal set; }
        public long StderrStoredBytes { get; internal set; }
        public bool OutputTruncatedToLimit { get; internal set; }
        public bool OutputArtifactsWithinLimit { get; internal set; }
        public string FailureStage { get; internal set; }
        public string Error { get; internal set; }
        public int? NativeErrorCode { get; internal set; }
        public string CleanupError { get; internal set; }
        public bool JobHandleClosed { get; internal set; }
        public long DurationMilliseconds { get; internal set; }
    }

    public static class IncidentJob
    {
        private const uint JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x00002000;
        private const uint STARTF_USESTDHANDLES = 0x00000100;
        private const uint CREATE_SUSPENDED = 0x00000004;
        private const uint CREATE_UNICODE_ENVIRONMENT = 0x00000400;
        private const uint EXTENDED_STARTUPINFO_PRESENT = 0x00080000;
        private const uint CREATE_NO_WINDOW = 0x08000000;
        private const uint GENERIC_READ = 0x80000000;
        private const uint GENERIC_WRITE = 0x40000000;
        private const uint FILE_SHARE_READ = 0x00000001;
        private const uint FILE_SHARE_WRITE = 0x00000002;
        private const uint FILE_SHARE_DELETE = 0x00000004;
        private const uint CREATE_NEW = 1;
        private const uint OPEN_EXISTING = 3;
        private const uint FILE_ATTRIBUTE_NORMAL = 0x00000080;
        private const uint WAIT_OBJECT_0 = 0;
        private const uint WAIT_TIMEOUT = 258;
        private const uint WAIT_FAILED = 0xFFFFFFFF;
        private const uint TERMINATION_EXIT_CODE = 1460;
        private const uint PROC_THREAD_ATTRIBUTE_HANDLE_LIST = 0x00020002;
        private const uint PROC_THREAD_ATTRIBUTE_JOB_LIST = 0x0002000D;
        private const int JobObjectBasicAccountingInformation = 1;
        private const int JobObjectExtendedLimitInformation = 9;
        private static readonly IntPtr InvalidHandleValue = new IntPtr(-1);

        [StructLayout(LayoutKind.Sequential)]
        private struct SECURITY_ATTRIBUTES
        {
            public int nLength;
            public IntPtr lpSecurityDescriptor;
            [MarshalAs(UnmanagedType.Bool)]
            public bool bInheritHandle;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct STARTUPINFO
        {
            public int cb;
            public IntPtr lpReserved;
            public IntPtr lpDesktop;
            public IntPtr lpTitle;
            public uint dwX;
            public uint dwY;
            public uint dwXSize;
            public uint dwYSize;
            public uint dwXCountChars;
            public uint dwYCountChars;
            public uint dwFillAttribute;
            public uint dwFlags;
            public ushort wShowWindow;
            public ushort cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct STARTUPINFOEX
        {
            public STARTUPINFO StartupInfo;
            public IntPtr lpAttributeList;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PROCESS_INFORMATION
        {
            public IntPtr hProcess;
            public IntPtr hThread;
            public uint dwProcessId;
            public uint dwThreadId;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct IO_COUNTERS
        {
            public ulong ReadOperationCount;
            public ulong WriteOperationCount;
            public ulong OtherOperationCount;
            public ulong ReadTransferCount;
            public ulong WriteTransferCount;
            public ulong OtherTransferCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct JOBOBJECT_BASIC_LIMIT_INFORMATION
        {
            public long PerProcessUserTimeLimit;
            public long PerJobUserTimeLimit;
            public uint LimitFlags;
            public UIntPtr MinimumWorkingSetSize;
            public UIntPtr MaximumWorkingSetSize;
            public uint ActiveProcessLimit;
            public UIntPtr Affinity;
            public uint PriorityClass;
            public uint SchedulingClass;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct JOBOBJECT_EXTENDED_LIMIT_INFORMATION
        {
            public JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
            public IO_COUNTERS IoInfo;
            public UIntPtr ProcessMemoryLimit;
            public UIntPtr JobMemoryLimit;
            public UIntPtr PeakProcessMemoryUsed;
            public UIntPtr PeakJobMemoryUsed;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct JOBOBJECT_BASIC_ACCOUNTING_INFORMATION
        {
            public long TotalUserTime;
            public long TotalKernelTime;
            public long ThisPeriodTotalUserTime;
            public long ThisPeriodTotalKernelTime;
            public uint TotalPageFaultCount;
            public uint TotalProcesses;
            public uint ActiveProcesses;
            public uint TotalTerminatedProcesses;
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr CreateJobObject(IntPtr jobAttributes, string name);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr CreateFileW(
            string fileName,
            uint desiredAccess,
            uint shareMode,
            ref SECURITY_ATTRIBUTES securityAttributes,
            uint creationDisposition,
            uint flagsAndAttributes,
            IntPtr templateFile);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool InitializeProcThreadAttributeList(
            IntPtr attributeList,
            int attributeCount,
            int flags,
            ref UIntPtr size);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool UpdateProcThreadAttribute(
            IntPtr attributeList,
            uint flags,
            IntPtr attribute,
            IntPtr value,
            IntPtr size,
            IntPtr previousValue,
            IntPtr returnSize);

        [DllImport("kernel32.dll")]
        private static extern void DeleteProcThreadAttributeList(IntPtr attributeList);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CreateProcessW(
            string applicationName,
            StringBuilder commandLine,
            IntPtr processAttributes,
            IntPtr threadAttributes,
            [MarshalAs(UnmanagedType.Bool)] bool inheritHandles,
            uint creationFlags,
            IntPtr environment,
            string currentDirectory,
            ref STARTUPINFOEX startupInfo,
            out PROCESS_INFORMATION processInformation);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetInformationJobObject(
            IntPtr job,
            int informationClass,
            ref JOBOBJECT_EXTENDED_LIMIT_INFORMATION information,
            uint informationLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool IsProcessInJob(
            IntPtr process,
            IntPtr job,
            [MarshalAs(UnmanagedType.Bool)] out bool result);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool TerminateJobObject(IntPtr job, uint exitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool QueryInformationJobObject(
            IntPtr job,
            int informationClass,
            out JOBOBJECT_BASIC_ACCOUNTING_INFORMATION information,
            uint informationLength,
            IntPtr returnLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint ResumeThread(IntPtr thread);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint WaitForSingleObject(IntPtr handle, uint milliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetExitCodeProcess(IntPtr process, out uint exitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool TerminateProcess(IntPtr process, uint exitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CloseHandle(IntPtr handle);

        private static void ThrowLastError(string operation)
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), operation + " failed.");
        }

        private static string QuoteWindowsArgument(string argument)
        {
            if (argument == null)
            {
                throw new ArgumentNullException("argument");
            }

            bool requiresQuotes = argument.Length == 0;
            for (int i = 0; i < argument.Length && !requiresQuotes; i++)
            {
                requiresQuotes = char.IsWhiteSpace(argument[i]) || argument[i] == '"';
            }
            if (!requiresQuotes)
            {
                return argument;
            }

            var quoted = new StringBuilder();
            quoted.Append('"');
            int backslashes = 0;
            foreach (char character in argument)
            {
                if (character == '\\')
                {
                    backslashes++;
                    continue;
                }
                if (character == '"')
                {
                    quoted.Append('\\', (backslashes * 2) + 1);
                    quoted.Append('"');
                    backslashes = 0;
                    continue;
                }
                quoted.Append('\\', backslashes);
                backslashes = 0;
                quoted.Append(character);
            }
            quoted.Append('\\', backslashes * 2);
            quoted.Append('"');
            return quoted.ToString();
        }

        private static StringBuilder BuildCommandLine(string applicationName, string[] arguments)
        {
            var commandLine = new StringBuilder(QuoteWindowsArgument(applicationName));
            foreach (string argument in arguments)
            {
                commandLine.Append(' ');
                commandLine.Append(QuoteWindowsArgument(argument));
            }
            return commandLine;
        }

        private static void CloseNoThrow(ref IntPtr handle, NativeCommandResult result, string description)
        {
            if (handle == IntPtr.Zero || handle == InvalidHandleValue)
            {
                handle = IntPtr.Zero;
                return;
            }
            IntPtr closing = handle;
            handle = IntPtr.Zero;
            if (!CloseHandle(closing))
            {
                AppendCleanupError(result, "CloseHandle(" + description + ") failed: " +
                    new Win32Exception(Marshal.GetLastWin32Error()).Message);
            }
        }

        private static void AppendCleanupError(NativeCommandResult result, string message)
        {
            if (String.IsNullOrEmpty(result.CleanupError))
            {
                result.CleanupError = message;
            }
            else
            {
                result.CleanupError += " | " + message;
            }
        }

        private static void RecordFailure(NativeCommandResult result, string stage, Exception exception)
        {
            if (!String.IsNullOrEmpty(result.Error))
            {
                AppendCleanupError(result, stage + ": " + exception.Message);
                return;
            }
            result.FailureStage = stage;
            result.Error = exception.Message;
            var native = exception as Win32Exception;
            if (native != null)
            {
                result.NativeErrorCode = native.NativeErrorCode;
            }
        }

        private static JOBOBJECT_BASIC_ACCOUNTING_INFORMATION QueryAccounting(IntPtr job)
        {
            JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting;
            if (!QueryInformationJobObject(
                job,
                JobObjectBasicAccountingInformation,
                out accounting,
                (uint)Marshal.SizeOf<JOBOBJECT_BASIC_ACCOUNTING_INFORMATION>(),
                IntPtr.Zero))
            {
                ThrowLastError("QueryInformationJobObject");
            }
            return accounting;
        }

        private static void UpdateAccounting(NativeCommandResult result, JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting)
        {
            result.ActiveProcessesFinal = accounting.ActiveProcesses;
            if (accounting.TotalProcesses > result.TotalProcessesObserved)
            {
                result.TotalProcessesObserved = accounting.TotalProcesses;
            }
        }

        private static bool TryReapRoot(
            ref IntPtr process,
            NativeCommandResult result,
            bool beforeDeadline)
        {
            if (process == IntPtr.Zero)
            {
                return result.RootHandleReaped;
            }
            uint waitResult = WaitForSingleObject(process, 0);
            if (waitResult == WAIT_TIMEOUT)
            {
                return false;
            }
            if (waitResult == WAIT_FAILED)
            {
                ThrowLastError("WaitForSingleObject(process)");
            }
            if (waitResult != WAIT_OBJECT_0)
            {
                throw new InvalidOperationException("Unexpected process wait result: " + waitResult + ".");
            }

            uint exitCode;
            if (!GetExitCodeProcess(process, out exitCode))
            {
                ThrowLastError("GetExitCodeProcess");
            }
            result.ExitCode = exitCode;
            if (!CloseHandle(process))
            {
                ThrowLastError("CloseHandle(process)");
            }
            process = IntPtr.Zero;
            result.RootHandleReaped = true;
            if (beforeDeadline)
            {
                result.RootExitedBeforeDeadline = true;
            }
            return true;
        }

        private static void UpdateOutputSizes(
            NativeCommandResult result,
            string stdoutPath,
            string stderrPath,
            long maximumOutputBytes)
        {
            long stdoutLength = File.Exists(stdoutPath) ? new FileInfo(stdoutPath).Length : 0;
            long stderrLength = File.Exists(stderrPath) ? new FileInfo(stderrPath).Length : 0;
            if (stdoutLength > result.StdoutObservedBytes)
            {
                result.StdoutObservedBytes = stdoutLength;
            }
            if (stderrLength > result.StderrObservedBytes)
            {
                result.StderrObservedBytes = stderrLength;
            }
            if (stdoutLength > maximumOutputBytes || stderrLength > maximumOutputBytes)
            {
                result.OutputLimitExceeded = true;
            }
        }

        private static void TerminateAndDrain(
            NativeCommandResult result,
            IntPtr job,
            ref IntPtr process,
            int cleanupTimeoutMilliseconds)
        {
            result.TreeTerminationRequested = true;
            if (job != IntPtr.Zero)
            {
                try
                {
                    if (!TerminateJobObject(job, TERMINATION_EXIT_CODE))
                    {
                        ThrowLastError("TerminateJobObject");
                    }
                    result.TreeTerminationApiSucceeded = true;
                }
                catch (Exception exception)
                {
                    AppendCleanupError(result, exception.Message);
                    result.TreeCleanupProven = false;
                    return;
                }
            }
            else if (process != IntPtr.Zero)
            {
                try
                {
                    if (!TerminateProcess(process, TERMINATION_EXIT_CODE))
                    {
                        ThrowLastError("TerminateProcess");
                    }
                    result.TreeTerminationApiSucceeded = true;
                }
                catch (Exception exception)
                {
                    AppendCleanupError(result, exception.Message);
                    result.TreeCleanupProven = false;
                    return;
                }
            }

            var cleanupWatch = Stopwatch.StartNew();
            while (true)
            {
                try
                {
                    TryReapRoot(ref process, result, false);
                    if (job != IntPtr.Zero)
                    {
                        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting = QueryAccounting(job);
                        UpdateAccounting(result, accounting);
                        if (accounting.ActiveProcesses == 0)
                        {
                            result.TreeCleanupProven = result.JobMembershipVerified ||
                                (!result.PrimaryThreadResumed && result.RootHandleReaped);
                            return;
                        }
                    }
                    else if (result.RootHandleReaped)
                    {
                        result.ActiveProcessesFinal = 0;
                        result.TreeCleanupProven = !result.PrimaryThreadResumed;
                        return;
                    }
                }
                catch (Exception exception)
                {
                    AppendCleanupError(result, exception.Message);
                    result.TreeCleanupProven = false;
                    return;
                }

                if (cleanupWatch.ElapsedMilliseconds >= cleanupTimeoutMilliseconds)
                {
                    result.TreeCleanupProven = false;
                    return;
                }
                int remaining = cleanupTimeoutMilliseconds - (int)cleanupWatch.ElapsedMilliseconds;
                Thread.Sleep(Math.Min(20, Math.Max(1, remaining)));
            }
        }

        private static void TruncateIfRequired(
            NativeCommandResult result,
            string path,
            long maximumOutputBytes)
        {
            if (!File.Exists(path))
            {
                return;
            }
            long length = new FileInfo(path).Length;
            if (length <= maximumOutputBytes)
            {
                return;
            }
            using (var stream = new FileStream(
                path,
                FileMode.Open,
                FileAccess.Write,
                FileShare.Read | FileShare.Write | FileShare.Delete))
            {
                stream.SetLength(maximumOutputBytes);
                stream.Flush(true);
            }
            result.OutputTruncatedToLimit = true;
        }

        public static NativeCommandResult Run(
            string applicationName,
            string[] arguments,
            string workingDirectory,
            string stdoutPath,
            string stderrPath,
            int timeoutMilliseconds,
            long maximumOutputBytes,
            int cleanupTimeoutMilliseconds)
        {
            var result = new NativeCommandResult();
            var stopwatch = Stopwatch.StartNew();
            IntPtr job = IntPtr.Zero;
            IntPtr standardInput = IntPtr.Zero;
            IntPtr standardOutput = IntPtr.Zero;
            IntPtr standardError = IntPtr.Zero;
            IntPtr attributeList = IntPtr.Zero;
            IntPtr inheritedHandleList = IntPtr.Zero;
            IntPtr jobList = IntPtr.Zero;
            IntPtr process = IntPtr.Zero;
            IntPtr thread = IntPtr.Zero;
            string stage = "argument-validation";

            try
            {
                if (String.IsNullOrWhiteSpace(applicationName) || !Path.IsPathRooted(applicationName))
                {
                    throw new ArgumentException("The executable path must be fully qualified.", "applicationName");
                }
                if (arguments == null)
                {
                    throw new ArgumentNullException("arguments");
                }
                if (String.IsNullOrWhiteSpace(workingDirectory) || !Directory.Exists(workingDirectory))
                {
                    throw new DirectoryNotFoundException("Working directory does not exist: " + workingDirectory);
                }
                if (timeoutMilliseconds < 1 || cleanupTimeoutMilliseconds < 1 || maximumOutputBytes < 1)
                {
                    throw new ArgumentOutOfRangeException("Timeout and output limits must be positive.");
                }

                stage = "job-create";
                job = CreateJobObject(IntPtr.Zero, null);
                if (job == IntPtr.Zero)
                {
                    ThrowLastError("CreateJobObject");
                }
                result.JobCreated = true;

                stage = "job-policy";
                var limits = new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
                limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                if (!SetInformationJobObject(
                    job,
                    JobObjectExtendedLimitInformation,
                    ref limits,
                    (uint)Marshal.SizeOf<JOBOBJECT_EXTENDED_LIMIT_INFORMATION>()))
                {
                    ThrowLastError("SetInformationJobObject");
                }
                result.JobKillOnCloseConfigured = true;
                result.BreakawayDisabled = true;

                var inheritable = new SECURITY_ATTRIBUTES();
                inheritable.nLength = Marshal.SizeOf<SECURITY_ATTRIBUTES>();
                inheritable.bInheritHandle = true;
                uint shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

                stage = "stdout-open";
                standardOutput = CreateFileW(
                    stdoutPath,
                    GENERIC_WRITE,
                    shareMode,
                    ref inheritable,
                    CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL,
                    IntPtr.Zero);
                if (standardOutput == InvalidHandleValue)
                {
                    standardOutput = IntPtr.Zero;
                    ThrowLastError("CreateFileW(stdout)");
                }

                stage = "stderr-open";
                standardError = CreateFileW(
                    stderrPath,
                    GENERIC_WRITE,
                    shareMode,
                    ref inheritable,
                    CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL,
                    IntPtr.Zero);
                if (standardError == InvalidHandleValue)
                {
                    standardError = IntPtr.Zero;
                    ThrowLastError("CreateFileW(stderr)");
                }

                stage = "stdin-open";
                standardInput = CreateFileW(
                    "NUL",
                    GENERIC_READ,
                    shareMode,
                    ref inheritable,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    IntPtr.Zero);
                if (standardInput == InvalidHandleValue)
                {
                    standardInput = IntPtr.Zero;
                    ThrowLastError("CreateFileW(NUL)");
                }

                stage = "attribute-list-size";
                UIntPtr attributeListSize = UIntPtr.Zero;
                InitializeProcThreadAttributeList(IntPtr.Zero, 2, 0, ref attributeListSize);
                if (attributeListSize == UIntPtr.Zero)
                {
                    ThrowLastError("InitializeProcThreadAttributeList(size)");
                }
                attributeList = Marshal.AllocHGlobal(checked((int)attributeListSize.ToUInt64()));

                stage = "attribute-list-init";
                if (!InitializeProcThreadAttributeList(attributeList, 2, 0, ref attributeListSize))
                {
                    ThrowLastError("InitializeProcThreadAttributeList");
                }

                inheritedHandleList = Marshal.AllocHGlobal(IntPtr.Size * 3);
                Marshal.WriteIntPtr(inheritedHandleList, 0, standardInput);
                Marshal.WriteIntPtr(inheritedHandleList, IntPtr.Size, standardOutput);
                Marshal.WriteIntPtr(inheritedHandleList, IntPtr.Size * 2, standardError);

                stage = "restricted-handle-list";
                if (!UpdateProcThreadAttribute(
                    attributeList,
                    0,
                    new IntPtr(PROC_THREAD_ATTRIBUTE_HANDLE_LIST),
                    inheritedHandleList,
                    new IntPtr(IntPtr.Size * 3),
                    IntPtr.Zero,
                    IntPtr.Zero))
                {
                    ThrowLastError("UpdateProcThreadAttribute(HANDLE_LIST)");
                }
                result.RestrictedHandleListConfigured = true;

                jobList = Marshal.AllocHGlobal(IntPtr.Size);
                Marshal.WriteIntPtr(jobList, job);
                stage = "atomic-job-list";
                if (!UpdateProcThreadAttribute(
                    attributeList,
                    0,
                    new IntPtr(PROC_THREAD_ATTRIBUTE_JOB_LIST),
                    jobList,
                    new IntPtr(IntPtr.Size),
                    IntPtr.Zero,
                    IntPtr.Zero))
                {
                    ThrowLastError("UpdateProcThreadAttribute(JOB_LIST)");
                }
                result.AtomicJobListConfigured = true;

                var startupInfo = new STARTUPINFOEX();
                startupInfo.StartupInfo.cb = Marshal.SizeOf<STARTUPINFOEX>();
                startupInfo.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
                startupInfo.StartupInfo.hStdInput = standardInput;
                startupInfo.StartupInfo.hStdOutput = standardOutput;
                startupInfo.StartupInfo.hStdError = standardError;
                startupInfo.lpAttributeList = attributeList;

                stage = "create-process-suspended";
                PROCESS_INFORMATION processInformation;
                uint creationFlags = CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT |
                    EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW;
                if (!CreateProcessW(
                    applicationName,
                    BuildCommandLine(applicationName, arguments),
                    IntPtr.Zero,
                    IntPtr.Zero,
                    true,
                    creationFlags,
                    IntPtr.Zero,
                    workingDirectory,
                    ref startupInfo,
                    out processInformation))
                {
                    ThrowLastError("CreateProcessW");
                }
                process = processInformation.hProcess;
                thread = processInformation.hThread;
                result.Started = true;
                result.ProcessId = checked((int)processInformation.dwProcessId);

                // Only the child retains the three standard handles. Attribute storage
                // is no longer needed after CreateProcessW returns.
                CloseNoThrow(ref standardInput, result, "stdin");
                CloseNoThrow(ref standardOutput, result, "stdout");
                CloseNoThrow(ref standardError, result, "stderr");
                DeleteProcThreadAttributeList(attributeList);
                Marshal.FreeHGlobal(attributeList);
                attributeList = IntPtr.Zero;
                Marshal.FreeHGlobal(inheritedHandleList);
                inheritedHandleList = IntPtr.Zero;
                Marshal.FreeHGlobal(jobList);
                jobList = IntPtr.Zero;

                stage = "job-membership-verification";
                bool inJob;
                if (!IsProcessInJob(process, job, out inJob))
                {
                    ThrowLastError("IsProcessInJob");
                }
                if (!inJob)
                {
                    throw new InvalidOperationException("CreateProcessW returned a process outside the requested Job Object.");
                }
                result.JobMembershipVerified = true;

                stage = "primary-thread-resume";
                if (ResumeThread(thread) == UInt32.MaxValue)
                {
                    ThrowLastError("ResumeThread");
                }
                result.PrimaryThreadResumed = true;
                CloseNoThrow(ref thread, result, "primary-thread");

                stage = "bounded-tree-monitor";
                while (true)
                {
                    bool rootHandleWasOpen = process != IntPtr.Zero;
                    bool rootReaped = TryReapRoot(ref process, result, false);
                    if (rootHandleWasOpen && rootReaped && stopwatch.ElapsedMilliseconds < timeoutMilliseconds)
                    {
                        result.RootExitedBeforeDeadline = true;
                    }
                    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting = QueryAccounting(job);
                    UpdateAccounting(result, accounting);
                    UpdateOutputSizes(result, stdoutPath, stderrPath, maximumOutputBytes);

                    if (result.OutputLimitExceeded)
                    {
                        break;
                    }
                    if (accounting.ActiveProcesses == 0)
                    {
                        if (stopwatch.ElapsedMilliseconds >= timeoutMilliseconds)
                        {
                            result.TimedOut = true;
                            break;
                        }
                        result.TreeCleanupProven = true;
                        break;
                    }
                    if (stopwatch.ElapsedMilliseconds >= timeoutMilliseconds)
                    {
                        result.TimedOut = true;
                        break;
                    }

                    int remaining = timeoutMilliseconds - (int)stopwatch.ElapsedMilliseconds;
                    Thread.Sleep(Math.Min(20, Math.Max(1, remaining)));
                }

                if (result.TimedOut || result.OutputLimitExceeded)
                {
                    stage = result.TimedOut ? "deadline-cleanup" : "output-limit-cleanup";
                    TerminateAndDrain(result, job, ref process, cleanupTimeoutMilliseconds);
                }
            }
            catch (Exception exception)
            {
                RecordFailure(result, stage, exception);
            }
            finally
            {
                if (attributeList != IntPtr.Zero)
                {
                    DeleteProcThreadAttributeList(attributeList);
                    Marshal.FreeHGlobal(attributeList);
                    attributeList = IntPtr.Zero;
                }
                if (inheritedHandleList != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(inheritedHandleList);
                    inheritedHandleList = IntPtr.Zero;
                }
                if (jobList != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(jobList);
                    jobList = IntPtr.Zero;
                }
                CloseNoThrow(ref standardInput, result, "stdin");
                CloseNoThrow(ref standardOutput, result, "stdout");
                CloseNoThrow(ref standardError, result, "stderr");
                CloseNoThrow(ref thread, result, "primary-thread");

                if (result.Started && result.TreeCleanupProven != true && !result.TreeTerminationRequested)
                {
                    TerminateAndDrain(result, job, ref process, cleanupTimeoutMilliseconds);
                }

                if (job != IntPtr.Zero)
                {
                    IntPtr closingJob = job;
                    job = IntPtr.Zero;
                    if (CloseHandle(closingJob))
                    {
                        result.JobHandleClosed = true;
                    }
                    else
                    {
                        AppendCleanupError(result, "CloseHandle(job) failed: " +
                            new Win32Exception(Marshal.GetLastWin32Error()).Message);
                    }
                }

                // KILL_ON_JOB_CLOSE is the final fail-safe. It is not used as cleanup
                // proof because accounting can no longer be queried after this point.
                if (process != IntPtr.Zero)
                {
                    try
                    {
                        uint waitResult = WaitForSingleObject(process, (uint)cleanupTimeoutMilliseconds);
                        if (waitResult == WAIT_OBJECT_0)
                        {
                            TryReapRoot(ref process, result, false);
                        }
                        else if (waitResult == WAIT_FAILED)
                        {
                            ThrowLastError("WaitForSingleObject(process after job close)");
                        }
                    }
                    catch (Exception exception)
                    {
                        AppendCleanupError(result, exception.Message);
                    }
                    CloseNoThrow(ref process, result, "process-final");
                }

                try
                {
                    UpdateOutputSizes(result, stdoutPath, stderrPath, maximumOutputBytes);
                    if (result.OutputLimitExceeded && result.TreeCleanupProven == true)
                    {
                        TruncateIfRequired(result, stdoutPath, maximumOutputBytes);
                        TruncateIfRequired(result, stderrPath, maximumOutputBytes);
                    }
                    result.StdoutStoredBytes = File.Exists(stdoutPath) ? new FileInfo(stdoutPath).Length : 0;
                    result.StderrStoredBytes = File.Exists(stderrPath) ? new FileInfo(stderrPath).Length : 0;
                    result.OutputArtifactsWithinLimit = result.StdoutStoredBytes <= maximumOutputBytes &&
                        result.StderrStoredBytes <= maximumOutputBytes;
                }
                catch (Exception exception)
                {
                    AppendCleanupError(result, "output-finalization: " + exception.Message);
                }

                stopwatch.Stop();
                result.DurationMilliseconds = stopwatch.ElapsedMilliseconds;
            }

            return result;
        }
    }
}
'@
}

New-Item -ItemType Directory -Path $resolvedOutputRoot | Out-Null
$OutputRoot = $resolvedOutputRoot
$ledgerPath = Join-Path $OutputRoot "commands.jsonl"
$script:CommandSequence = 0
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$script:CleanupTimeoutMilliseconds = 5000

function Write-Utf8File {
    param(
        [Parameter(Mandatory)] [string] $Path,
        [AllowEmptyString()] [string] $Text = ""
    )

    [System.IO.File]::WriteAllText($Path, $Text, $script:utf8NoBom)
}

function Write-LedgerRecord {
    param([Parameter(Mandatory)] [System.Collections.IDictionary] $Record)

    $line = ($Record | ConvertTo-Json -Compress -Depth 8) + [Environment]::NewLine
    $bytes = $script:utf8NoBom.GetBytes($line)
    $stream = [System.IO.FileStream]::new(
        $script:ledgerPath,
        [System.IO.FileMode]::Append,
        [System.IO.FileAccess]::Write,
        [System.IO.FileShare]::Read,
        4096,
        [System.IO.FileOptions]::WriteThrough
    )
    try {
        $stream.Write($bytes, 0, $bytes.Length)
        $stream.Flush($true)
    } finally {
        $stream.Dispose()
    }
}

function Get-OutputArtifactMetadata {
    param([Parameter(Mandatory)] [string] $Path)

    $retry = [System.Diagnostics.Stopwatch]::StartNew()
    while ($true) {
        try {
            $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
            $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path -ErrorAction Stop).Hash.ToLowerInvariant()
            return [pscustomobject]@{ Bytes = $item.Length; Sha256 = $hash }
        } catch [System.IO.IOException] {
            if ($retry.ElapsedMilliseconds -ge 2000) { throw }
            Start-Sleep -Milliseconds 20
        }
    }
}

function ConvertTo-PosixSingleQuotedLiteral {
    param([Parameter(Mandatory)] [AllowEmptyString()] [string] $Text)

    if ($Text.IndexOf([char] 0) -ge 0) {
        throw "POSIX command text must not contain NUL."
    }
    if ($Text.IndexOf("`r", [StringComparison]::Ordinal) -ge 0 -or
        $Text.IndexOf("`n", [StringComparison]::Ordinal) -ge 0) {
        throw "POSIX command text must be one line."
    }
    if ($Text.IndexOf('\', [StringComparison]::Ordinal) -ge 0) {
        throw "POSIX command text must not contain backslash because the SSH login shell and /bin/sh quote it differently."
    }

    $singleQuote = [string] [char] 39
    $doubleQuote = [string] [char] 34
    $escapedSingleQuote = [string]::Concat(
        $singleQuote,
        $doubleQuote,
        $singleQuote,
        $doubleQuote,
        $singleQuote
    )
    return [string]::Concat(
        $singleQuote,
        $Text.Replace($singleQuote, $escapedSingleQuote),
        $singleQuote
    )
}

function New-PosixRemoteCommand {
    param([Parameter(Mandatory)] [ValidateNotNullOrEmpty()] [string] $Body)

    # The caller supplies only source-literal, closed-world read-only command
    # bodies.  The absolute shell path and -eu options make their status
    # independent of the SSH account's configured login shell.
    return "exec /bin/sh -eu -c {0}" -f (ConvertTo-PosixSingleQuotedLiteral -Text $Body)
}

function Invoke-CapturedCommand {
    param(
        [Parameter(Mandatory)] [string] $Name,
        [Parameter(Mandatory)] [string] $FilePath,
        [Parameter(Mandatory)] [AllowEmptyCollection()] [AllowEmptyString()] [string[]] $ArgumentList,
        [int[]] $AllowedExitCodes = @(0),
        [ValidateSet("must-be-empty", "record-only")]
        [string] $StderrPolicy = "must-be-empty"
    )

    $script:CommandSequence += 1
    $stdoutName = "{0}.stdout.log" -f $Name
    $stderrName = "{0}.stderr.log" -f $Name
    $stdoutPath = Join-Path $script:OutputRoot $stdoutName
    $stderrPath = Join-Path $script:OutputRoot $stderrName
    $startedAt = [DateTimeOffset]::UtcNow
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $nativeResult = $null
    $failureMessage = $null

    $record = [ordered]@{
        schema                         = "halofpx.read-only-command.v3"
        sequence                       = $script:CommandSequence
        name                           = $Name
        argv                           = @($FilePath) + @($ArgumentList)
        started_at                     = $startedAt.ToString("o")
        ended_at                       = $null
        duration_seconds               = $null
        deadline_seconds               = $script:CommandTimeoutSeconds
        cleanup_grace_seconds          = $script:CleanupTimeoutMilliseconds / 1000
        maximum_output_bytes_per_stream = $script:MaximumOutputBytes
        process_group                  = "windows-job-object"
        started                        = $false
        pid                            = $null
        exit_code                      = $null
        allowed_exit_codes             = @($AllowedExitCodes)
        stderr_policy                  = $StderrPolicy
        job_created                    = $false
        job_kill_on_close_configured   = $false
        breakaway_disabled             = $false
        restricted_handle_list         = $false
        atomic_job_list                = $false
        job_membership_verified        = $false
        primary_thread_resumed         = $false
        timed_out                      = $false
        output_limit_exceeded          = $false
        tree_termination_requested     = $false
        tree_termination_api_succeeded = $false
        parent_reaped                  = $false
        parent_exited_before_deadline  = $false
        tree_cleanup_proven            = $null
        tree_cleanup_basis             = $null
        active_processes_final         = $null
        total_processes_observed       = 0
        job_handle_closed              = $false
        stdout_observed_bytes          = 0
        stderr_observed_bytes          = 0
        stdout_stored_bytes            = 0
        stderr_stored_bytes            = 0
        stdout_sha256                  = $null
        stderr_sha256                  = $null
        output_truncated_to_limit      = $false
        output_artifacts_within_limit  = $false
        failure_class                  = $null
        failure_stage                  = $null
        native_error_code              = $null
        error                          = $null
        cleanup_error                  = $null
        stdout_path                    = $stdoutName
        stderr_path                    = $stderrName
    }

    try {
        if (-not [System.IO.Path]::IsPathFullyQualified($FilePath)) {
            throw "Executable path must be fully qualified: $FilePath"
        }
        $deadlineMilliseconds = [int] ([long] $script:CommandTimeoutSeconds * 1000L)
        $nativeResult = [HaloFpx.IncidentJob]::Run(
            $FilePath,
            [string[]] $ArgumentList,
            $script:bundleRoot,
            $stdoutPath,
            $stderrPath,
            $deadlineMilliseconds,
            [long] $script:MaximumOutputBytes,
            $script:CleanupTimeoutMilliseconds
        )

        $record.started = $nativeResult.Started
        $record.pid = $nativeResult.ProcessId
        $record.exit_code = $nativeResult.ExitCode
        $record.job_created = $nativeResult.JobCreated
        $record.job_kill_on_close_configured = $nativeResult.JobKillOnCloseConfigured
        $record.breakaway_disabled = $nativeResult.BreakawayDisabled
        $record.restricted_handle_list = $nativeResult.RestrictedHandleListConfigured
        $record.atomic_job_list = $nativeResult.AtomicJobListConfigured
        $record.job_membership_verified = $nativeResult.JobMembershipVerified
        $record.primary_thread_resumed = $nativeResult.PrimaryThreadResumed
        $record.timed_out = $nativeResult.TimedOut
        $record.output_limit_exceeded = $nativeResult.OutputLimitExceeded
        $record.tree_termination_requested = $nativeResult.TreeTerminationRequested
        $record.tree_termination_api_succeeded = $nativeResult.TreeTerminationApiSucceeded
        $record.parent_reaped = $nativeResult.RootHandleReaped
        $record.parent_exited_before_deadline = $nativeResult.RootExitedBeforeDeadline
        $record.tree_cleanup_proven = $nativeResult.TreeCleanupProven
        $record.active_processes_final = $nativeResult.ActiveProcessesFinal
        $record.total_processes_observed = $nativeResult.TotalProcessesObserved
        $record.job_handle_closed = $nativeResult.JobHandleClosed
        $record.stdout_observed_bytes = $nativeResult.StdoutObservedBytes
        $record.stderr_observed_bytes = $nativeResult.StderrObservedBytes
        $record.stdout_stored_bytes = $nativeResult.StdoutStoredBytes
        $record.stderr_stored_bytes = $nativeResult.StderrStoredBytes
        $record.output_truncated_to_limit = $nativeResult.OutputTruncatedToLimit
        $record.output_artifacts_within_limit = $nativeResult.OutputArtifactsWithinLimit
        $record.failure_stage = $nativeResult.FailureStage
        $record.native_error_code = $nativeResult.NativeErrorCode
        $record.cleanup_error = $nativeResult.CleanupError

        if ($nativeResult.TreeCleanupProven -eq $true) {
            $record.tree_cleanup_basis = if ($nativeResult.JobMembershipVerified) {
                "atomic-job-membership-and-active-processes-zero"
            } else {
                "suspended-root-reaped-before-user-code"
            }
        } elseif (-not $nativeResult.Started) {
            $record.tree_cleanup_basis = "no-process-created"
        } else {
            $record.tree_cleanup_basis = "unproven"
        }

        if ($nativeResult.Error) {
            $record.failure_class = if (-not $nativeResult.Started) {
                "start-error"
            } elseif (-not $nativeResult.PrimaryThreadResumed) {
                "containment-setup-error"
            } else {
                "controller-error"
            }
            $failureMessage = "Read-only capture '$Name' failed during $($nativeResult.FailureStage): $($nativeResult.Error)"
        } elseif ($nativeResult.OutputLimitExceeded) {
            $record.failure_class = if ($nativeResult.TreeCleanupProven -eq $true) { "output-limit" } else { "output-limit-unreaped" }
            $failureMessage = "Read-only capture '$Name' exceeded its $($script:MaximumOutputBytes)-byte per-stream output limit."
        } elseif ($nativeResult.TimedOut) {
            $record.failure_class = if ($nativeResult.TreeCleanupProven -eq $true) { "timeout" } else { "timeout-unreaped" }
            $failureMessage = "Read-only capture '$Name' exceeded its $($script:CommandTimeoutSeconds)-second deadline."
        } elseif ($nativeResult.CleanupError) {
            $record.failure_class = "controller-error"
            $failureMessage = "Read-only capture '$Name' could not complete controller cleanup: $($nativeResult.CleanupError)"
        } elseif ($nativeResult.TreeCleanupProven -ne $true) {
            $record.failure_class = "cleanup-unproven"
            $failureMessage = "Read-only capture '$Name' ended without proving that its contained process tree reached zero active processes."
        } elseif ($null -eq $nativeResult.ExitCode) {
            $record.failure_class = "controller-error"
            $failureMessage = "Read-only capture '$Name' completed without an observable root exit code."
        } elseif ([long] $nativeResult.ExitCode -notin @($AllowedExitCodes | ForEach-Object { [long] $_ })) {
            $stderrText = [System.IO.File]::ReadAllText($stderrPath, $utf8NoBom)
            $record.failure_class = if ($null -ne $script:sshExecutable -and $FilePath -eq $script:sshExecutable) {
                if ($stderrText -match '(?i)REMOTE HOST IDENTIFICATION HAS CHANGED|host key verification failed|no matching host key') {
                    "ssh-host-key"
                } elseif ($stderrText -match '(?i)permission denied|authentication failed|no supported authentication methods') {
                    "ssh-authentication"
                } elseif ($stderrText -match '(?i)connection (?:timed out|refused|closed)|could not resolve hostname|network is unreachable|no route to host') {
                    "ssh-connection"
                } else {
                    "ssh-command"
                }
            } else {
                "nonzero-exit"
            }
            $failureMessage = "Read-only capture '$Name' failed with exit code $($nativeResult.ExitCode)."
        } elseif ($StderrPolicy -eq "must-be-empty" -and $nativeResult.StderrStoredBytes -ne 0) {
            $record.failure_class = "unexpected-stderr"
            $failureMessage = "Read-only capture '$Name' returned exit code $($nativeResult.ExitCode) but emitted $($nativeResult.StderrStoredBytes) stderr bytes."
        }
    } catch {
        if ($null -eq $record.failure_class) {
            $record.failure_class = "controller-error"
        }
        if ($null -eq $failureMessage) {
            $failureMessage = "Read-only capture '$Name' failed: $($_.Exception.Message)"
        }
        $record.error = $_.Exception.Message
    } finally {
        foreach ($outputPath in @($stdoutPath, $stderrPath)) {
            if (-not (Test-Path -LiteralPath $outputPath)) {
                $emptyOutput = [System.IO.FileStream]::new(
                    $outputPath,
                    [System.IO.FileMode]::CreateNew,
                    [System.IO.FileAccess]::Write,
                    [System.IO.FileShare]::Read
                )
                $emptyOutput.Dispose()
            }
        }
        $stdoutMetadata = Get-OutputArtifactMetadata -Path $stdoutPath
        $stderrMetadata = Get-OutputArtifactMetadata -Path $stderrPath
        $record.stdout_stored_bytes = $stdoutMetadata.Bytes
        $record.stderr_stored_bytes = $stderrMetadata.Bytes
        $record.stdout_sha256 = $stdoutMetadata.Sha256
        $record.stderr_sha256 = $stderrMetadata.Sha256
        $stopwatch.Stop()
        $record.ended_at = [DateTimeOffset]::UtcNow.ToString("o")
        $record.duration_seconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 6)
        if ($failureMessage -and -not $record.error) {
            $record.error = $failureMessage
        }
        Write-LedgerRecord -Record $record
    }

    if ($failureMessage) {
        throw $failureMessage
    }
}

if ($SelfTest) {
    try {
        $argvHelper = Join-Path $selfTestRoot "argv helper.ps1"
        Write-Utf8File -Path $argvHelper -Text @'
[Console]::Out.Write((ConvertTo-Json -Compress -InputObject @($args)))
'@
        $expectedArguments = @(
            "value with spaces",
            "",
            'quote"inside',
            'ends with slash\'
        )
        Invoke-CapturedCommand -Name "argv-success" -FilePath $pwshExecutable -ArgumentList (@(
            "-NoProfile", "-NonInteractive", "-File", $argvHelper
        ) + $expectedArguments)
        $actualArguments = @(
            [System.IO.File]::ReadAllText((Join-Path $OutputRoot "argv-success.stdout.log"), $utf8NoBom) |
                ConvertFrom-Json
        )
        if ($actualArguments.Count -ne $expectedArguments.Count) {
            throw "Self-test argv cardinality mismatch."
        }
        for ($index = 0; $index -lt $expectedArguments.Count; $index += 1) {
            if (-not [string]::Equals($actualArguments[$index], $expectedArguments[$index], [StringComparison]::Ordinal)) {
                throw "Self-test argv mismatch at index $index."
            }
        }

        $multilineRefused = $false
        try {
            New-PosixRemoteCommand -Body "first`nsecond" | Out-Null
        } catch {
            $multilineRefused = $_.Exception.Message.Contains("one line")
        }
        if (-not $multilineRefused) {
            throw "Self-test expected multiline POSIX command refusal."
        }

        $nulRefused = $false
        try {
            New-PosixRemoteCommand -Body ([string]::Concat("first", [char] 0, "second")) | Out-Null
        } catch {
            $nulRefused = $_.Exception.Message.Contains("NUL")
        }
        if (-not $nulRefused) {
            throw "Self-test expected NUL POSIX command refusal."
        }

        $backslashRefused = $false
        try {
            New-PosixRemoteCommand -Body 'printf backslash\fixture' | Out-Null
        } catch {
            $backslashRefused = $_.Exception.Message.Contains("backslash")
        }
        if (-not $backslashRefused) {
            throw "Self-test expected backslash POSIX command refusal."
        }

        $fakeLoginShell = Join-Path $selfTestRoot "fake-fish-login-shell.ps1"
        Write-Utf8File -Path $fakeLoginShell -Text @'
param([Parameter(Mandatory)] [string] $RemoteCommand)

$rawPrefix = "set -eu;"
if ($RemoteCommand.StartsWith($rawPrefix, [StringComparison]::Ordinal)) {
    [Console]::Error.Write("set: invalid option combination")
    [Console]::Out.Write("continued-after-invalid-set")
    exit 0
}

$shellPrefix = "exec /bin/sh -eu -c "
if (-not $RemoteCommand.StartsWith($shellPrefix, [StringComparison]::Ordinal)) {
    [Console]::Error.Write("fake-shell: missing POSIX shell boundary")
    exit 90
}

$literal = $RemoteCommand.Substring($shellPrefix.Length)
$singleQuote = [string] [char] 39
$doubleQuote = [string] [char] 34
$escapedSingleQuote = [string]::Concat(
    $singleQuote,
    $doubleQuote,
    $singleQuote,
    $doubleQuote,
    $singleQuote
)
if ($literal.Length -lt 2 -or
    -not $literal.StartsWith($singleQuote, [StringComparison]::Ordinal) -or
    -not $literal.EndsWith($singleQuote, [StringComparison]::Ordinal)) {
    [Console]::Error.Write("fake-shell: malformed POSIX literal")
    exit 91
}

$inner = $literal.Substring(1, $literal.Length - 2)
$parts = [regex]::Split($inner, [regex]::Escape($escapedSingleQuote))
if (@($parts | Where-Object { $_.Contains($singleQuote) }).Count -ne 0) {
    [Console]::Error.Write("fake-shell: unescaped single quote")
    exit 92
}
$body = $parts -join $singleQuote

if ($body -ceq "exit 23") {
    exit 23
}
if ($body -ceq "printf unexpected >&2") {
    [Console]::Error.Write("unexpected-inner-stderr")
    exit 0
}
[Console]::Out.Write($body)
'@

        Invoke-CapturedCommand -Name "raw-fish-record-only" -FilePath $pwshExecutable -ArgumentList @(
            "-NoProfile", "-NonInteractive", "-File", $fakeLoginShell,
            "set -eu; self-test-read-only"
        ) -StderrPolicy record-only

        $fishDiagnosticRefused = $false
        try {
            Invoke-CapturedCommand -Name "raw-fish-default-shell" -FilePath $pwshExecutable -ArgumentList @(
                "-NoProfile", "-NonInteractive", "-File", $fakeLoginShell,
                "set -eu; self-test-read-only"
            )
        } catch {
            $fishDiagnosticRefused = $_.Exception.Message.Contains("stderr bytes")
        }
        if (-not $fishDiagnosticRefused) {
            throw "Self-test expected fish-style setup diagnostic refusal."
        }

        $quotedBody = 'printf "%s" "owner''s record"'
        $wrappedQuotedBody = New-PosixRemoteCommand -Body $quotedBody
        Invoke-CapturedCommand -Name "posix-wrapper-success" -FilePath $pwshExecutable -ArgumentList @(
            "-NoProfile", "-NonInteractive", "-File", $fakeLoginShell, $wrappedQuotedBody
        ) -StderrPolicy must-be-empty
        $decodedBody = [System.IO.File]::ReadAllText(
            (Join-Path $OutputRoot "posix-wrapper-success.stdout.log"),
            $utf8NoBom
        )
        if (-not [string]::Equals($decodedBody, $quotedBody, [StringComparison]::Ordinal)) {
            throw "Self-test POSIX single-quote round trip mismatch."
        }

        $wrappedNonzeroRefused = $false
        try {
            Invoke-CapturedCommand -Name "posix-wrapper-nonzero" -FilePath $pwshExecutable -ArgumentList @(
                "-NoProfile", "-NonInteractive", "-File", $fakeLoginShell,
                (New-PosixRemoteCommand -Body "exit 23")
            ) -StderrPolicy must-be-empty
        } catch {
            $wrappedNonzeroRefused = $_.Exception.Message.Contains("exit code 23")
        }
        if (-not $wrappedNonzeroRefused) {
            throw "Self-test expected wrapped command nonzero propagation."
        }

        $wrappedStderrRefused = $false
        try {
            Invoke-CapturedCommand -Name "posix-wrapper-stderr" -FilePath $pwshExecutable -ArgumentList @(
                "-NoProfile", "-NonInteractive", "-File", $fakeLoginShell,
                (New-PosixRemoteCommand -Body "printf unexpected >&2")
            ) -StderrPolicy must-be-empty
        } catch {
            $wrappedStderrRefused = $_.Exception.Message.Contains("stderr bytes")
        }
        if (-not $wrappedStderrRefused) {
            throw "Self-test expected wrapped command stderr refusal."
        }

        $nonzeroRefused = $false
        try {
            Invoke-CapturedCommand -Name "nonzero" -FilePath $pwshExecutable -ArgumentList @(
                "-NoProfile", "-NonInteractive", "-Command", "[Console]::Error.Write('expected-nonzero'); exit 7"
            )
        } catch {
            $nonzeroRefused = $_.Exception.Message.Contains("exit code 7")
        }
        if (-not $nonzeroRefused) {
            throw "Self-test expected nonzero exit refusal."
        }

        $timeoutRefused = $false
        try {
            Invoke-CapturedCommand -Name "timeout" -FilePath $pwshExecutable -ArgumentList @(
                "-NoProfile", "-NonInteractive", "-Command", "Start-Sleep -Seconds 30"
            )
        } catch {
            $timeoutRefused = $_.Exception.Message.Contains("deadline")
        }
        if (-not $timeoutRefused) {
            throw "Self-test expected timeout refusal."
        }

        $childHelper = Join-Path $selfTestRoot "child-sleeper.ps1"
        Write-Utf8File -Path $childHelper -Text @'
Start-Sleep -Seconds 30
[Console]::Out.Write("unexpected-child-completion")
'@
        $parentHelper = Join-Path $selfTestRoot "parent-spawns-child.ps1"
        Write-Utf8File -Path $parentHelper -Text @'
param([string] $PwshPath, [string] $ChildScript)
$childArguments = @("-NoProfile", "-NonInteractive", "-File", ('"{0}"' -f $ChildScript))
Start-Process -FilePath $PwshPath -ArgumentList $childArguments -NoNewWindow | Out-Null
[Console]::Out.Write("parent-exited")
'@
        $childTreeRefused = $false
        $childTreeError = $null
        try {
            Invoke-CapturedCommand -Name "inherited-stdout-child" -FilePath $pwshExecutable -ArgumentList @(
                "-NoProfile", "-NonInteractive", "-File", $parentHelper, $pwshExecutable, $childHelper
            )
        } catch {
            $childTreeError = $_.Exception.Message
            $childTreeRefused = $_.Exception.Message.Contains("deadline")
        }
        if (-not $childTreeRefused) {
            throw "Self-test expected inherited-stdout child tree timeout refusal; observed: $childTreeError"
        }

        $startErrorRefused = $false
        $missingExecutable = Join-Path $selfTestRoot "does-not-exist.exe"
        try {
            Invoke-CapturedCommand -Name "start-error" -FilePath $missingExecutable -ArgumentList @()
        } catch {
            $startErrorRefused = $_.Exception.Message.Contains("create-process-suspended")
        }
        if (-not $startErrorRefused) {
            throw "Self-test expected nonexistent executable start refusal."
        }

        $records = @(Get-Content -LiteralPath $ledgerPath | ForEach-Object { $_ | ConvertFrom-Json })
        if ($records.Count -ne 10) { throw "Self-test command ledger cardinality mismatch." }
        if (@($records | Where-Object schema -ne "halofpx.read-only-command.v3").Count -ne 0) {
            throw "Self-test command ledger schema mismatch."
        }
        if ($records[0].failure_class -ne $null -or $records[0].exit_code -ne 0 -or
            $records[0].stderr_policy -ne "must-be-empty" -or
            -not $records[0].parent_reaped -or -not $records[0].tree_cleanup_proven -or
            -not $records[0].atomic_job_list -or -not $records[0].restricted_handle_list -or
            -not $records[0].job_membership_verified -or -not $records[0].output_artifacts_within_limit) {
            throw "Self-test argv-success ledger mismatch."
        }
        if ($records[1].failure_class -ne $null -or $records[1].exit_code -ne 0 -or
            $records[1].stderr_policy -ne "record-only" -or $records[1].stderr_stored_bytes -le 0 -or
            -not [string]::Equals(
                $records[1].argv[-1],
                "set -eu; self-test-read-only",
                [StringComparison]::Ordinal
            ) -or -not ([System.IO.File]::ReadAllText(
                (Join-Path $OutputRoot "raw-fish-record-only.stdout.log"),
                $utf8NoBom
            ).Contains("continued-after-invalid-set"))) {
            throw "Self-test fish-style record-only stderr ledger mismatch."
        }
        if ($records[2].failure_class -ne "unexpected-stderr" -or $records[2].exit_code -ne 0 -or
            $records[2].stderr_policy -ne "must-be-empty" -or $records[2].stderr_stored_bytes -le 0 -or
            -not ([System.IO.File]::ReadAllText(
                (Join-Path $OutputRoot "raw-fish-default-shell.stderr.log"),
                $utf8NoBom
            ).Contains("invalid option combination"))) {
            throw "Self-test fish-style stderr refusal ledger mismatch."
        }
        if ($records[3].failure_class -ne $null -or $records[3].exit_code -ne 0 -or
            $records[3].stderr_policy -ne "must-be-empty" -or $records[3].stderr_stored_bytes -ne 0 -or
            -not [string]::Equals($records[3].argv[-1], $wrappedQuotedBody, [StringComparison]::Ordinal)) {
            throw "Self-test POSIX wrapper success ledger mismatch."
        }
        if ($records[4].failure_class -ne "nonzero-exit" -or $records[4].exit_code -ne 23 -or
            $records[4].stderr_policy -ne "must-be-empty" -or $records[4].stderr_stored_bytes -ne 0) {
            throw "Self-test POSIX wrapper nonzero ledger mismatch."
        }
        if ($records[5].failure_class -ne "unexpected-stderr" -or $records[5].exit_code -ne 0 -or
            $records[5].stderr_policy -ne "must-be-empty" -or $records[5].stderr_stored_bytes -le 0) {
            throw "Self-test POSIX wrapper stderr ledger mismatch."
        }
        if ($records[6].failure_class -ne "nonzero-exit" -or $records[6].exit_code -ne 7) {
            throw "Self-test nonzero ledger mismatch."
        }
        if ($records[7].failure_class -ne "timeout" -or -not $records[7].timed_out -or
            -not $records[7].tree_termination_requested -or -not $records[7].tree_termination_api_succeeded -or
            -not $records[7].parent_reaped -or -not $records[7].tree_cleanup_proven -or
            $records[7].active_processes_final -ne 0) {
            throw "Self-test timeout ledger mismatch."
        }
        if ($records[8].failure_class -ne "timeout" -or -not $records[8].timed_out -or
            -not $records[8].parent_exited_before_deadline -or -not $records[8].tree_cleanup_proven -or
            $records[8].active_processes_final -ne 0 -or $records[8].total_processes_observed -lt 2) {
            throw "Self-test inherited-stdout child ledger mismatch."
        }
        if ($records[9].failure_class -ne "start-error" -or $records[9].started -or
            $null -ne $records[9].pid -or $records[9].tree_cleanup_proven -ne $null -or
            $records[9].failure_stage -ne "create-process-suspended") {
            throw "Self-test start-error ledger mismatch."
        }

        "PASS: atomic Job Object collector argv fidelity, POSIX-shell quoting, cross-shell unsafe-input refusal, fish-style stderr refusal, stderr policy, nonzero propagation, deadline cleanup, inherited-stdout descendant cleanup, start-error ledger, and output bounds"
        return
    } finally {
        if ($selfTestRoot -and (Test-Path -LiteralPath $selfTestRoot)) {
            Remove-Item -LiteralPath $selfTestRoot -Recurse
        }
    }
}

$knownHostsSshPath = $resolvedKnownHostsFile.Replace('\', '/')
$sshConfigSshPath = $resolvedSshConfigFile.Replace('\', '/')
$sshOptions = @(
    "-F", $sshConfigSshPath,
    "-o", "BatchMode=yes",
    "-o", "StrictHostKeyChecking=yes",
    "-o", "UpdateHostKeys=no",
    "-o", "UserKnownHostsFile=$knownHostsSshPath",
    "-o", "GlobalKnownHostsFile=none",
    "-o", "ControlMaster=no",
    "-o", "ControlPath=none",
    "-o", "ClearAllForwardings=yes",
    "-o", "ExitOnForwardFailure=yes",
    "-o", "ForwardAgent=no",
    "-o", "ForwardX11=no",
    "-o", "KnownHostsCommand=none",
    "-o", "PermitLocalCommand=no",
    "-o", "RequestTTY=no",
    "-o", "VerifyHostKeyDNS=no",
    "-o", "ConnectTimeout=10",
    "-o", "ConnectionAttempts=1"
)

Invoke-CapturedCommand -Name "nimo-2-kernel-oom-window" -FilePath $sshExecutable -ArgumentList ($sshOptions + @(
    "nimo-2",
    (New-PosixRemoteCommand -Body "journalctl -k --since '2026-08-12 19:10:05.500000' --until '2026-08-12 19:10:15.629000' --no-pager -o short-iso-precise")
)) -StderrPolicy must-be-empty

Invoke-CapturedCommand -Name "nimo-2-worker-restart-window" -FilePath $sshExecutable -ArgumentList ($sshOptions + @(
    "nimo-2",
    (New-PosixRemoteCommand -Body "journalctl -u minimax-m27-rpc-worker.service --since '2026-08-12 19:08:00' --until '2026-08-12 19:18:00' --no-pager -o short-iso-precise")
)) -StderrPolicy must-be-empty

Invoke-CapturedCommand -Name "nimo-1-coordinator-restart-window" -FilePath $sshExecutable -ArgumentList ($sshOptions + @(
    "nimo-1",
    (New-PosixRemoteCommand -Body "journalctl -u minimax-m27-q6-server.service --since '2026-08-12 19:08:00' --until '2026-08-12 19:18:00' --no-pager -o short-iso-precise")
)) -StderrPolicy must-be-empty

Invoke-CapturedCommand -Name "nimo-2-current-authority" -FilePath $sshExecutable -ArgumentList ($sshOptions + @(
    "nimo-2",
    (New-PosixRemoteCommand -Body "date --iso-8601=ns; free -k; cat /proc/meminfo; systemctl --system show minimax-m27-rpc-worker.service -p Id -p ActiveState -p SubState -p MainPID -p InvocationID -p NRestarts -p Result -p ExecMainCode -p ExecMainStatus -p ControlGroup")
)) -StderrPolicy must-be-empty

Invoke-CapturedCommand -Name "nimo-1-current-authority" -FilePath $sshExecutable -ArgumentList ($sshOptions + @(
    "nimo-1",
    (New-PosixRemoteCommand -Body "date --iso-8601=ns; free -k; cat /proc/meminfo; systemctl --system show minimax-m27-q6-server.service -p Id -p ActiveState -p SubState -p MainPID -p InvocationID -p NRestarts -p Result -p ExecMainCode -p ExecMainStatus -p ControlGroup; curl -fsS http://127.0.0.1:8081/health")
)) -StderrPolicy must-be-empty

Invoke-CapturedCommand -Name "github-issue-41" -FilePath $ghExecutable -ArgumentList @(
    "issue",
    "view",
    "41",
    "--repo",
    "JCFrags/HaloFPX",
    "--json",
    "number,title,state,url,body,labels,comments"
) -StderrPolicy record-only

$checksumLines = Get-ChildItem -LiteralPath $OutputRoot -File |
    Where-Object Name -ne "SHA256SUMS" |
    Sort-Object Name |
    ForEach-Object {
        $digest = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
        "{0}  {1}" -f $digest, $_.Name
    }
$checksumPath = Join-Path $OutputRoot "SHA256SUMS"
Write-Utf8File -Path $checksumPath -Text (($checksumLines -join [Environment]::NewLine) + [Environment]::NewLine)
$checksumLines
