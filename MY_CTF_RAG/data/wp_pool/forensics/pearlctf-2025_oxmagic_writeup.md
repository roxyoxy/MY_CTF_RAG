# pearlctf 2025 oxmagic writeup

# Pearl CTF 2025 - oxmagic

## Title: oxmagic

### Description:
Something seems off about this file. Can you figure out what’s hidden beneath? Wrap the hidden message in lowercase with underscores in `pearl{}`.

### Files:
- chall.jpg

---

## Solution

### Metadata Inspection:
Running `exiftool` on `chall.jpg` revealed an unusual entry in the `Artist` field:

```
Artist : aV9sb3ZlX3Jvc2U=
```

Decoding this Base64 string:

```
$ echo "aV9sb3ZlX3Jvc2U=" | base64 -d
i_love_rose
```

### Extracting Hidden Data with steghide:
Using the decoded string as the passphrase, we extract hidden data from the image:

```
$ steghide extract -sf chall.jpg
Enter passphrase: i_love_rose
wrote extracted data to "flag.txt".
```

### Repairing the Extracted File:
The extracted file (`flag.txt`) didn’t immediately reveal its type—`file flag.txt` reported it as generic data. 

While examining it in a hex editor (e.g., HxD), we noticed its header resembled a damaged WAV file header.

The proper WAV header should begin with:

```
52 49 46 46 ?? ?? ?? ?? 57 41 56 45
```

but we observed:

```
EF BB BF 46 D4 B2 01 00 57 41 56 45
```

After repairing the header, running `file flag.txt` confirmed it’s now recognized as a WAV file:

```
$ file flag.txt
flag.txt: RIFF (little-endian) data, WAVE audio, Microsoft PCM, 8 bit, mono 8000 Hz
```

We then rename it to `flag.wav`.

### Decoding the Hidden Message:
Listening to `flag.wav` reveals a Morse code audio message. Decoding the Morse yields:

```
M4G1C BYT3S 1S THE W4Y
```

Converting this message to lowercase and formatting it with underscores as required gives:

```
pearl{m4g1c_byt3s_1s_the_w4y}
```

### Final Flag:
```
pearl{m4g1c_byt3s_1s_the_w4y}
