using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace A03_4_SafeObfuscatedLoader
{
    class Program
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr VirtualAlloc(
            IntPtr lpAddress,
            UIntPtr dwSize,
            uint flAllocationType,
            uint flProtect
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateThread(
            IntPtr lpThreadAttributes,
            UIntPtr dwStackSize,
            IntPtr lpStartAddress,
            IntPtr lpParameter,
            uint dwCreationFlags,
            out uint lpThreadId
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        const uint MEM_COMMIT = 0x1000;
        const uint MEM_RESERVE = 0x2000;
        const uint PAGE_READWRITE = 0x04;
        const uint INFINITE = 0xFFFFFFFF;

        static byte[] XorTransform(byte[] data, byte key)
        {
            byte[] output = new byte[data.Length];
            for (int i = 0; i < data.Length; i++)
            {
                output[i] = (byte)(data[i] ^ key);
            }
            return output;
        }

        // Benign thread routine target
        static uint BenignWorker(IntPtr lpParameter)
        {
            Console.WriteLine("[*] Benign worker thread started.");
            Thread.Sleep(1000);
            Console.WriteLine("[*] Benign worker thread exiting.");
            return 0;
        }

        delegate uint ThreadProc(IntPtr lpParameter);

        static void Main(string[] args)
        {
            byte xorKey = 0x5A;

            // Hardcoded obfuscated benign blob
            string embeddedBlob = "GR8UHhUWGxwVHwkfHBMJ"; // placeholder encoded data
            byte[] encoded;

            try
            {
                encoded = Convert.FromBase64String(embeddedBlob);
            }
            catch
            {
                // fallback so the sample still runs if you haven't generated a final blob yet
                encoded = Convert.ToBase64String(
                    XorTransform(Encoding.UTF8.GetBytes("CONTROL_SAMPLE"), xorKey)
                ) is string tmp
                    ? Convert.FromBase64String(tmp)
                    : Array.Empty<byte>();
            }

            byte[] decoded = XorTransform(encoded, xorKey);

            Console.WriteLine("[*] Decoded blob length: {0}", decoded.Length);

            IntPtr mem = VirtualAlloc(
                IntPtr.Zero,
                (UIntPtr)decoded.Length,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE
            );

            if (mem == IntPtr.Zero)
            {
                Console.WriteLine("[-] VirtualAlloc failed.");
                return;
            }

            Marshal.Copy(decoded, 0, mem, decoded.Length);
            Console.WriteLine("[*] Decoded blob copied into allocated memory at 0x{0:X}.", mem.ToInt64());

            // Safe handoff: create a thread, but do NOT start it at the decoded buffer.
            ThreadProc proc = new ThreadProc(BenignWorker);
            IntPtr procPtr = Marshal.GetFunctionPointerForDelegate(proc);

            uint threadId;
            IntPtr hThread = CreateThread(
                IntPtr.Zero,
                UIntPtr.Zero,
                procPtr,
                IntPtr.Zero,
                0,
                out threadId
            );

            if (hThread == IntPtr.Zero)
            {
                Console.WriteLine("[-] CreateThread failed.");
                return;
            }

            Console.WriteLine("[*] Thread created successfully (safe benign start routine).");
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);

            Console.WriteLine("[*] Safe specimen complete. Decoded memory was staged but not executed.");
        }
    }
}