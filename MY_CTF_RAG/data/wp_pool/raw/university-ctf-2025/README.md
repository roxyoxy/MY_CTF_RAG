# README

<p align='center'>
  <img src='assets/banner.png' alt="HTB">
</p>

# University CTF 2025 Official Writeups

# [**Challenges**](#challenges)

| Category | Name | Objective | Difficulty [⭐⭐⭐⭐⭐] |
| - | - | - | - |
| Pwn | [SHL337](<pwn/SHL337>) | Write a one-line assembly code to shift left some bytes on a certain register with the correct value. | ⭐ |
| Pwn | [Feel My Terror](<pwn/Feel My Terror>) | format string vulnerability to overwrite 5 values using the fmtstr_payload from pwntools. | ⭐⭐ |
| Pwn | [Starshard Core](<pwn/Starshard Core>) | Exploit a format string vulnerability to leak heap, PIE, and libc addresses. Then, leverage a Use-After-Free vulnerability to overwrite a FILE structure in the heap, create a fake wide_data_struct, and overwrite pointers to call __GI__IO_wfile_overflow and trigger the exploit chain. | ⭐⭐⭐ |
| Reversing | [Clock Work Memory](<reversing/Clock Work Memory>) | Decompiling WASM and identifying hardcoded XOR keys | ⭐ |
| Reversing | [Starshard Reassembly](<reversing/Starshard Reassembly>) | Analyzing a Go binary using dynamic dispatch for flag checking | ⭐⭐ |
| Reversing | [CloudyCore](<reversing/CloudyCore>) | Reverse engineering a TensorFlow model file to extract keys | ⭐⭐⭐ |
| Web | [SilentSnow](<web/silentsnow>) | Identify and exploit an Arbitrary Option Update vulnerability in a custom WordPress plugin to escalate privileges, then achieve Remote Code Execution (RCE) to retrieve the flag. | ⭐ |
| Web | [DeadRoute](<web/deadroute>) | The user must exploit a race condition in the custom Go router implementation to bypass IP-based access control and retrieve an authentication token. Using the obtained token, the user must exploit a path traversal vulnerability in the note reading functionality to retrieve the flag from the root directory. | ⭐⭐ |
| Web | [PeppermintRoute](<web/peppermintroute>) | Perform unseen SQL Injection to auth bypass by using type confusion in javascript. Perform zipslip to overwrite server files, and then trigger DOS by unhandled asynchronus exception | ⭐⭐⭐ |
| Crypto | [Optimistic](<crypto/Optimistic>) | Break Nihilist cipher with known keyword and known key length | ⭐ |
| Crypto | [One Trick Pony](<crypto/One Trick Pony>) | Break PRNG based on the Legendre symbol using Meet in The Middle and predict the following outputs | ⭐⭐ |
| Crypto | [Disguised](<crypto/Disguised>) | Break 1-round AES using two plaintext/ciphertext pairs | ⭐⭐⭐ |
| Forensics | [Snowy Extension](<forensics/Snowy Extension>) | Disect a malicious vscode extension by analyzing javascript code. | ⭐ |
| Forensics | [A Trail of Snow and Deception](<forensics/A Trail of Snow and Deception>) | Aanalyzing a provided network capture file to gather key details about attack | ⭐⭐ |
| Forensics | [SantaGiveaway](<forensics/SantaGiveaway>) | analyze a memory dump to uncover how an Amadey malware variant infiltrated the system after the user was tricked into running a fake “giveaway installer,” ultimately leading to full system compromise. | ⭐⭐⭐ |
| Coding | [Flickering Snowglobe](<coding/Flickering Snowglobe>) | Process a long string and count the number of maximal contiguous segments of identical characters. A new segment begins whenever the current character differs from the previous one. The solution requires a single linear pass over the input. | ⭐ |
| Coding | [Bauble Sort](<coding/Bauble Sort>) | Parse structured input records and perform a multi-key sort. Baubles must be ordered by descending sparkle, then ascending stability, and finally lexicographically by identifier. Efficient sorting is required to handle large input sizes. | ⭐⭐ |
| Coding | [Cellcode](<coding/Cellcode>) | Simulate a cellular automaton on an NxN grid for T generations with parallel updates. Each cell’s next state depends on the count of alive neighbors (8-directional) and a B/S rulestring that defines birth and survival conditions. | ⭐⭐⭐ |
| OSINT | [Tinselwick](<osint/Tinselwick/>) | Intelligence investigation combining registry comparison, audit log analysis, change detection, and data correlation to discover the tampered child record | ⭐ |
| OSINT | [SleighComms](<osint/SleighComms/>) | SIGINT challenge involving audio signal analysis, simple substitution cipher decoding (tone count to letter pairs), pattern recognition, and database cross-referencing | ⭐⭐ |
| OSINT | [FrostFleet](<osint/FrostFleet/>) | Maritime OSINT challenge involving coordinate analysis, Haversine distance calculations, multi-source data correlation (AIS data, dock registry, captain's logs), and analytical reasoning to pinpoint vessel location | ⭐⭐⭐ |
