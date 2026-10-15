# justctf 2025 private README

### Solution

1. Create a challenge instance
2. Set `server` variable in `xss-v3.3.6.sh` to the instance URL and append `:3333` port
3. In `exploit.html` set the `EXFIL` variable to your exfil server
4. Run `xss-v3.3.6.sh`
5. Send `exploit.html` to the admin (the bot is on `:3332` port)
6. In a couple of minutes the flag should arrive