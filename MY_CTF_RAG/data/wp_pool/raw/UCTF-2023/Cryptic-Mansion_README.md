# Cryptic Mansion README


# 🏰 Cryptic Mansion

🔍 Embark on a cerebral journey of geolocation and digital investigation. Crack the code hidden within the image to unveil the secrets of the Cryptic Mansion. A unique OSINT challenge awaits!

<div style="text-align:center;">
  <img src="https://git.uctf.ir/uctf/Cryptic-Mansion/raw/branch/main/ScreenShots/Output%28ScreenShot%29.PNG" alt="Cryptic Mansion">
</div>

# 🚀 Challenge Title / Description

## 🏰 Cryptic Mansion

🕵️‍♂️ Welcome, intrepid digital detectives, to the "Cryptic Mansion" challenge! Are you ready to put your OSINT skills to the ultimate test? Get ready to embark on a virtual journey that will lead you through twists and turns, all while unveiling the secrets hidden within a single image.

## Challenge Description:

Nestled within the pixels of a single image lies the enigmatic "Cryptic Mansion." Your mission, should you choose to accept it, is to reveal the well-guarded location of this elusive villa. You might be thinking, "How hard could it be to find a villa?" But this isn't just any villa—it's cloaked in mystery, designed to challenge even the most seasoned OSINT adventurers.

## The Image:

Before you stands a captivating image of a luxurious villa. The details are alluring, but don't be fooled by appearances. This image holds the key to a virtual realm that conceals the villa's true whereabouts. A single image, they say, can contain a thousand secrets, and it's your task to extract them.

## The Quest:

Your first step is to deploy reverse image search tools, carefully analyzing each pixel for traces that will lead you on this thrilling journey. You'll delve into the depths of the web, employing your skills to uncover the origin, the history, and the real-world counterpart of this captivating mansion.

## Unveiling the Flag:

🏆 Victory awaits those who successfully unveil the Cryptic Mansion's exact location. As you navigate the labyrinth of data, keep an eye out for the golden key—a distinctive link that opens the gates to Google Maps. Within this link, you'll discover the latitude and longitude coordinates that pinpoint the villa's precise location.

When you find the Google Maps link, it will have a structure like this:

https://goo.gl/maps/VMdjyjA7xs8E2E5s6

Extract the latitude and longitude from the link. For example, latitude is `37.123456789012345`, and longitude is `45.987654321098765`.

Then, use these coordinates to construct the flag in the following format:

```
UCTF{37.123456789012345, 45.987654321098765}
```

This challenge not only tests your OSINT prowess but also your geolocation skills. Are you ready to showcase your expertise in pinpointing the exact location of the Cryptic Mansion?

Now, the example flag contains random values with the exact numbers after the decimal point.

This is more than just a challenge; it's an opportunity to showcase your OSINT prowess, outsmarting the virtual world's intricate puzzles to emerge as the ultimate Cryptic Mansion conqueror.

**Are you ready to embark on a journey through pixels and pathways? Will you unravel the hidden truths of the villa's location? The Cryptic Mansion awaits your investigative finesse. Good luck, digital detectives!**

# 📝 Write Up

## Step 1: Decode the Enigmatic Image

Upon receiving the challenge, participants are presented with an intriguing image of a luxurious villa. The image, seemingly innocent at first glance, holds the key to unraveling the secrets of the "Cryptic Mansion." Participants must employ their reverse image search skills to meticulously analyze the pixels, seeking out clues and details that might lead them closer to the villa's location.

## Step 2: Unearth the Digital Trails

Armed with reverse image search tools, participants embark on a digital journey to uncover the origins and history of the villa. By employing advanced search techniques, they can trace the image back to its sources, looking for metadata, similarities, and any potential mentions across the vast expanse of the internet.

## Step 3: Decode the Google Maps Link

In their quest, participants stumble upon a critical breakthrough—a distinctive link that opens the gates to Google Maps, seemingly leading to the exact location of the Cryptic Mansion. The link has a structure similar to the following:
```
https://goo.gl/maps/PhSnXedVSiHfA3Yq9
```
However, the catch is that the last part of the link, in this case `PhSnXedVSiHfA3Yq9`, is encrypted and holds the real coordinates of the mansion.

## Step 4: Extract the Coordinates

Participants must extract the last part of the Google Maps link, treating it like a cryptographic puzzle piece. This code, a unique identifier, is the missing link between the digital realm and the real-world coordinates of the Cryptic Mansion. For instance, in the given link `PhSnXedVSiHfA3Yq9`, the unique code is `PhSnXedVSiHfA3Yq9`.

## Step 5: Construct the Flag

With the extracted unique code in hand, participants can now construct the final flag in the specified format:
```
UCTF{PhSnXedVSiHfA3Yq9}
```
This flag signifies their success in deciphering the enigma of the Cryptic Mansion's location, marking them as true digital detectives who navigated through the virtual labyrinth with precision and skill.

## Step 6: Bask in Victory

With the flag constructed, participants have triumphed over the challenge. They've showcased their OSINT prowess, deciphering pixelated clues, navigating the digital landscape, and unlocking the secrets of a seemingly ordinary image. As they submit the flag, they stand as conquerors of the Cryptic Mansion, having proven their expertise in the art of Open Source Intelligence.

---

By following these steps, participants can successfully navigate through the "Cryptic Mansion" challenge, showcasing their OSINT capabilities and emerging as skilled digital detectives. This challenge not only tests their technical skills but also their analytical thinking and attention to detail, making it a memorable and engaging experience within the realm of Capture The Flag (CTF) competitions.



# ⚙️ Challenge Creation Tools

Here are the tools I used to create this challenge:

- 🔍 [metapixel](https://linux.die.net/man/1/metapixel)
- 🔮 [MetaPixel](https://www.complang.tuwien.ac.at/schani/metapixel/)
- 🔍 [TinEye](https://tineye.com/)
- 🕵️‍♂️ [ReverseSearch](https://www.revesesearch.com/)
- 🕵️‍♂️ [Pixsy](https://www.pixsy.com/)
- 🌐 [Google Images](https://images.google.com/)
- 🔍 [Bing Visual Search](https://www.bing.com/visualsearch)
- 📷 [Yahoo Image Search](https://images.search.yahoo.com/)
- 🔍 [ReverseImage](https://reverseimage.net/)
- 🔍 [Yandex Images](https://yandex.com/images?)

Additional Resources:
- 📍 [Exact Location](https://goo.gl/maps/PhSnXedVSiHfA3Yq9)
- 🏰 [Ashraf and Behrouz Vossoughi's Villa in the Urmia Lake](http://womendresses21.blogspot.com/2013/10/ashraf-and-behrouz-vossoughis-villa-in.html)

Feel free to reach out if you have any questions or need further details!

# 🚩 Flag

UCTF{37.496805716848954, 45.63767444207702}

# 📚 Categories

Check the categories which the challenge belongs to.

- [ ] Web
- [ ] Reverse
- [ ] PWN
- [ ] Misc
- [ ] Forensics
- [ ] Cryptography
- [x] Steganography
- [x] OSINT

# 🧮 Points

| Warm up | This Challenge  | Evil |
| ------- |:---------------:| ----:|
| 25      | 350 | 500  |

# 📚 Resources
Participants are exclusively granted access to the following designated file: "Output (7MB Version).jpg". Furthermore, it is imperative to alter the nomenclature assigned to these files in accordance with the competition guidelines.