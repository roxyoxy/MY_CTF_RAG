# pearlctf-2025_malz

# Analysis of Malz

## Initial Setup for Dynamic Analysis

For this challenge, I received a UPX-packed, stripped Windows executable and a prompt file explaining the flag format. To safely analyze the malware, I set up:

- A FlareVM instance connected to a host-only network adapter
- A REMnux VM on the same network segment
- Wireshark for network traffic analysis


I disabled Windows Defender to prevent interference with the malware's execution.

## Initial Binary Examination

First, I examined the binary to confirm its properties
: The file was UPX-packed as expected, so I unpacked it to facilitate basic static analysis:

```
$ upx -d malz.exe
```

## Basic Static Analysis

After unpacking, I examined the binary for interesting strings and imports:

1. The binary had relatively few readable strings due to encryption/obfuscation
2. Notable imported functions included:
   - Network-related APIs (WSAStartup, gethostbyname)
   - Process enumeration functions (CreateToolhelp32Snapshot)
   - Registry manipulation functions (RegOpenKeyEx, RegSetValueEx)
   - File operations (CreateFile, WriteFile)

I also noticed the binary contained anti-analysis checks, including functions like IsDebuggerPresent and various timing checks.

## Anti-Analysis Detection & Bypass

Initial execution attempts resulted in no observable activity. I realized the malware was implementing various anti-analysis techniques:

1. Process name detection (checking for analysis tools like debuggers)
2. VM detection (checking system resources)
3. Timing-based anti-debugging checks

To bypass these protections, I:
- Configured the VM with multiple CPU cores and sufficient RAM
- Renamed any potentially flagged monitoring processes
- Used less intrusive monitoring tools

## Dynamic Analysis

### Network Activity

Once anti-VM checks were bypassed, I ran the malware with Wireshark capturing all traffic:

1. The malware immediately attempted a DNS resolution for "pearlctf.in"
2. Since this domain didn't resolve to a real IP in my isolated environment, the malware appeared to perform certain activity(Discussed later.)

To simulate DNS resolution, I configured the REMnux VM with INetSim and configured FlareVM to direct all DNS Queries to REMnux VM.

It created a file thankyou.txt and stopped execution in case pearlctf.in was reachable. 
[Note from author: This design choice was intentional from the challenge author as a safety measure in case any CTF participants accidentally executed the malware on their host machines instead of in a controlled analysis environment. By creating a harmless text file rather than performing more malicious actions when the domain was reachable, the risk of actual damage was minimized. :) ]

### Process Manipulation

Using Procwatch, I observed:


1. It specifically targeted and terminated instances of "Taskmgr.exe" and "TaskManager.exe"
2. This activity confirmed the "killprocess" behavior for the flag
3. Launched lots of processes.
### File System Activity

Monitoring file system activity revealed:

1. The malware created a copy of itself in a different location (confirming "replicate" behavior)
2. It also created a separate data file with different content, "thankyou.txt" (confirming "filecreate" behavior)

### Registry Modifications

Comparing registry state before and after execution:

1. The malware added entries to "SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System"
3. These modifications confirmed the "regedit" behavior for the flag

## Constructing the Flag

Based on the behaviors observed during dynamic analysis, and following the flag format described in the prompt, I identified these behaviors:

1. DNS queries to "pearlctf.in"
2. File creation (different from self-replication)
3. Process termination (Task Manager)
4. Binary was UPX packed
5. Registry modifications
6. Self-replication
7. Launching other processes.(Self replicates and run it's own code.)

