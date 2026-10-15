# htb university 2025 CloudyCore

<img src="../../assets/banner.png" style="zoom: 80%;" align=center />

<img src="../../assets/htb.png" style="zoom: 80%;" align='left' /><font size="5">Cloudy-Core</font>

  16<sup>th</sup> 12 2025

  Prepared By: kailash

  Challenge Author: kailash

  Difficulty: <font color=orange>Medium</font>

  Classification: Official






# Synopsis

Cloudy-Core is a Medium Reversing challenge. Players must reverse a TensorFlow model file to extract keys and recover the flag.

## Skills Required
    - Python scripting
    - Basic cryptography primitives
## Skills Learned
    - ML model reversing
    - Systematic reversing

# Solution

## Analyzing the source code

- Upon unzipping the challenge, we are provided with a single file: `snownet_stronger.tflite`. This is the "source file" that must be analyzed.

- The first step is to perform basic enumeration on this binary file.

- **`file` command:**

	- Running `file snownet_stronger.tflite` will output: `TensorFlow Lite model`.
	- This confirms the file type and tells us we are dealing with a machine learning model. This suggests the challenge is either about ML or (more likely) a reversing challenge using a `.tflite` file as a container.
        
- **`strings` command:**
    
    - Running `strings snownet_stronger.tflite` reveals several readable clues:
        
        ```
		TFL3
		serving_default
		output_1
		output_0
		in_payload
		in_meta
		CONVERSION_METADATA
		min_runtime_version
		2.19.0
		1.5.0
		MLIR Converted.
		main
		StatefulPartitionedCall_1:0
		StatefulPartitionedCall_1:1
		functional_2_1/meta_holder_1/MatMul
		arith.constant
		serving_default_in_meta:0
		serving_default_in_payload:0
        ```
        
    - **Analysis:** 
        
        - `in_payload` and `in_meta` strongly suggest the model contains two distinct data blocks: a **'payload'** (likely the encrypted flag) and **'meta'** data (likely the key).
        
        - `functional_2_1/meta_holder_1/MatMul` reinforces this by explicitly naming a **'meta_holder'**.
        
    
## Neutron inspection

- After using `strings` to form our hypothesis, we use **Netron** to confirm our findings. Netron is a visualizer for neural network models and is perfect for this.

- When we open `snownet_stronger.tflite` in Netron, we can see the model's graph structure and find the tensors (data blocks) we're looking for.

- **What we find in Netron:**
    1. **Meta Tensor:** We can click on the `meta_holder` layer (which we found in `strings`). In its properties, we'll find a constant **weight tensor** with a small, fixed size: **4 elements** (`[1, 4]`) of type `float32`. This confirms it's our key block.
    2. **Payload Tensor:** We can find the layer associated with the payload (e.g., `snownet_dense`). We'll find its `kernel` (weight) tensor has a larger, fixed size: **9 elements** (`[9, 1]`) of type `float32`. This confirms it's our data block.

- This visual confirmation is the final step of enumeration. We now know _exactly_ where our two target data arrays are, their sizes, and their data types. We can now proceed to write a script to extract them.


# Solution

## Finding the vulnerability 

- Based on our enumeration (`strings` and Netron), we have identified two key data tensors:
    1. A **4-element `float32` "meta" tensor** (possibly our key).
    2. A **9-element `float32` "payload" tensor** (possibly our encrypted data).

- **Reversing the Key Algorithm:** We first need to figure out how the 4 `float32` "meta" elements (16 bytes total) are used to create the decryption key. Our initial hypothesis is to check for a simple byte-to-float conversion (a value from 0-255).We can come up with a simple python program to do this and check the values.

```python
import tensorflow as tf
import numpy as np

interpreter = tf.lite.Interpreter(model_path="snownet_stronger.tflite")
interpreter.allocate_tensors()

all_tensors = interpreter.get_tensor_details()
meta_details = next(t for t in all_tensors if np.prod(t['shape']) == 4)
payload_details = next(t for t in all_tensors if np.prod(t['shape']) == 9)
meta_floats = interpreter.get_tensor(meta_details['index']).flatten()
payload_floats = interpreter.get_tensor(payload_details['index']).flatten()

print(f"Meta Floats: {meta_floats}")
print(f"Payload Floats: {payload_floats}")
```

- This script uses the `tensorflow` library to load the model, find the _first_ tensor with 4 elements and the _first_ with 9, and print their float values.

when we run this script we get an output like 

```array
[ 6.1588016e-14 -4.4441445e+28 -5.4198931e-34  2.4949116e-33
  3.1142921e-27  4.1407697e-32  9.0417994e-17  4.8709663e-14
  3.1809475e-43]
```

1. The `Payload Floats` are large, complex numbers, which **disproves our "0-255" hypothesis**. This confirms the data is hidden using a more complex method, like bit-reinterpretation.

