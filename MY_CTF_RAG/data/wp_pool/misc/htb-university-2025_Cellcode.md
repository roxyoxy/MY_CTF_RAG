# htb-university-2025_Cellcode

![](../../assets/banner.png)

<img src='../../assets/htb.png' style='zoom: 80%;' align=left /> <font size='10'>Cellcode</font>

2<sup>nd</sup> October 2025

Prepared By: `6cloudguy`

Challenge Author: `6cloudguy`

Difficulty: <font color='orange'>Medium</font>

<br><br>

# Synopsis
Cellcode is a medium coding challenge, where the player has to write a simulation for cellular automaton on a N x N grid for T generations, with parallel updates. Each cell's next state depends on the count of alive neighbors, and a birth / survival rulestring that defines birth and survival conditions.

## Skills Required
- Logic simulation
- Working with 2D arrays
- Parsing strings and rules
- Problem decomposition

## Skills Learned
- Grid-based problem solving and traversal
- Implementing cellular automata with custom rules
- Managing state transitions across multiple iterations

## Description

```
Felix Brambletail’s enchanted petri dish is supposed to be a harmless cocoa-side hobby—until its living sugar-cells start blinking in the exact rhythm of the Starshard Bauble.
When the Bauble vanishes, the dish “rewinds” too, as if someone scrubbed a corrupted wish from reality.
Help Felix replay the experiment faithfully and see what pattern survives the rollback.
```

## Technical Description

```
Felix Brambletail — Tinselwick’s Hot Cocoa Coder — doesn’t just stir marshmallows. Between Everlight rehearsals, he runs tiny “life spells” on enchanted petri dishes, where living sugar-cells blink like little lanterns. This year, Felix and Tilda Baublenose noticed something eerie: the dish’s living patterns were syncing with the Starshard Bauble’s flicker logs. And at the exact moment the Bauble vanished, both records seemed to rewind — as if a corrupted line of festive magic had been rejected and rolled back. Felix needs a simulator he can trust: a precise way to evolve the dish forward in time, one generation at a time, following a spell-rule that controls when dead cells awaken and when living cells endure. Your task is to simulate the dish for T generations. Each generation updates all cells in parallel based on the number of alive neighbors (up to 8 surrounding cells inside the grid), using the provided rulestring.

Input format:
- The first line is an integer N, the size of the grid (N x N) representing Felix's petri dish.
- The next N lines each contain N characters (0 or 1) representing the initial state of the dish.
    - 0 = dead cell, 1 = alive cell
- The next line is a rulestring for birth and survival in the format "Bx/Sy" where x and y are sequences of digits (e.g: B3/S23).
- B = birth conditions (dead cell becomes alive) - B3 means a dead cell with exactly 3 alive neighbors becomes alive - S = survival conditions (alive cell stays alive) - S23 means an alive cell with 2 or 3 alive neighbors stays alive
- The last line contains an integer T, the number of generations to simulate (how many “ticks” Felix and Tilda let the experiment run).

Output format:
- Print the final grid after T generations.
- The output should be N lines each containing N characters (0 or 1) representing the final state of the grid.
- The output grid should maintain the same format as the input grid.

2 <= N <= 750
0 <= Birth/Survival condition <= 6
1 <= T <= 6

Input:
5
0111
0101
1101
0100
B25/S05
2

Expected output:
1000
0100
0010
0000

The initial grid is a 4x4 matrix with a mix of alive (1) and dead (0) cells on Felix's enchanted petri dish.
The rulestring "B25/S05" indicates that:
- A dead cell becomes alive if it has exactly 2 or 5 alive neighbors (B25).
- An alive cell survives only if it has 0 or 5 alive neighbors (S05).
- All the 8 surrounding cells are considered neighbors (up, down, left, right, and diagonals), as long as they are inside the grid.

After 1st generation:
1000
0000
0010
0000

After 2nd generation:
1000
0100
0010
0000
```

# Solution
Let us consider our approach:
First we start by reading the input
```python
N = int(input())  
grid = [list(input().strip()) for _ in range(N)]  
rule = input()  
generations = int(input())  
```
Our next step is to split and understand the rules as they are split into birth and survival conditions
```python
birth, survive = rule.split('/')
birth_nums = set(int(x) for x in birth[1:])
survive_nums = set(int(x) for x in survive[1:])
```
Example: `"B3/S23"` means:
- A dead cell will become alive if it has exactly 3 live neighbours
- A live cell with 2 or 3 live neighbours will survive
- Otherwise the cell would be dead
 Now we should parse the grid properly

```python
grid = [[int(cell) for cell in row] for row in grid]
```
Next is the simulation loop
```python
for _ in range(generations):
new_grid = [[0] * m for _ in range(n)]
...
grid = new_grid
```
- `new_grid` starts empty for each generations
- After applying all the rules, we replace the old grid with the `new_grid`
Now we start counting the neighbours for each cell `(i,j)`
```python
count = 0
for dx in [-1, 0, 1]:
    for dy in [-1, 0, 1]:
        if dx == 0 and dy == 0: continue
        ni, nj = i + dx, j + dy
        if 0 <= ni < n and 0 <= nj < m:
            count += grid[ni][nj]
```
- We loop over the relative positions (`dx`,`dy`)
- Skip the center position
- If a neighbour is 1, increase the count(this count determines the next state of the cell)
Since the neighbour count is ready, we apply the rule:
```python
if grid[i][j] == 1:  
    new_grid[i][j] = 1 if count in survive_nums else 0  
else:  
    new_grid[i][j] = 1 if count in birth_nums else 0  
```
- The state of cell is determined based on the count and birth/survival rule
After running all the generations, we return the final grid
```python
return [''.join(str(cell) for cell in row) for row in grid]
```
- Convert the numeric back to strings
- Makes the output match the input format
