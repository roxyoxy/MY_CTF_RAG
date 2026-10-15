# htb-university-2025_Clock Work Memory

<img src="../../assets/banner.png" style="zoom: 80%;" align=center />

<img src="../../assets/htb.png" style="zoom: 80%;" align='left' /><font size="5">Clock Work Memory</font>

  16<sup>th</sup> 12 2025

  Prepared By: kailash

  Challenge Author: kailash

  Difficulty: <font color=green>Very Easy</font>

  Classification: Official






# Synopsis

Clock Work Memory is a Very Easy Reversing challenge. Players must decompile the provided `pocketwatch.wasm` WebAssembly binary into its human-readable text format (`.wat`). By analyzing this text file, they must identify a hardcoded encrypted string, a hardcoded 4-byte integer key, and the repeating-key XOR algorithm used. Finally, they must write a simple script to apply the key to the data to decrypt the flag.

## Skills Required
    - Python
    - Researching
## Skills Learned
    - Disassembling and reversing WebAssembly
    - Parsing different data formats

# Solution

## Analyzing the source code

We are provided with a single file: `pocketwatch.wasm`.

As a first step, we can run `strings` on the binary to see if there are any obvious clues.

```Bash
strings pocketwatch.wasm
memory
check_flag
__indirect_function_table
_initialize
_emscripten_stack_restore
emscripten_stack_get_current
A k"
A j$
0#{0&
s'un>
```
The `strings` command shows us some exported function names, notably `check_flag`, but no flag or obvious key. This means the logic is compiled into the binary, and we need to decompile it.

The standard tool for converting a `.wasm` binary to the human-readable WebAssembly Text Format (`.wat`) is `wasm2wat` (part of the WABT toolkit).

```Bash
$ wasm2wat pocketwatch.wasm -o pocketwatch.wat
```

This command gives us the `pocketwatch.wat` file, Using cat on the file gives us the  full source code of the module.

If we look at `pocketwatch.wat`, we can see that our goal is:

- Specify the goal of the challenge: The flag is hidden within the `.wasm` file. It is stored as an encrypted string in the `data` section and can be decrypted by reversing the logic found in the `check_flag` function.

The basic workflow of the script is as follows:

1. A function `(func 1)` (which is exported as `check_flag`) is the main logic.
2. This function contains a loop that performs an XOR operation.
3. The data for this XOR operation is loaded from two places: a hardcoded data block and a hardcoded integer.

A little summary of all the interesting things we have found out so far:

1. The encrypted flag is stored in the data section at offset 1024: `(data (;0;) (i32.const 1024) "\1c\1b\010#{0&\0b=p=\0b~0\147\7fs'un>")`
2. A hardcoded 4-byte integer, `1262702420`, is stored in memory. This is almost certainly the key.
3. The `check_flag` function's loop uses a bitwise `i32.and` with `i32.const 3`, which indicates a repeating-key cipher with a key length of 4 bytes.


## Finding the vulnerability

**1. Finding the Encrypted Data**

At the bottom of the `pocketwatch.wat` file, we find the `(data ...)` section.

```Bash
(data (;0;) (i32.const 1024) "\1c\1b\010#{0&\0b=p=\0b~0\147\7fs'un>")
```
This is our encrypted flag, stored at memory offset `1024`. It's represented as a string with custom-escaped hex bytes.

**2. Finding the Key**

Inside `(func 1)` (the `check_flag` function), we see a constant integer being stored in memory.

```wat
(func (;1;) (type 1) (param i32) (result i32)
  i32.const 1262702420
  i32.store offset=27 align=1
```

This number, `1262702420`, seems random. However, if we convert it from decimal to hexadecimal, its purpose becomes clear.

 - 1262702420 (decimal) = 4B434F54 (Hex) = KCOT (ASCII) 

The files uses little endian and hence the ASCII is reversed , so our key would be `TOCK`.

**3. Finding the Algorithm**

Inside the main loop of `(func 1)`, we see the following key instructions:

```assembly

  local.get 1         ;; This is the loop counter 'i'
  i32.const 3         ;; The number 3 (binary 11)
  i32.and           ;; This calculates (i % 4)
  i32.add           ;; Adds to the key's base address
  i32.load8_u       ;; Loads one byte of the key
  i32.load8_u offset=1024 ;; Loads one byte of encrypted data
  i32.xor             ;; Performs the XOR operation
```

This logic confirms our theory: it's a **repeating-key XOR** cipher. It XORs each byte of the encrypted data with a byte from the key "TOCK", repeating the key as it goes .

## Exploitation

Let us consider our attack scenario.

- First, we need to parse the custom-escaped string from the `.wat` file into a clean byte array.
- We will use the provided Python helper script for this.
- Next, we define our key, `TOCK`.
- Finally, we loop through the encrypted byte array, XORing each byte `i` with `key[i % 4]`.
- The result of this operation, converted back to characters, will be the flag.

Note : 
The string `\1c\1b\010#{0&\0b=p=\0b~0\147\7fs'un>` is not regular unescape or hex it's Wat specific so we would have to recover hex from this to then perform the xor . 

The attack explained above can be implemented with the following code. First, we'll set up some keys to begin decoding.

```python
encoded_key = [0x4E, 0x55, 0x59, 0x51]
meta = 0x1A
key_bytes = bytes([b ^ meta for b in encoded_key])
key = key_bytes.decode()

print("Recovered key:", key)
```

We'll then use them to decode the flag.
```python
encoded_flag = [
0x1C, 0x1B, 0x01, 0x30, 0x23, 0x7B, 0x30, 0x26, 0x0B, 0x3D,
0x70, 0x3D, 0x0B, 0x7E, 0x30, 0x14, 0x37, 0x7F, 0x73, 0x27,
0x75, 0x6E, 0x3E
]

decoded = bytes([b ^ key_bytes[i % len(key_bytes)] for i, b in enumerate(encoded_flag)])
print("Decoded flag:", decoded.decode())
```
