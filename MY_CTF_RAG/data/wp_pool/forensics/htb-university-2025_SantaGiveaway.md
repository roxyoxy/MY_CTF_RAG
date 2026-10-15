# htb university 2025 SantaGiveaway

<img src="../../assets/banner.png" style="zoom: 80%;" align=center />

<img src="../../assets/htb.png" style="zoom: 80%;" align='left' /><font size="5">Santa Giveaway</font>

  4<sup>th</sup> 12 2025

  Prepared By: 0xSh3rl0ck

  Challenge Author: 0xSh3rl0ck

  Difficulty: <font color=orange>Medium</font>

  Classification: Official




# Synopsis

A medium-difficulty memory forensics challenge: analyze a memory dump to uncover how an Amadey malware variant infiltrated the system after the user was tricked into running a fake “giveaway installer,” ultimately leading to full system compromise.

## Description

- At Wintercrest Workshop, an employee ran a cheerful holiday giveaway helper that left behind a shimmerdust‑thin trail no one noticed at first. That single action set off a quiet compromise beneath the system’s surface. You are provided only with a full memory dump. Reconstruct the incident using volatile artifacts: identify the process that began the chain, uncover the in‑memory traces it left behind, and extract the command line showing how the intruder secured its foothold.

## Skills Required

- Intermediate knowledge of memory forensics tools and common Volatility plugins.
- Ability to identify malicious processes and loaders in a memory dump.
- Understanding of how persistence mechanisms can be detected through volatile artifacts.

## Skills Learned

- Reading and interpreting process structures, command-line arguments, and memory regions from a memory dump.
- Extracting in‑memory artifacts associated with malicious loaders.
- Detecting persistence techniques based solely on memory‑resident data.

# Enumeration

Players are provided with the following file.

- **Challenge-Snapshot.vmem**: `a full memory dump captured at the moment of compromise.`
- **Challenge-Snapshot.vmsn**: `the VMware snapshot metadata file containing VM state information.`

# Solution

### [1/9] During initial compromise, What is the full URL including the installer filename that the victim downloaded to start the compromise?

We can search the cached files using the `windows.filescan` Plugin, and inside find the Chrome History file:

```
2217:0xe786d47c1ef0     \Users\user\AppData\Local\Google\Chrome\User Data\Default\History
```

The resulting file though, which is supposed to be an `sqlite` database, is corrupted:

```bash
$> file file.0xe786d47c1ef0.0xe786d37d7010.DataSectionObject.History.dat
file.0xe786d47c1ef0.0xe786d37d7010.DataSectionObject.History.dat: SQLite 3.x database, last written using SQLite version 3049002, file counter 26, database pages 75, 1st free page 61, free pages 32, cookie 0x21, schema 4, UTF-8, version-valid-for 26

$> sqlite3 file.0xe786d47c1ef0.0xe786d37d7010.DataSectionObject.History.dat 
SQLite version 3.51.1 2025-11-28 17:28:25
Enter ".help" for usage hints.
sqlite> .tables
Error: database disk image is malformed
sqlite>
```

Fortunately we can recover some of it using the following `sqlite3` commands:

```
$> sqlite3 file.0xe786d47c1ef0.0xe786d37d7010.DataSectionObject.History.dat ".recover" > recovered.sql
$> sqlite3 History.sqlite < recovered.sql
```

The first one will create an `SQL` file with the commands to create a database, while the latter executes them.

Afterwards, we can examine the database:

```bash
$> sqlite3 History.sqlite

SQLite version 3.51.1 2025-11-28 17:28:25
Enter ".help" for usage hints.
sqlite> .tables
cluster_keywords          downloads                 segment_usage
cluster_visit_duplicates  downloads_slices          segments
clusters                  downloads_url_chains      urls
clusters_and_visits       history_sync_metadata     visit_source
content_annotations       keyword_search_terms      visited_links
context_annotations       meta                      visits
sqlite>
```

Examining the `urls` and `downloads` tables we find that the user has visited:

```bash
sqlite> select * from urls;
[...]
68|http://graveyard.htb:8000/|Directory listing for /|1|1|13408297641744166|0
[...]
```

And downloaded:

```bash
sqlite> select * from downloads;
57|9bd2a31e-622e-4fdb-95b0-fd87eba0c807|C:\Users\user\Downloads\DiscordGiveaway.exe|C:\Users\user\Downloads\DiscordGiveaway.exe|13408297643715783|294401|294401|1|6|0||13408297662294387|0|0|0|http://graveyard.htb:8000/||
^H
|http://graveyard.htb:8000/||||||||application/x-msdownload|application/x-msdos-program
```

Answer: `http://graveyard.htb:8000`

### [2/9] What date(time) did the user initiated the download? (UTC Time: YYYY-MM-DD hh:mm:ss)

If we describe the `downloads` table we can see there exists a `start_time` column that represents `microseconds` from Windows Epoch. We can covert it using:

