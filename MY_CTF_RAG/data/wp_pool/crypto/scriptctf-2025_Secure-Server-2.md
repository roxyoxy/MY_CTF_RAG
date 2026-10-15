# scriptctf-2025_Secure-Server-2

# Secure-Server-2 | NoobMaster
- Description: This time, the server is even more secure, but did it actually receive the secret? Simple brute-force won't work!

files.zip: /attachment/files.zip

# Write-up

AES is not commutative, so the server DOES NOT receieve the secret. The solution is Meet-In-The-Middle. The three values you are given are:
1) E2(E1(flag)) # E1 = encryption with key1 etc.
2) E4(E3(E2(E1(flag))))
3) D2(D1(E4(E3(E2(E1(flag))))))

Looking at the first two provided values, the second one is first one encrypted with keys 3 and 4. To MITM:
* Encrypt the first provided value with all possible key values (2**16) (With correct key, output should be `E3(E2(E1(flag)))`)
* Decrypt the second provided value with all possible key values (2**16) (With correct key, output should be `E3(E2(E1(flag))))

When these two are equal, you have found keys 3 and 4! This is a computation of 2**17 rather than 2**32 via a normal bruteforce

For keys 1 and 2:
* Encrypt the third provided value with all possible key values (2**16) (With correct key, output should be `D1(E4(E3(E2(E1(flag)))))`)
* Decrypt the second provided value with all possible key values (2 ** 16) (With correct key, output should be `D1(E4(E3(E2(E1(flag)))))`)

When these two are equal, you have found keys 1 and 2! This is a computation of 2**17 rather than 2**32 via a normal bruteforce.

With MITM: 2**17 + 2**17 = 2**18
With Simple bruteforce: 2**32+2**32 = 2**33

Solve at `/solve/solve.py`

# Flag - scriptCTF{s3cr37_m3ss4g3_1337!_7e4b3f8d}
