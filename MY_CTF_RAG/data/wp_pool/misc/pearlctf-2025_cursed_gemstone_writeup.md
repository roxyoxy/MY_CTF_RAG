# pearlctf-2025_cursed_gemstone__writeup

# Writeup : Cursed Gemstone 

- **Category** : misc
- **Points** : 200
- **Author** : ethereum

## Overview

```
Ali Baba and the Vanishing Treasure
The infamous Thieves have hidden their stolen treasure deep within a mystical cave.
Every Gem the thieves stole has a numerical value of importance
Among the countless shimmering gems, one of the thieves has stashed away a CURSED GEMSTONE!
This Cursed Gemstone has zero value of importance.
Your mission is to unmask the cursed gemstone before the thieves return.
You can query the magical scale with three gems at a time, which will reveal the difference between
the most and least valuable gems in your selection. Use this power wisely.
Once you're ready, you will get two final guesses to reveal the cursed gemstone.
Can you expose the thieves' secret before they disappear forever?
```

Consider only four gemstones for the time being. Without loss of generality, assume the values of these gemstones to be s<sub>1</sub>, s<sub>2</sub>, s<sub>3</sub>, s<sub>4</sub>  where  s<sub>1</sub> < s<sub>2</sub> < s<sub>3</sub> < s<sub>4</sub>  . Now examine all four possible combinations of three-stone queries we can make to the magical scale:

1. **Query 2 3 4 :** Compares the second, third, and fourth gemstones.
2. **Query 1 3 4 :** Compares the first, third, and fourth gemstones.
3. **Query 1 2 4 :** Compares the first, second, and fourth gemstones.
4. **Query 1 2 3 :** Compares the first, second, and third gemstones.

The results of these queries will be :
- **First query :**  s<sub>4</sub> - s<sub>2</sub> 
- **Second query :**  s<sub>4</sub> - s<sub>1</sub> 
- **Third query :**  s<sub>4</sub> - s<sub>1</sub> 
- **Fourth query :**  s<sub>3</sub> - s<sub>1</sub> 

Observe that the queries where the gemstones not included are neither the minimum nor the maximum give same result ( s<sub>4</sub>- s<sub>1</sub> ) , and can never yield the correct answer. This generalized case is obviously still applicable when  s<sub>1</sub> = 0 .

### Strategy
1.  We consider groups of four gemstones at a time.
2. For each group, we eliminate two gemstones as impossible answers based on the query results then consider next two gemstones. When only three elements remain, we reintroduce one previously eliminated gemstone to continue the process.
3. This will ensure that at the end of this process when only two gemstones remain, one of them is guaranteed to be the CURSED GEMSTONE, as we need to guess two gemstones in the end.

If `n` is even ( `n` being the number of gemstones ) we have made `(n-2)/2 * 4 = 2n - 4` queries else we have made `(n-3)/2 * 4  +  4 = 2n - 2` queries . Observing the netcat prompt it requires the player to make `2n - 2` queries . So if  `n` is even, we make two random queries at the end ( which won't affect our answer ) .

A solution script implementing the same is : -

```python 
import sys
from pwn import *

def query(x, conn):
    conn.recvuntil(b">>>")
    conn.sendline(f'{x[0] + 1} {x[1] + 1} {x[2] + 1}'.encode())
    sys.stdout.flush()
    i = int(conn.recvline().decode())
    return i

def main():
    conn = remote("cursed-gemstone.ctf.pearlctf.in", 30010)
    context.log_level = "DEBUG"
    conn.recvuntil(b"forever?\n\n\n")
    n = int(conn.recvline().decode().split()[3])

    guess = [0, 1]
    for i in range(2, n - 1, 2):
        win = []
        curr = [guess[0], guess[1], i, i + 1]
        for j in range(4):
            x = curr[:j] + curr[j+1:]
            win.append((query(x,conn), curr[j]))
        win.sort()
        guess = [win[0][1], win[1][1]]
    
    if n % 2 == 1:
        lef = 0

        while lef in guess:
            lef += 1
        win = []
        curr = [guess[0], guess[1], n - 1, lef]
        for j in range(4):
            x = curr[:j] + curr[j+1:]
            win.append((query(x, conn), curr[j]))
        win.sort()
        guess = [win[0][1], win[1][1]]

#two random queries before guessing if n is even 

    if n % 2 == 0:
        query(x, conn)
        query(x, conn)
    
    conn.sendline(f'{guess[0] + 1} {guess[1] + 1}'.encode())
    conn.interactive()

if __name__ == '__main__':
    main()
```
We get the flag `pearl{al1baba_4nd_h1s_d4mn_th13v3s}` : )

