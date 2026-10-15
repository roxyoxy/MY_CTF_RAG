# scriptctf-2025_div-2

# Div 2 | NoobMaster

- Description: Some might call this a programming challenge...

chall.py:/attachments/chall.py

# Write-up

While guessing, the output would be `1` if `input <= secret` or `0` if `input > secret`. You can use this to binary search. If `0`, decrease the guess, or if `1`, increase the guess. If an input outputs `1`, and `input + 1`  outputs `0`, then the input is correct. Similarly, If an input outputs `0` , and `input - 1` outputs `1`, then `input - 1` is the correct input.

solve.py: /solve/solve.py

# Flag - scriptCTF{b1n4ry_s34rch_u51ng_d1v1s10n?!!}