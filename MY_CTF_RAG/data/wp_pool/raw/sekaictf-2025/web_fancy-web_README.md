# web fancy web README

## Fancy Web

| Author       | Difficulty  | Points | Solves |
| ------------ | ----------- | ------ | ------ |
| DimasMaulana | 2⯁ (Normal) | 500    | 0      |

---

### Description

<blockquote>

The Ministry of Information and Communications Technology of Konoha has recently launched their new official website. While it appears to be a standard government portal showcasing public services and announcements, our intelligence sources have indicated that this WordPress-based website contains hidden information that could expose corruption and human rights violations. The website features a unique table processing system that displays various government data, but our analysts suspect that the developers have hidden sensitive information within the table structures themselves. The site's administrators are known for their sophisticated obfuscation techniques, making it difficult to distinguish between legitimate public data and concealed evidence. Your mission is to investigate this website and uncover the hidden information by looking beyond the surface-level content and examining how the tables are processed and displayed - the truth might be hidden, waiting for someone with the right skills to reveal it.

<https://ctf.sekai.team/launcher/>

<details closed>
<summary><b>Hint(s)</b>:</summary>

1. Taking a closer look at `in_array` might offer some inspiration on where to look next.
2. The intended solution is to use `__toString` Gadget.

</details>
</blockquote>

### Challenge Files

- [dist.zip](dist)
