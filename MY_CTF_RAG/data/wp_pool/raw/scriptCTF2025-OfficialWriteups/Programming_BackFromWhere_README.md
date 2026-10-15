# Programming_BackFromWhere_README

# Back From Where | Connor Chang

Description: On a grid, you begin on the top left, moving right and down until reaching the bottom right, multiplying every number you encounter on the path. Find the maximum number of trailing zeroes. Note: You might want to check out BackFromBrazil from n00bzctf 2024.

server.py:attachments/server.py

# Writeup

The number of trailing zeroes can be found by taking the minimum of the number of factors of two and the number of factors of fives. Given this, we can use a simple dp to solve the challenge by storing the maximum possible fives and maximum possible twoes on each state. 

# Flag - scriptCTF{d0nt_u_ju5t_!@ve_z%ro3s???}
