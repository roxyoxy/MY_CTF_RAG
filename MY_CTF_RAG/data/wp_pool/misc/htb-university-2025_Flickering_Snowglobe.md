# htb-university-2025_Flickering Snowglobe

![](../../assets/banner.png)

<img src="../../assets/htb.png" style="margin-left: 20px; zoom: 80%;" align="left" />        <font size="10">Flickering Snowglobe</font>

​        18<sup>th</sup> December 2025

​        Prepared By: 131LL

​        Challenge Author(s): 131LL

​        Difficulty: <font color=green>Very Easy</font>


# Synopsis

Flickering Snowglobe is a very easy coding challenge that requires the player to process a long string and count the number of maximal contiguous segments of identical characters. A new segment begins whenever the current character differs from the previous one. The solution requires a single linear pass over the input.

## Skills Required

- Basic input parsing
- Looping over a string
- Comparing adjacent characters
- Handling large input efficiently (O(N))

## Skills Learned

- Writing fast linear time solutions for big inputs

## Description

```
The Great Snowglobe once pulsed with calm, steady light, guiding Everlight Eve from atop Sprucetop Tower.
Since the Starshard Bauble fled, its glow now stutters and fractures, breaking harmony into jittery bursts.
Tilda studies the flicker log, counting each break in the Snowglobe’s rhythm.
```

## Technical Description

```
Flickering Snowglobe
High above Tinselwick, the Great Snowglobe atop Sprucetop Tower is supposed to glow in smooth, gentle patterns — a steady orchestra of magical pulses that keeps the Festival of Everlight in harmony.

Each moment, the Snowglobe emits a tiny pulse:
• '.' for a short flicker  
• '-' for a long shimmer  
• '*' for a bright spark  

Normally these pulses form long, stable bands of the same type. But since the Starshard Bauble fled, the glow has begun to stutter — breaking into many short, uneven chunks.

Tilda Baublenose has captured a log of the Snowglobe’s pulses as a single string. She wants to know how many stable segments the Snowglobe’s glow has fractured into.

A stable segment is defined as a maximal contiguous sequence of the same pulse character.
For example, the sequence "..--***.-" is broken into the segments:
* ".."
* "--"
* "***"
* "."
* "-"
a total of 5 stable segments.

Your task is to analyze the Snowglobe’s flicker log and determine how many stable segments it contains.

The input has the following format:
The first line contains a single integer N, the length of the pulse string S.
The second line contains the pulse string S, which consists of the characters '.', '-', and '*'.

Print a single integer — the number of stable segments in the pulse string.

1 ≤ N ≤ 10^6
Example:

Input:
...--***.-

Expected output:
5

We are given the pulse string:

...--***.-

We break it into maximal contiguous segments of identical characters:

1. ...  (three short flickers .)  
2. --   (two long shimmers -)  
3. ***  (three bright sparks *)  
4. .    (a single short flicker .)  
5. -    (a single long shimmer -)  

There are 5 such stable segments in total, so the correct output is 5.
```

## Solving the challenge

To begin, we will parse our given input. The first line contains the length of the pulse string S, while the second line contains the pulse string itself.

```python
N = int(input())
S = input()
```

Now, we can initialize the segment counter. We initialize with the value `1`, because there is always at least one segment, since empty strings are not allowed.

```python
cnt = 1
```

Finally, we can count the transitions between adjacent pulses. We will iterate from `i = 0`, to `i = N - 2` so `S[i + 1]` is always valid. If the current pulse differs from the next pulse, a segment ends a new one begins. Each change increase the number of segments by `1`.

```python
for i in range(N - 1):
    if S[i] != S[i+1]: cnt += 1
```