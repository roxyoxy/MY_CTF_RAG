# scriptctf 2025 pdf

# pdf | Connor Chang

Description: so sad cause no flag in pdf :(

challenge.pdf:attachments/challenge.pdf

# Writeup

Running strings on the pdf file gives us a hint to look between stream and endstream. Using a tool like pdf-parser.py will let us extract the contents within the streams. Looking at obj 0 4, we see that /Length 48 /Filter /FlateDecode, which means that the content is 48 bytes long, which is fairly typical for a flag. We also know that it is compressed using Flate (zip) compression algorithm. Using zlib, we can extract the flag using the commands `python3 pdf-parser.py -o 4 -d stream.bin challenge.pdf && python3 -c "import zlib; print(zlib.decompress(open('stream.bin','rb').read()).decode())"`.

# Flag - scriptCTF{pdf_s7r34m5_0v3r_7w17ch_5tr34ms}
