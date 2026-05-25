using System;
using System.Management;
using Microsoft.Win32; // Required for native registry management

namespace A06_4_wmi_persistence
{
    class Program
    {
        // Define WMI Paths and Names matching your matrix layout
        private const string EventFilterName = "A06_4_RegistryFilter";
        private const string ConsumerName = "A06_4_RegistryConsumer";
        private const string WqlQuery = @"SELECT * FROM RegistryValueChangeEvent WHERE Hive='HKEY_LOCAL_MACHINE' AND KeyPath='SOFTWARE\\A06_4_Test' AND ValueName='Trigger'";
        private const string CommandLinePayload = @"cmd.exe /c echo A06_4_TRIGGERED > C:\Users\Public\A06_4_marker.txt";

        static void Main(string[] args)
        {
            bool uninstall = false;

            // Check arguments for a cleanup flag
            if (args.Length > 0 && (args[0].Equals("/cleanup", StringComparison.OrdinalIgnoreCase) || args[0].Equals("-u", StringComparison.OrdinalIgnoreCase)))
            {
                uninstall = true;
            }

            try
            {
#pragma warning disable CA1416
                // WMI Event classes live inside the root\subscription namespace on modern OS
                ManagementScope scope = new ManagementScope(@"\\.\root\subscription");
                scope.Connect();
#pragma warning restore CA1416

                if (uninstall)
                {
                    ExecuteCleanup(scope);
                }
                else
                {
                    ExecuteInstallation(scope);
                }
            }
            catch (ManagementException mex)
            {
                Console.WriteLine($"[-] WMI Management Error: {mex.Message} (Error Code: {mex.ErrorCode})");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Unexpected Error: {ex.Message}");
            }
        }

