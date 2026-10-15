# scriptctf 2025 qr

# Off-By-One | Connor Chang

Description: i hid a qr inside a qr 

hidden.png:attachments/hidden.png

# Writeup

Scanning the qr code gives a youtube link that experienced hackers wouldn't click on. Looking at the individual pixel values, we can see that there appears to be some pixels that are off by 1. Some pixels are (0, 0, 1) or (255, 255, 254) instead of (0, 0, 0) and (255, 255, 255). Taking the pixels that are "off by one" and forming a QR code where (0,0,1) are black pixels and (255,255,254) are white pixels gives us a qr code that contains the flag.

# Flag - scriptCTF{qrqrqrc0d3s}