2. The `Meta Floats` might be all zeros. This tells us that the simple `next()` command isn't good enough, as the model has _multiple_ tensors of that size. Our final solution will need to be smarter and check _all_ potential tensors.

**Reversing the Payload Steganography:**We have 9 floats, but we need bytes to decrypt. Our script confirmed these are large, random-looking numbers, not simple byte-to-float conversions. This strongly implies **bit-reinterpretation**. This is a 1-to-1, reversible operation. We will hypothesize that the 9 floats (40 bytes) are the raw, encrypted bytes simply _interpreted_ as floats. We'll reverse this by reinterpreting the float bits back into their original 40 bytes.

**Reversing the Encryption:** The description explicitly gives us the crypto algorithm: **"simple XOR"**.

### How we will proceed : 

1. **Stage 1:** Systematically test key-derivation algorithms on all 4-element "meta" floats until we find a potential key.

2. **Stage 2:** Use the "bit-reinterpretation" logic to extract the 40 encrypted bytes from the correct 9-element "payload" float .

3. **Stage 3:** Use our potential key from Stage 1 to decrypt the data from Stage 2 using a repeating-key XOR.

4. **Stage 4:** The result of the XOR should be a flag .

## Exploitation

For our stage 1 . We will write a "test harness" script to check all potential 'meta' tensors and all common derivation algorithms to find a human-readable key . There are 2 ways that this can go from here .
	- The first way is we find a human readable key , which looks readable and we can then confirm this is the correct approach also making it the challenge significantly easier.
	- The second way being we find no human readable key and we have to hypothesize that the key is in a non-readable format would have to begin brute-force.

```python

import tensorflow as tf
import numpy as np
import struct
import warnings
import os

warnings.filterwarnings("ignore", message=".*TFLiteConverter.*", category=UserWarning)
tf.get_logger().setLevel('ERROR')

TFLITE_FILE = "snownet_stronger.tflite"
KEY_FLOAT_COUNT = 4

def get_key_from_LSB(floats):
    key = bytearray()

    for f in floats:
        u32 = struct.unpack("<I", struct.pack("<f", f))[0]
        key.append(u32 & 0xFF)  # LSB
    return bytes(key)
    
def get_key_from_MSB(floats):
    key = bytearray()
    for f in floats:
        u32 = struct.unpack("<I", struct.pack("<f", f))[0]
        key.append((u32 >> 24) & 0xFF)  # MSB
    return bytes(key)
    
def get_key_from_Byte1(floats):
    key = bytearray()
    for f in floats:
        u32 = struct.unpack("<I", struct.pack("<f", f))[0]
        key.append((u32 >> 8) & 0xFF)
    return bytes(key)

def get_key_from_Byte2(floats):
    key = bytearray()
    for f in floats:
        u32 = struct.unpack("<I", struct.pack("<f", f))[0]
        key.append((u32 >> 16) & 0xFF)
    return bytes(key)

def is_printable(key_bytes):
    if not key_bytes or len(key_bytes) != KEY_FLOAT_COUNT:
        return False
    return all(32 <= b < 127 for b in key_bytes)

def find_keys():
    print(f"Attempting to find keys in '{TFLITE_FILE}'...\n")
    if not os.path.exists(TFLITE_FILE):
        print(f"Error: File not found: {TFLITE_FILE}")
        return
        
    try:
        interpreter = tf.lite.Interpreter(model_path=TFLITE_FILE)
        interpreter.allocate_tensors()
    except Exception as e:
        print(f"Error loading TFLite model: {e}")
        return
        
    tensor_details = interpreter.get_tensor_details()
    potential_meta_tensors = []

    print("Scanning for potential key tensors (size=4)...")
    
    for details in tensor_details:
        shape = details['shape']
        if any(s is None for s in shape):
            size = -1
        else:
            size = np.prod(shape)

        if size == KEY_FLOAT_COUNT:
            potential_meta_tensors.append(details)
            print(f"  > Found potential meta tensor at index {details['index']} (shape {shape})")

    if not potential_meta_tensors:
        print("\nError: Could not find any tensors with {KEY_FLOAT_COUNT} elements.")
        return

 
    print("\n--- Testing Key Derivation Algorithms ---")
    found_key = False
    key_algorithms_to_try = [
        ("LSB", get_key_from_LSB),
        ("MSB", get_key_from_MSB),
        ("Byte1", get_key_from_Byte1),
        ("Byte2", get_key_from_Byte2)
    ]

    for meta_details in potential_meta_tensors:
        meta_tensor = interpreter.get_tensor(meta_details['index'])
        if np.all(meta_tensor == 0):
            print(f"\nSkipping meta tensor {meta_details['index']} (all zeros)")
            continue
      
        print(f"\nTesting meta tensor {meta_details['index']}...")
        for name, key_func in key_algorithms_to_try:
            derived_key = key_func(meta_tensor.flatten())
            if is_printable(derived_key):
                print(f"  [!!! SUCCESS !!!]")
                print(f"  Found HUMAN-READABLE key: '{derived_key.decode()}'")
                print(f"  Using algorithm: {name}")
                found_key = True
            else:
                print(f"  > Tried {name}, got non-printable key: {derived_key.hex()}")
    if not found_key:
        print("\n--- Search Complete ---")
        print("No human-readable keys were found.")
        print("The key is likely non-printable (like 'd50598bc')")
        print("and can only be verified by decrypting the payload.")

if __name__ == "__main__":
    find_keys()
```
This script, `extract_key.py`, automates our search for the key. It loads the `.tflite` file, finds all 4-element tensors, and then loops through them, applying our different hypotheses (LSB, MSB, etc.) to see if any combination produces a human-readable key. 

