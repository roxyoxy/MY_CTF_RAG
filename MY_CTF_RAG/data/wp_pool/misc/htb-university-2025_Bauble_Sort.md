# htb university 2025 Bauble Sort

![](../../assets/banner.png)

<img src="../../assets/htb.png" style="margin-left: 20px; zoom: 80%;" align="left" />        <font size="10">Bauble Sort</font>

​        18<sup>th</sup> December 2025

​        Prepared By: 131LL

​        Challenge Author(s): 131LL

​        Difficulty: <font color=green>Easy</font>


# Synopsis

Bauble Sort is an easy coding challenge, where the player is required to parse structured input records and perform a multi-key sort. Baubles must be ordered by descending sparkle, then ascending stability, and finally lexicographically by identifier. Efficient sorting is required to handle large input sizes.

## Skills Required

- Parsing structured text input
- Custom sorting with multiple criteria
- Writing efficient code that scales to large input sizes

## Skills Learned

- Using built-in sorting mechanisms effectively
- Understanding how ordering priorities interact
- Handling large lists without unnecessary overhead

## Description

```
When the Starshard Bauble vanished, Tilda Baublenose discovered that the ancient Bauble Registry had been scrambled by lingering festive corruption.
Each ornament’s glow flickers out of order, threatening Everlight itself.
Only by restoring the proper sequence can Tilda coax the magic back into harmony.
```

## Technical Description

```
In the quiet hours before Everlight Eve, Tilda Baublenose uncovered a fragment of the ancient Bauble Registry — an enchanted log used to catalog every magical ornament in Tinselwick.

Each bauble is recorded with:
• its unique identifier,  
• its sparkle power,  
• and its glow stability.  

But after the Starshard Bauble vanished, the registry was left in disarray — scrambled by corrupted festive magic.

To restore the registry, the baubles must be sorted using the following enchanted rules:

1. Baubles with higher sparkle power must come first.
2. If two baubles have the same sparkle power, the one with lower stability comes first.
3. If both sparkle and stability are the same, the baubles are ordered alphabetically by their identifier.

As Tilda, you must restore the correct order of the registry so the festival systems can safely reboot. Only once the baubles are properly sorted can the magic of Everlight begin to flow again.

The input has the following format:

The first line contains a single integer N, the number of baubles.

The next N lines each contain:
A bauble identifier (string without spaces),
An integer sparkle value,
An integer stability value.

Each line follows the format: "identifier: sparkle , stability"

Print the identifiers of the baubles in the correct sorted order, one per line.

10 ≤ N ≤ 500000
0 ≤ sparkle ≤ 1000
0 ≤ stability ≤ 1000
Example:

Input:
3
starwishbell_01f106: 10 , 3
tinselflaredrop_7a0b68: 10 , 1
glimmerglowheart_a00165: 7 , 0

Expected output:
tinselflaredrop_7a0b68
starwishbell_01f106
glimmerglowheart_a00165

The baubles must be sorted using the following enchanted rules:

1. Higher sparkle comes first.
2. If sparkle is equal, lower stability comes first.
3. If both are equal, sort by identifier alphabetically.

From the example input:

- starwishbell_01f106 has sparkle 10 and stability 3  
- tinselflaredrop_7a0b68 has sparkle 10 and stability 1  
- glimmerglowheart_a00165 has sparkle 7 and stability 0  

The two baubles with sparkle 10 are starwishbell_01f106 and tinselflaredrop_7a0b68.  
Between them, tinselflaredrop_7a0b68 comes first because it has lower stability (1 < 3).  
glimmerglowheart_a00165 comes last due to its lower sparkle value (7 < 10).

The correct restored registry order is:
tinselflaredrop_7a0b68  
starwishbell_01f106  
glimmerglowheart_a00165
```

## Solving the challenge

This challenge is a multi-key sorting problem. Each bauble must be ordered according to three criteria, applied in strict priority order:

1. Sparkle power, descending
2. Stability, ascending
3. Identifier, alphabetically

The most important observation is that once all input has been parsed into a structured form, the problem is essentially solved by a single sort operation.

Python's built-in `sort()` function is stable, fast, and allows us to define an explicit key that encodes all three rules at once.

The challenge therefore splits naturally into two parts:

1. Parse each registry line into (identifier, sparkle, stability)
2. Sort the list using a creafully constructed key

Let's begin by reading the number of baubles and preparing a list to store them.

```python
N = int(input())
baubles = []
```

At this point, baubles will eventually hold one tuples per registry entry. Each tuple will contain exactly the data we need to sort later.

Next, we read and parse each registry line.

```python
for _ in range(N):
    line = input()
    name = line.split(":")[0]
    sparkle = int(line.split(": ")[1].split(",")[0])
    stability = int(line.split(": ")[1].split(",")[1])
    baubles.append((name, sparkle, stability))
```

Each input line follows the format:

```
identifier: sparkle , stability
```

We split the line at the colon to extract the identifier.
The remaining part is further split at the comma to separate sparkle and stability, both of which are converted to integers.

By the end of this loop, baubles looks conceptually like this:

```
[
  ("starwishbell_01f106", 10, 3),
  ("tinselflaredrop_7a0b68", 10, 1),
  ("glimmerglowheart_a00165", 7, 0),
  ...
]
```

The heart of the solution is the sorting step:

```python
baubles.sort(key=lambda x: (-x[1], x[2], x[0]))
```

Here, the lambda defines the exact priority rules:

* `-x[1]` sorts sparkle in descending order
* `x[2]` sorts stability in ascending order
* `x[0]` sorts identifiers alphabetically if everything else is equal

Python compares tuples from left to right, so this single key expression perfectly encodes the enchanted rules given in the problem statement.

This sort runs in O(NlogN) time and is efficient enough even for hundreds of thousands of baubles.

Finally, we print the output - the restored registry order.

```python
print("\n".join([bauble[0] for bauble in baubles]))
```