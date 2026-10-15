# scriptctf-2025_diskchal

# diskchal | Connor Chang

Description: i accidentally vanished my flag, can u find it for me

stick.img:attachments/stick.img

# Writeup

In FAT32, when a file is deleted, its directory entry remains in the root directory, but the first byte is overwritten, usually with 0xE5, and marked as free. If the file hasn't been overwritten, it's easy to recover. You can solve it with the following commands:
```fls -r -f fat32 stick.img
icat -f fat32 stick.img 10 > test.gz
gzip -d test.gz
cat flag.txt
```

# Flag - scriptCTF{1_l0v3_m461c_7r1ck5}

