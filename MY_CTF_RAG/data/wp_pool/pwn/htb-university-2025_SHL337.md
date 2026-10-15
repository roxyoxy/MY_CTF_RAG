# htb university 2025 SHL337

![](../../assets/banner.png)



<img src="../../assets/htb.png" style="margin-left: 20px; zoom: 80%;" align=left />    	    <font size="6">SHL337</font>

​		4<sup>th</sup> Sept 2025 

​		Prepared By: w3th4nds

​		Challenge Author(s): w3th4nds

​		Difficulty: <font color=green>Very Easy</font>

​		Classification: Official

 





# Synopsis

SHL337 is an easy difficulty challenge that features writing a one-line assembly code to shift left some bytes on a certain register with the correct value.

# Description

The mischievous elves have tampered with Nibbletop’s registers—most notably the EBX register—and now he’s stuck, unable to continue delivering Christmas gifts. Can you step in, restore his register, and save Christmas once again for everyone?

## Skills Required

- Basic C and assembly.

## Skills Learned

- Overwrite registers with assembly.

# Enumeration

First of all, we start with a `checksec`:  

```console
pwndbg> checksec
File:     /home/w3t/github/Backup-challs/cyber-santa-2025/SHL337/challenge/shl33t
Arch:     amd64
RELRO:      Full RELRO
Stack:      Canary found
NX:         NX enabled
PIE:        PIE enabled
SHSTK:      Enabled
IBT:        Enabled
Stripped:   No
```

### Protections 🛡️

As we can see:

| Protection | Enabled  | Usage   |
| :---:      | :---:    | :---:   |
| **Canary** | ✅      | Prevents **Buffer Overflows**  |
| **NX**     | ✅       | Disables **code execution** on stack |
| **PIE**    | ✅      | Randomizes the **base address** of the binary |
| **RelRO**  | **Full** | Makes some binary sections **read-only** |

The program's interface 

![](assets/inter.png)

The program is straight forward and tells us what we need to do:

Make `EBX` register have the value `0x13370000` instead of `0x00001337`. 

We see that `EBX` contains `0x00001337`. We can simply shift left the last bytes to the upper ones, with the `shl` command.

```python
r.send(asm('shl ebx, 16; ret'))
```

We use `16` because that's bits and not bytes!

