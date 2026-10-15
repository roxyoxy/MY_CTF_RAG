# forensics predators pixels predators pixels

# Predator's Pixels


## Steps to Solve

### Step 1: Listing Running Processes
Participants begin by identifying running processes within the memory dump using Volatility’s `pslist` plugin:
```bash
python3 ~/volatility3/vol.py -f dump.mem windows.pslist
```
This command lists all active processes, helping to identify potential targets for further investigation.

### Step 2: Extracting Memory Map for a Process
After identifying an interesting process (e.g., PID 7668), participants extract its memory map and dump its contents:
```bash
python3 ~/volatility3/vol.py -f dump.mem windows.memmap --pid 7668 --dump
```
This produces a dumped memory file, `pid.7668.dmp`, which could contain valuable forensic artifacts.

### Step 3: Carving Files from Memory Dump
To identify embedded files, participants use `binwalk` to analyze and extract data from the memory dump:
```bash
binwalk -e pid.7668.dmp
```
`binwalk` scans for recognizable file signatures, revealing the presence of a PNG image:
```
3237808       0x3167B0        PNG image, 2444 x 518, 8-bit/color RGBA, non-interlaced
```

### Step 4: Recovering the Image
Using `dd`, participants extract the PNG file from the memory dump based on the offset and size identified:
```bash
dd if=pid.7668.dmp of=recovered1.jpg bs=1 skip=0x254000 count=1140224
```
This reconstructs the hidden image, which contains the flag.
