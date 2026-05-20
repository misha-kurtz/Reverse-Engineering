using System;
using Microsoft.Win32;

namespace A06_1a_registry_run_persist
{
    public class Program
    {
        public static void Main()
        {
            try
            {
                // Retrieve the MachineGuid to simulate realistic autorun value names
                string deviceId = DeviceInfo.GetUUID();

                // Controlled persistence payload:
                // Marker executable that writes a proof artifact after logon
                string markerPath =
                    @"C:\Users\Public\A06_1a_persistence_marker.exe";

                // Build properly quoted command line
                string finalCommand =
                    $"\"{markerPath}\"";

                Console.WriteLine(
                    $"[*] Target Payload Path configured: {finalCommand}");

                // Establish persistence under current user context
                RegistryHelper.SetRunKey(deviceId, finalCommand);

                Console.WriteLine(
                    $"[+] Persistence successfully configured under value name: {deviceId}");

                Console.WriteLine(
                    "[*] Ready for analysis. Log off/on or reboot to trigger persistence.");
            }
            catch (Exception ex)
            {
                Console.WriteLine(
                    $"[-] An error occurred: {ex.Message}");
            }
        }
    }

    public static class DeviceInfo
    {
        public static string GetUUID()
        {
            using (var baseKey =
                RegistryKey.OpenBaseKey(
                    RegistryHive.LocalMachine,
                    RegistryView.Registry64))

            using (var subKey =
                baseKey.OpenSubKey(
                    @"SOFTWARE\Microsoft\Cryptography"))
            {
                if (subKey != null)
                {
                    string? machineGuid =
                        subKey.GetValue("MachineGuid") as string;

                    if (!string.IsNullOrWhiteSpace(machineGuid))
                    {
                        return new Guid(
                            machineGuid.Trim())
                            .ToString("N");
                    }
                }
            }

            return Guid.Empty.ToString("N");
        }
    }

    public static class RegistryHelper
    {
        private const string RunKeyPath =
            @"Software\Microsoft\Windows\CurrentVersion\Run";

        public static void SetRunKey(
            string name,
            string command)
        {
            using (var baseKey =
                RegistryKey.OpenBaseKey(
                    RegistryHive.CurrentUser,
                    RegistryView.Registry64))

            using (var subKey =
                baseKey.OpenSubKey(
                    RunKeyPath,
                    true))
            {
                if (subKey != null)
                {
                    subKey.SetValue(name, command);
                }
                else
                {
                    throw new InvalidOperationException(
                        "Unable to open the specified Registry key path.");
                }
            }
        }
    }
}