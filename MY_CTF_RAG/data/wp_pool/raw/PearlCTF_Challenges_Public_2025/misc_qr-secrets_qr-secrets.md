# misc_qr-secrets_qr-secrets

# QR Secrets Writeup  

## Challenge Overview  
The challenge requires us to reconstruct a broken QR code by understanding the missing components and adding them back.  

## Understanding the Issue  
A standard QR code consists of various structural components, including:  

1. **Finder Patterns** – The three large square patterns located at three corners of the QR code.  

2. **Alignment Patterns** – Small squares that help with distortion correction, especially for larger QR codes.  

3. **Format Information** – Encodes error correction levels and mask patterns, appearing in two locations.  

In this challenge, the QR code provided was missing:  

- The finder patterns and alignment patterns.  
- One set of format information (QR codes contain two identical sets for redundancy, but one was missing).  

## Solution Approach  

1. **Analyze the QR Code:** By inspecting the QR, it was evident that essential parts were missing.  
2. **Use Qrazy Box:**  
   - Qrazy Box is a website that allows users to draw and recover QR codes.  
   - Its "Help" section provided insights into QR structure.  
3. **Reconstruct the QR:**  
   - Manually redraw the missing finder patterns at three corners.  
   - Add the alignment pattern in the appropriate position.  
   - Restore the missing format information to match the existing one.  
4. **Verify & Scan:** After making the necessary corrections, the QR code became readable, revealing the flag.  
