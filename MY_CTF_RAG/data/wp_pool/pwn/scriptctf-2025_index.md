# scriptctf-2025_index

# Index | NoobMaster

- Description: I literally hand you the flag, just exploit it already!

index.zip: /attachments/index.zip

# Write-up

You can see a secret option `case 1337` to load the flag. After doing so, the flag is stored in a global variable, allowing you to read beyond the array because no bound checks. Read index `8` to get the flag!

# Flag - scriptCTF{4rr4y_00B_unl0ck3d}


