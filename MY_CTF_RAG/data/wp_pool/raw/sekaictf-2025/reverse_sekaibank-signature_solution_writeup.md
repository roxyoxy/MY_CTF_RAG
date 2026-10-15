# reverse_sekaibank-signature_solution_writeup

## Sekai Bank - Signature Writeup

Brief write-up of Sekai Bank - Signature.

After downloading the APK, we use jadx to decompile it. Looking through all the files in `com.sekai.bank`, especially searching for keyword "Signature", we found relevant logic in `/network` folder.

`ApiService`:

```java
@POST("flag")
Call<String> getFlag(@Body FlagRequest flagRequest);
```

`ApiClient` has the logic for making network requests and handling responses. Feeding all code to LLMs and we get a good understanding of how the API works.

```console
$ curl http://149.28.148.83:3000/api/flag
{"success":false,"error":"X-Signature header is required"}
```

Since `X-Signature` is required, and it is calculated via the following formula in our code:

```
HMAC_SHA256(
    method + "/api" + path + body,
    sha256(app_signature)
)
```

We need to get the app's signing certificate hash. We can use [print-apk-signature](https://github.com/warren-bank/print-apk-signature) to get it:

```console
$ ./print-apk-signature SekaiBank.apk
Verifies
Verified using v1 scheme (JAR signing): false
Verified using v2 scheme (APK Signature Scheme v2): true
Verified using v3 scheme (APK Signature Scheme v3): false
Number of signers: 1
Signer #1 certificate DN: C=ID, ST=Bali, L=Indonesia, O=HYPERHUG, OU=Development, CN=Aimar S. Adhitya
Signer #1 certificate SHA-256 digest: 3f3cf8830acc96530d5564317fe480ab581dfc55ec8fe55e67dddbe1fdb605be
Signer #1 certificate SHA-1 digest: 2c9760ee9615adabdee0e228aed91e3d4ebdebdf
Signer #1 certificate MD5 digest: fcab4af1f7411b4ba70ec2fa915dee8e
Signer #1 key algorithm: RSA
Signer #1 key size (bits): 2048
Signer #1 public key SHA-256 digest: bb2a8180a2291113361629892f6c57d9e57ae33a65d8bb9922eb302feaf5efff
Signer #1 public key SHA-1 digest: dca7dbe8eb77680f4c6da71a82e80165153a1486
Signer #1 public key MD5 digest: cd1a32fcbec386791523cff50a628912
```

We have extracted the SHA-256 digest of the APK signing certificate, which is `3f3cf8830acc96530d5564317fe480ab581dfc55ec8fe55e67dddbe1fdb605be`. Now we just need a simple Python script to calculate the HMAC signature and make the request:

```py
import hmac
import hashlib
import requests

APP_SIGNATURE_SHA256 = "3f3cf8830acc96530d5564317fe480ab581dfc55ec8fe55e67dddbe1fdb605be"
json_body = '{"unmask_flag":true}'
string_to_sign = "POST/api/flag" + json_body

signature = hmac.new(
    bytes.fromhex(APP_SIGNATURE_SHA256),
    string_to_sign.encode("utf-8"),
    hashlib.sha256
).hexdigest()

headers = {
    "Content-Type": "application/json",
    "X-Signature": signature
}

response = requests.post(
    "http://149.28.148.83:3000/api/flag",
    headers=headers,
    json={"unmask_flag": True}
)
print(response.text)
```

This gives the flag `SEKAI{are-you-ready-for-the-real-challenge?}`. The challenge is fairly straightforward, you can prompt [ChatGPT](https://chatgpt.com/share/687f95d6-5280-800a-88b3-b6d9a95420b8) for all the solve steps easily.