# pearlctf 2025 lost thoughts writeup

# PearlCTF2025 : Lost_Thoughts

## Challenge Overview

In this challenge, we are provided with a memory dump of a machine. Our first task is to analyze the dump to determine which operating system it belongs to.

## Step 1: Analyzing the Memory Dump

To identify the operating system, we can use `volatility3`:

```bash
python3 ~/volatility3/vol.py -f dump.mem windows.info
```

If the output contains references to Windows structures (e.g., `Windows NT`, `Registry Hives`), then it confirms that the dump is from a Windows machine.

## Step 2: Identifying Notepad-Related Files

First, we analyze the memory dump to find relevant files. We use `volatility3` to scan for file paths related to Notepad.

```bash
python3 ~/volatility3/vol.py -f dump.mem windows.filescan > output.txt
```

Next, we search for Notepad-related files:

```bash
grep -F "\Local\Packages\Microsoft.WindowsNotepad" output.txt
```

Output:

```
0x8e0dec47f7f0 \Users\vboxuser\AppData\Local\Packages\Microsoft.WindowsNotepad_8wekyb3d8bbwe\LocalState\WindowState\b3505914-9f4a-457a-ad98-ab47d1fba35d.0.bin
0x8e0dec48c630 \Users\vboxuser\AppData\Local\Packages\Microsoft.WindowsNotepad_8wekyb3d8bbwe\Settings\settings.dat
0x8e0dee681360 \Users\vboxuser\AppData\Local\Packages\Microsoft.WindowsNotepad_8wekyb3d8bbwe\LocalState\TabState\9342b2f6-59b2-40fc-973f-a6961d1f5471.bin.tmp
```

We identify that the flag is stored in the `TabState` directory.

## Step 3: Extracting the TabState File

Using `volatility3`, we extract the file using its virtual address:

```bash
python3 ~/volatility3/vol.py -f dump.mem -o ./ windows.dumpfiles --virtaddr 0x8e0dee681360
```

Output:

```
Volatility 3 Framework 2.23.0
Progress:  100.00    PDB scanning finished
Cache    FileObject    FileName    Result

DataSectionObject    0x8e0dee681360    9342b2f6-59b2-40fc-973f-a6961d1f5471.bin.tmp    file.0x8e0dee681360.0x8e0de4a88960.DataSectionObject.9342b2f6-59b2-40fc-973f-a6961d1f5471.bin.tmp.dat
```

## Step 4: Reading and Decoding the Flag

Now, we list the extracted files:

```bash
ls
```

Output:

```
dump.mem
file.0x8e0dee681360.0x8e0de4a88960.DataSectionObject.9342b2-40fc-973f-a6961d1f5471.bin.tmp.dat
output.txt
```

We check the contents of the extracted file:

```bash
cat file.0x8e0dee681360.0x8e0de4a88960.DataSectionObject.9342b2-40fc-973f-a6961d1f5471.bin.tmp.dat
```

Output:

```
NP:::cGVhcmx7YzBuZ3I0dHVsNHRpMG41X3kwdV9nMHRfdGgzX20zc3M0ZzN9==
```

We decode the Base64-encoded content:

```bash
base64 -d <<< cGVhcmx7YzBuZ3I0dHVsNHRpMG41X3kwdV9nMHRfdGgzX20zc3M0ZzN9==
```

Output:

```
pearl{c0ngr4tul4ti0n5_y0u_g0t_th3_m3ss4g3}
```

## Conclusion

The extracted flag is:

```
pearl{c0ngr4tul4ti0n5_y0u_g0t_th3_m3ss4g3}
```

By analyzing the Windows memory dump with `volatility3`, we successfully retrieved the flag hidden in Notepad's `TabState` file.
