# scriptctf-2025_index-2

# Index-2 | NoobMaster

- Description: This time, you get the file pointer, not the flag itself.
  
index-2.zip: /attachments/index-2.zip


# Write-up

Basically, leak the file pointer, then overwrite stdin (a global variable, can be accessed via index `-6`) with the file pointer. What this does is next time input is taken from the file. This gives you the error message: `Invalid choice: <flag>`.

Solve at `solve/exploit.py`

# Flag - scriptCTF{4rr4y_OOB_l3v3l_up!}
