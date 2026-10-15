# misc_sekai-bank-transaction_README

## Sekai Bank - Transaction

| Author | Difficulty  | Points | Solves | First Blood | Time to Blood |
| ------ | ----------- | ------ | ------ | ----------- | ------------- |
| Marc   | 5⯁ (Master) | 292    | 17     | CatBox      | 5 hours       |

---

### Description

Tags: `SEKAI`, `Mobile`

<blockquote>

Let me introduce you to Sekai Bank!

**Notes**:
This is an Android Exploitation challenge that requires you to create an exploit application (.apk). Once you have created a working exploit, please submit it to POC Tester below. It is recommended that you verify the exploit works locally before submitting.

**Objectives**:
You need to steal one million from the user `admin`, which is the Sekai Bank user that the POC Tester is authenticated with. Please refrain from performing penetration testing on the backend, as it will yield no results; this is a pure Android Exploitation challenge. The flag will be displayed in the transaction history if your exploitation was successful.

**POC Tester Flow**:

1. Vuln App (Sekai Bank) will be launched.
2. It will authenticate as user `admin` and create two transactions. The first transaction is an instant transaction, and the second one is a delayed transaction (will be sent within 5 minutes after the transaction is placed).
3. Exploit App (Your POC) will be launched.
4. Your POC will have up to 5 minutes to run the exploit.
5. The final screenshot will be provided in the POC Tester.

> ❖ Warning
> Attempting to mass-create accounts and accumulating a total of one million, then transferring it, won't give you the flag. The condition for the flag to be sent is only when the user `admin` sends a million to the target user.

</blockquote>

### Challenge Files

- [SekaiBank.apk](dist)
