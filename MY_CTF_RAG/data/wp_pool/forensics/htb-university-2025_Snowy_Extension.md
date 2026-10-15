# htb university 2025 Snowy Extension

![img](../../assets/banner.png)

<img src='../../assets/htb.png' style='zoom: 80%;' align=left /> <font size='10'>Snowy Extension</font>

10<sup>th</sup> December 2025 

Prepared By: `0x157`

Challenge Author(s): `0x157`

Difficulty: <font color=green>Very Easy</font>

<br><br>

# Synopsis

- Snowy Extension is a very easy forensics challenge that involves dissecting a malicious vscode extension by analyzing javascript code.

## Description

- The Starshard Bauble’s theft left behind subtly altered source files hidden among festive scripts. Players must inspect code highlighting anomalies to spot injected logic, trace tampering patterns, and reveal how the Gingerbit Gremlin sabotaged Everlight—one miscolored line at a time.

## Skills Required

- Basic Encoding Knowledge
- Researching Skills

## Skills Learned

- Learn how malicious vscode extensions are structured and executed.
- Understand and implement basic xor-based string decryption.

# Enumeration

Players are provided with the following file:

- `code-highlight-0.0.1.vsix`: which contains a folder inside called **extension**

If you do a quick Google search, you'll see that a `.vsix` file is basically just a zip archive. That means you can simply rename the file, change the extension to `.zip` and then open it like any other zip file to look at what's inside.

<p align="center">
  <img src="assets/basic_enum.png" alt="basic_enum" width="900">
</p>

After unpacking the zip file, we get a handful of files, but the one we really care about is **`extension.js`**. This file is the main code for the extension and is responsible for most of its core functionality.

Looking through the file, a few red flags immediately stand out: suspicious imports, functions with random-looking names, code that executes system commands and logic to send data via HTTP Post request.

<p align="center">
  <img src="assets/func.png" alt="func" width="800">
</p>

Examining these functions more, we can see that one of them builds a string by concatenating two separate string values. That combined string (**`sn0wwyygl0be1337`**) is then fed into the XOR routine and is used as the encryption key. Below is also a helper function for base64 decoding.

Following the code, we can see an array of base64-encoded strings being passed into the `qmJJvtPnmp` function and then used inside `scan`. Based on the analysis, its valid to assume this is used to define which file extension to look for and then later exfiltrate matching files to a remote host.

<p align="center">
  <img src="assets/array.png" alt="arr" width="800">
</p>

After all, this isn't really a harmless code highlighting extension at all. It's a malicious extension designed to scan for specific files like configs and secrets and exfiltrate them to a remote server quietly.

<p align="center">
  <img src="assets/core.png" alt="core" width="800">
</p>

With enough information and recon, we can proceed to solve this challenge.

# Solution

```py
import base64

key = b"sn0wwyygl0be1337"

enc = [
    "XQ9HBA==",
    "XQVFFRI=",
    "XR1DHw==",
    "XQteAQ==",
    "XR5UEQ==",
    "XQpfFA8=",
    "OzpyDDpNFQ4PWVIQQmw=",
    "JT1zKDIBDQICQ1NVXwY=",
    "LF9DKBlJDQ9dXgU6XwBESg=="
]

def xor(data, key):
    return bytes(b ^ key[i % len(key)] for i, b in enumerate(data))

for x in enc:
    data = base64.b64decode(x)
    dec = xor(data, key)
    print(dec.decode("utf-8"))

```

Putting the 3 pieces of the flag together in order we get the full flag!