```bash
sqlite> SELECT datetime((start_time / 1000000) - 11644473600, 'unixepoch'), current_path from downloads;
[...]
2025-11-22 15:07:23|C:\Users\user\Downloads\DiscordGiveaway.exe
```

Answer: `2025-11-22 15:07:23`

### [3/9] Which process became the primary malicious executable? Provide the PID associated with this process.

We can use the **`windows.pslist.PsList`** plugin to see which processes are active in memory and check if there is anything unusual.

![image.png](assets/image%208.png)

As we can see, there is an abnormal process named `rgbux.exe` running in memory. Let’s validate whether it is malicious by dumping it and submitting it to an online sandbox.

We can do this by specifying the PID of the process along with the dump option to extract the process.

![image.png](assets/image%209.png)

![image.png](assets/image%2010.png)

As we can see, that is our malicious process.
`Answer: 6520`

### [4/9] Based on memory analysis, what is the location the malware was originally launched from?

We can start by viewing the full command line arguments of the process from the memory dump using the **`windows.cmdline.CmdLine`** plugin. We can also specify the PID of the process, which is `6520`, to avoid including information from other processes.

![image.png](assets/image%2011.png)

As we can see, this shows the full path from which the process is running.

`Answer: C:\Users\user\AppData\Local\Temp\152c6d54a1\rgbux.exe`

### [5/9] When reviewing network activity, which remote IP address and port did the malicious process communicate with?

We can use **`windows.netscan.NetScan`**, which is used to extract network connections from a Windows memory image by scanning memory structures directly.

![image.png](assets/image%2012.png)

As we can see, this reveals the IP and port that the malicious software is communicating with.

`Answer: 89.58.51.107:80`

### [6/9] What is the Malware Family

From earlier, we can see that VirusTotal has identified the malware family

![](./assets/vt.png)

`Answer: amadey`

### [7/9] What is the GUID of the scheduled task created by the malware (HKLM\...)

Since it’s related to programs that run at a specified date and time, we can check the Scheduled Tasks registry keys to see if anything malicious is present. For scheduled task keys, we have several locations to review from them:

`HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache\Tree`

- Shows tasks in human‑readable hierarchy (best place to check for malicious tasks).

`HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache\Tasks`

- Contains task definitions with GUIDs.

We can use the **`windows.registry.printkey.PrintKey`** plugin to read a specific registry hive key from memory, but first we will use **`windows.registry.hivelist.HiveList`** plugin to show the offsets where each hive was found.

![image.png](assets/image%2013.png)

Then we can take the SOFTWARE hive offset and pass it to **`windows.registry.printkey.PrintKey`** along with our registry key location to see if anything suspicious exists.

![image.png](assets/image%2014.png)

As we can see, there is a task named **`rgbux.exe`**, matching the name of the malicious process that was running in memory. So let’s check the `Tasks` key to verify that and obtain the full path of the registry entry.

![image.png](assets/image%2015.png)

As we can see, there are a lot of tasks with their corresponding GUIDs. So let’s use the **`--recurse`** option to recurse through all keys, and since we already know what we are looking for, we can use **`grep`** to filter this process.

![image.png](assets/image%2016.png)

As we can see, the full registry key used by the malware is: `SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache\Tasks\{81A3950E-EE73-4DB9-B670-DF3979056B48}`

`Answer: 81A3950E-EE73-4DB9-B670-DF3979056B48`


### [8/9] As part of maintaining a long‑term foothold, **What full registry key did the malware create to ensure it launches at every user session startup** (HXCU\...)

As the question asks about startup execution when a user logs in, we can consider the following registry keys commonly used to reference items that run at session start.

- `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders`
- `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders`

So let’s go back to the **`windows.registry.hivelist.HiveList`** plugin to get the offset of the user registry hive.

![image.png](assets/image%2018.png)

Then we can use **`windows.registry.printkey.PrintKey`** again with the offset and the path of the Shell Folders key.

![image.png](assets/image%2019.png)

As we can see, there is nothing related to this process, and the output looks normal. So let’s continue with the other path.

![image.png](assets/image%2020.png)

As we can see, the malware changed the default **Startup** value name to its directory location to ensure persistence.

`Answer: HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders\Startup`

### [9/9] When reviewing the process token, how many privileges were enabled by default for the malicious process?

First, what is a process token? It is a security object that represents the security context of a process. It contains information about:

- The user account under which the process runs
- The groups the user belongs to
- Privileges granted to the process
- Access rights for system resources

Privileges are special permissions within the token that allow a process to perform sensitive operations. Examples include:

- `SeDebugPrivilege` → Allows debugging other processes

So we can use `windows.privileges.Privs`, which can show us this information by specifying our malicious PID, and we can also filter based on what the question asks—privileges that are enabled by default.

![image.png](assets/image%2021.png)

We can see that there are **3 Windows privileges** that the process `rgbux.exe` (PID `6520`) has enabled by default.

`Answer: 3`
