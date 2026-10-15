# justctf 2025 private README~1

# Solution

1. Exploit race condition in `/getIdentity` - `if err = validateSessionName(sessionName); err != nil`. Spamming server with requests won't work because of network noise. Instead, send n requests without last "\r\n", wait for all requests to be sent, and send last "\r\n\" 
1. Use race condition to send `/getIdentity?session_name=i-01bc8314faa773ad7` to impersonate admin instance. It allows us to bypass this constraint `bound_subnet_id=${ADMIN_SUBNET}`
2. Append `"role": "admin_role"` to the json returned in previous step, and send it to `/getToken`
3. Use the token to get `admin-data/data/flag`