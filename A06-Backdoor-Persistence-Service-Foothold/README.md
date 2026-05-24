# Backdoor / Persistence / Service Foothold Class

### Control Samples

|ID|Technique|Core Meaning|
|---|---|---|
|**A06_1a**|Registry Run-Key / Startup Persistence|Directly configures automatic execution during user logon or startup|
|**A06_1b**|Registry Persistence via Hive Save/Restore|Modifies offline registry hives to establish persistence indirectly|
|**A06_2**|Service-Based Persistence|Registers as a background Windows service for durable execution|
|**A06_3**|Scheduled Task Persistence|Uses scheduler triggers for recurring or event-driven execution|
|**A06_4**|WMI Event Subscription Persistence|Uses permanent WMI filters and consumers for stealthy event-triggered execution|
|**A06_5**|Persistent Backdoor / Command Loop|Maintains remote-access capability after persistence is established|