Amazingly the script gives us something 🥹 Which valuate's our hypothesis .

```key
Scanning for potential key tensors (size=4)...
  > Found potential meta tensor at index 1 (shape [1 4])
  > Found potential meta tensor at index 3 (shape [1 4])
--- Testing Key Derivation Algorithms ---
Skipping meta tensor 1 (all zeros)
Testing meta tensor 3...
  [!!! SUCCESS !!!]
  Found HUMAN-READABLE key: 'k3y!'
  Using algorithm: LSB
  > Tried MSB, got non-printable key: 00000000
  > Tried Byte1, got non-printable key: 00000000
  [!!! SUCCESS !!!]
  Found HUMAN-READABLE key: '@@@@'
  Using algorithm: Byte2
```
We do have 2 candidates , but i guess it's pretty clear the `k3y!` seems to be the key . Now that we have this we move on to extracting the 36 bytes (9 * 4 = 36) of the encrypted blob and then try to decode it with xor as hinted in the description.

```python
def floats_to_bytes_as_bitpatterns(floats):
    byte_array = bytearray()
    for f in floats:
        u32 = struct.unpack("<I", struct.pack("<f", f))[0]
        byte_array.extend(struct.pack("<I", u32))
    return bytes(byte_array)

def xor_repeating(data, key):
    out = bytearray()
    klen = len(key)
    for i, b in enumerate(data):
        out.append(b ^ key[i % klen])
    return bytes(out)
```
We can then use these functions to then reverse the float as bytes and then decrypt using xor .

### Getting the flag

The Exploit can be represented by code as:

```python
#!/usr/bin/env python3

import struct
import zlib

HEX_PAYLOAD_OBF = (
    "13 af 8a 29 1a 99 0f ef 5a 1b 34 88 e7 44 4f 09"
    "59 bd 76 13 45 00 57 0b 5d 7d d0 24 6b 5e 5b 29"
    "e3 00 00 00"
)
VERIFIED_KEY = b'k3y!'

def xor_repeating(data, key_bytes):
    out = bytearray()
    klen = len(key_bytes)
    for i, b in enumerate(data):
        out.append(b ^ key_bytes[i % klen])
    return bytes(out)

def hex_dump(data, length=16, prefix="  "):
    for i in range(0, len(data), length):
        chunk = data[i:i + length]
        hex_offset = f"{i:08x}:"
        hex_values = " ".join(f"{b:02x}" for b in chunk).ljust(length * 3 - 1)
        ascii_values = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        print(f"{prefix}{hex_offset} {hex_values}  |{ascii_values}|")

def decrypt_payload():
    print("--- Decrypting Hardcoded Payload ---")
    payload_obf = bytes.fromhex(HEX_PAYLOAD_OBF)
    print(f"Loaded {len(payload_obf)} bytes of obfuscated data:")
    hex_dump(payload_obf)
    print(f"\nDecrypting with repeating-key XOR (Key: {VERIFIED_KEY}) ...")
    payload_compressed = xor_repeating(payload_obf, VERIFIED_KEY)
    print("\nVerifying result (should be zlib-compressed data):")
    hex_dump(payload_compressed)
    if not payload_compressed.startswith(b'\x78'):
        print("\n[FAIL] Zlib header not found! Key or data is wrong.")
        return
        
    print("\n[SUCCESS] Zlib header found! Decompressing...")
    try:
        flag_bytes = zlib.decompress(payload_compressed)
        flag = flag_bytes.rstrip(b'\x00').decode('utf-8')
        print("\n" + "="*30)
        print(f"  FLAG: {flag}")
        print("="*30)
    except zlib.error as e:
        print(f"  [FAIL] Zlib decompression FAILED: {e}")
if __name__ == "__main__":
    decrypt_payload()
```
