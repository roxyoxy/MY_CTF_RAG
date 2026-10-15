# pearlctf 2025 babymalz

# Analysis of Baby Malz

## Initial Setup for Dynamic Analysis

For analyzing this malware sample, I established a controlled environment with:

- A FlareVM machine connected to a host-only adapter
- A REMnux VM on the same network segment
- Windows Defender disabled on the FlareVM to prevent interference

## Anti-Analysis Detection and Evasion

During initial execution attempts, the malware exhibited no activity. This suggested the presence of anti-analysis mechanisms. Through careful observation and trial-and-error, I identified several anti-VM and anti-debugging techniques implemented in the sample:

- Checks for analysis-related processes (debuggers, monitoring tools)
- Hardware resource verification (CPU count, RAM size)
- Debugger detection via timing discrepancies
- Window title scanning for analysis tools

To bypass these protections, I:
- Modified VM settings to allocate more than 2GB RAM
- Ensured multiple CPU cores were assigned
- Renamed any potentially flagged processes
- Avoided running any analysis tools with detectable window titles

## Network Traffic Analysis

After bypassing the anti-analysis checks, I monitored network traffic using Wireshark and observed:

1. The malware attempted a DNS query for a domain (decrypted from an encrypted string)
2. Since the domain didn't resolve to a real IP address, the malware's execution stopped at this point

## Simulating C2 Infrastructure

To observe the malware's complete behavior, I configured the REMnux VM to respond to DNS queries and HTTP requests:

1. Set up INetSim on REMnux to intercept all network traffic
2. Modified networking on FlareVM to use REMnux as the DNS server:
3. Configured INetSim to respond to all DNS queries and provide simulated HTTP responses

## Observed Behavior

With this setup in place, the malware:


1. Proceeded to make an request to download a secondary payload
2. The download attempt was captured by INetSim.

## Additional Monitoring

To thoroughly analyze the malware's behavior, I also:

- Compared registry state before and after execution (no modifications detected)
- Employed ProcWatch with appropriate filters to track process creation and file activity
- Monitored for file system changes (no secondary file creation or self-replication observed)

## Flag Construction

Based on the observed behavior and according to the prompt instructions, the malware:
- Performs DNS queries (token: `dns_sqwdej.2q3e2.xyz`)
- Attempts to download a secondary payload (token: `download_wqsqq1.com_wqsq.exe`)
- Is UPX-packed (token: packed)

The domains and filename were retrieved by analyzing the network traffic captured during dynamic analysis.
