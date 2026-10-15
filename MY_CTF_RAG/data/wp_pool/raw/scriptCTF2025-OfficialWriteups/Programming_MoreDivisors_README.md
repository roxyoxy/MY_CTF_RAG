# Programming_MoreDivisors_README

# More Divisors | Connor Chang

Description: find longest subsequence with gcd > 1 :)

server.py:attachments/server.py

# Writeup

## Solution 1
We know that if the subsequence has gcd > 1, then the subsequence must contain atleast one prime factor in common. We can count the number of times each prime shows up and the maximum number of times a prime shows up will be the answer.

Time Complexity: O(n * log(max element))

## Solution 2
With n == 2 * 10^5, we can essentially garuntee that the longest subsequence with gcd > 1 will have a gcd of 2, so we can just count the number of even numbers in the list.

Time Complexity: O(n)

# Flag - scriptCTF{7H3_m0r3_f4C70r5_7h3_b3773r}