        private static void ExecuteInstallation(ManagementScope scope)
        {
            Console.WriteLine("[*] Initializing WMI Registration Flow...");

            // 1. Natively create the registry test path and initialize the watched property string
            try
            {
#pragma warning disable CA1416
                using (RegistryKey key = Registry.LocalMachine.CreateSubKey(@"SOFTWARE\A06_4_Test", true))
                {
                    if (key != null)
                    {
                        Console.WriteLine("[+] Verified/Created target test registry path: HKLM\\SOFTWARE\\A06_4_Test");

                        // Seed the property so the WQL engine knows the target string variant exists
                        key.SetValue("Trigger", "INITIAL", RegistryValueKind.String);
                        Console.WriteLine("[+] Initialized target monitoring property 'Trigger' to value 'INITIAL'.");
                    }
                }
#pragma warning restore CA1416
            }
            catch (UnauthorizedAccessException)
            {
                Console.WriteLine("[-] Access Denied while creating registry key. Ensure you are running as Administrator.");
                return;
            }

#pragma warning disable CA1416
            // 2. Create the Event Filter
            ManagementClass filterClass = new ManagementClass(scope, new ManagementPath("__EventFilter"), null);
            ManagementObject filterObject = filterClass.CreateInstance();
            filterObject["Name"] = EventFilterName;
            filterObject["QueryLanguage"] = "WQL";
            filterObject["Query"] = WqlQuery;
            filterObject["EventNamespace"] = @"root\cimv2"; // Registry events are processed in root\cimv2
            filterObject.Put();
            Console.WriteLine("[+] Created __EventFilter successfully.");

            // 3. Create the CommandLineEventConsumer
            ManagementClass consumerClass = new ManagementClass(scope, new ManagementPath("CommandLineEventConsumer"), null);
            ManagementObject consumerObject = consumerClass.CreateInstance();
            consumerObject["Name"] = ConsumerName;
            consumerObject["CommandLineTemplate"] = CommandLinePayload;
            consumerObject.Put();
            Console.WriteLine("[+] Created CommandLineEventConsumer successfully.");

            // 4. Create the FilterToConsumerBinding
            ManagementClass bindingClass = new ManagementClass(scope, new ManagementPath("__FilterToConsumerBinding"), null);
            ManagementObject bindingObject = bindingClass.CreateInstance();
            bindingObject["Filter"] = filterObject.Path.RelativePath;
            bindingObject["Consumer"] = consumerObject.Path.RelativePath;
            bindingObject.Put();
            Console.WriteLine("[+] Created __FilterToConsumerBinding successfully.");
#pragma warning restore CA1416

            Console.WriteLine("\n[+] Verification Info: Installation Complete.");

            // Automated Trigger Integration
            Console.WriteLine("[*] Automatically executing validation trigger modification...");
            try
            {
#pragma warning disable CA1416
                // The '!' at the end tells the compiler you guarantee this will not be null
                using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\A06_4_Test", true)!)
                {
                    if (key != null)
                    {
                        // Modify the value to fire the RegistryValueChangeEvent WMI trigger
                        key.SetValue("Trigger", "A06_4_FIRE", RegistryValueKind.String);

                        // Flush forces physical serialization to disk immediately
                        key.Flush();
                        Console.WriteLine("[+] Fired trigger: HKLM\\SOFTWARE\\A06_4_Test -> Trigger = 'A06_4_FIRE'");
                    }
                }
#pragma warning restore CA1416
                Console.WriteLine("[*] Check C:\\Users\\Public\\A06_4_marker.txt to verify WmiPrvSE execution success.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Failed to execute automated registry trigger: {ex.Message}");
            }
        }

#pragma warning disable CA1416
        private static void ExecuteCleanup(ManagementScope scope)
        {
            Console.WriteLine("[*] Initializing WMI Cleanup Flow...");

            // Fix 1: Use ObjectQuery searcher pattern to find and remove bindings robustly
            try
            {
                string bindingQuery = $"SELECT * FROM __FilterToConsumerBinding WHERE Filter LIKE '%{EventFilterName}%' AND Consumer LIKE '%{ConsumerName}%'";
                using (ManagementObjectSearcher searcher = new ManagementObjectSearcher(scope, new ObjectQuery(bindingQuery)))
                {
                    int deletedBindingsCount = 0;
                    foreach (ManagementObject binding in searcher.Get())
                    {
                        binding.Delete();
                        deletedBindingsCount++;
                    }
                    Console.WriteLine($"[+] Removed {deletedBindingsCount} active __FilterToConsumerBinding instance(s).");
                }
            }
            catch (ManagementException)
            {
                Console.WriteLine("[-] Error or no matching binding instances discovered during query scanning.");
            }

            // Relational object paths for remaining direct target deletions
            string filterPath = $"__EventFilter.Name='{EventFilterName}'";
            string consumerPath = $"CommandLineEventConsumer.Name='{ConsumerName}'";

            // 2. Delete Event Filter
            try
            {
                using (ManagementObject filter = new ManagementObject(scope, new ManagementPath(filterPath), null))
                {
                    filter.Delete();
                    Console.WriteLine("[+] Successfully removed __EventFilter.");
                }
            }
            catch (ManagementException) { Console.WriteLine("[-] Event Filter not found or already deleted."); }

            // 3. Delete Consumer
            try
            {
                using (ManagementObject consumer = new ManagementObject(scope, new ManagementPath(consumerPath), null))
                {
                    consumer.Delete();
                    Console.WriteLine("[+] Successfully removed CommandLineEventConsumer.");
                }
            }
            catch (ManagementException) { Console.WriteLine("[-] Event Consumer not found or already deleted."); }

            // Fix 1b: Resolved missing closing brace syntax error on registry clean block
            try
            {
                Registry.LocalMachine.DeleteSubKeyTree(@"SOFTWARE\A06_4_Test", false);
                Console.WriteLine("[+] Cleaned up target test registry tree.");
            }
            catch (Exception)
            {
                /* Fail silently if key wasn't there */
            }

            Console.WriteLine("[+] Cleanup completed.");
        }
#pragma warning restore CA1416
    }
}