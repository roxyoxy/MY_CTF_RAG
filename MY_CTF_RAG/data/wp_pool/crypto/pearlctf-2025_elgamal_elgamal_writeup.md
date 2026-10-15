# pearlctf 2025 elgamal elgamal writeup

# Attack on the ElGamal

```
Fed up with decrypting? Let's do this. 
You give me the ciphertext, and I decrypt it this time. Remember to use it effectively.
```

We obtain the values for `p`, `g`, `h`, and the ciphertexts `c1` and `c2`, where the plaintext is our flag. The player can enter a different `c1'` and `c2'` (other than the ones given), and the machine promises to decrypt these ciphertexts using the same `p`, `g`, and `h`.

This challenge is based on the **homomorphic property of ElGamal**, where if the ciphertext is multiplied by `n` (i.e., `c2' = (n * c2) % p`), the decrypted text will also be `n` times the initial plaintext.

We use `n = 2` and `n = 3`, compute their modular inverses, retrieve `m` while considering possible modular wraparounds, and convert it into bytes to obtain the flag.


```python
from Crypto.Util.number import inverse
from pwn import remote

HOST = "attack-on-the-elgamal.ctf.pearlctf.in"
PORT = 30007

conn = remote(HOST, PORT)

data = conn.recvuntil(b"numbers:").decode()
print("Received Data:\n", data)

lines = data.split("\n")

p = int(lines[0].split(": ")[1])
g = int(lines[1].split(": ")[1])
h = int(lines[2].split(": ")[1])
ciphertext_line = lines[3].split(": ")[1]
c1, c2 = map(int, ciphertext_line.strip("()").split(", "))

c2_2x = (2 * c2) % p
c2_3x = (3 * c2) % p

ciphertext_2x = f"{c1}, {c2_2x}"
ciphertext_3x = f"{c1}, {c2_3x}"

print("Sending 2*c2 ciphertext:")
conn.sendline(ciphertext_2x.encode()) 
response_2x = conn.recvuntil(b"numbers:").decode().strip()
tmp = response_2x.split("\n")
m_2 = int(tmp[0].split(": ")[-1])
print("m2: ", m_2)

print("Sending 3*c2 ciphertext:")
conn.sendline(ciphertext_3x.encode()) 
response_2x = conn.recvuntil(b"numbers:").decode().strip()
tmp = response_2x.split("\n")
m_3 = int(tmp[0].split(": ")[-1])
print("m3: ", m_3)

inv_2 = inverse(2, p)  # 2^(-1) mod p
inv_3 = inverse(3, p)  # 3^(-1) mod p

base_m_from_2 = (m_2 * inv_2) % p
base_m_from_3 = (m_3 * inv_3) % p

max_attempts = 100  # Increase if needed
for k in range(max_attempts):
    for base_m in [base_m_from_2, base_m_from_3]:
        m_candidate = base_m + k * p  # Adjust for modular wraparound
        flag_bytes = m_candidate.to_bytes((m_candidate.bit_length() + 7) // 8, 'big')
        if b"{" in flag_bytes:
            print(f"Possible Flag Found: {flag_bytes.decode()}")
            exit(0)  # Stop once a valid flag is found

print("No valid flag found, try increasing max_attempts.")
```

The flag is: ```pearl{g4m31_tr0ub13}```
