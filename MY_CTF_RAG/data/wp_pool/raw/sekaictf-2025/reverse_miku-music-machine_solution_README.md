# reverse miku music machine solution README

# Escape

This challenge is a variant of the classic maze reversing challenge, where the flag represents an encoding of the moves done within a maze. A simple XOR is applied to the flag input to ensure that a flag can be chosen after-the-fact, instead of having to design a maze where the actual flag is also legal moves.

The twist here is that the challenges uses Microsoft's [eXtended Flow Guard (XFG)](https://connormcgarr.github.io/examining-xfg/) compiler mitigations as a way to represent walkability. When enabled, XFG replaces all indirect calls with an indirection that asserts that the target of the call is a valid function, and that the hashed type signature of that function is the same as the call site. XFG is designed to protect against rop chains and other exploitation techniques, by drastically reducing the amount of useful call gadgets within the binary.

The position of the user is represented as a simple index into a square array of function pointers. When performing a move, the position of the user is updated, and then the `cells[cur]` function is invoked. Since this is an indirect function call, XFG comes into play.

Normally, the call would go through as-is. After all, each of the `cell<N>` functions in the `cells` array is a normal function known to the compiler. However, this binary specifically has had a post-processing step applied to it that removed certain cells from the CFG/XFG list. This means that those functions are no longer a valid control-flow target, meaning that CFG/XFG mitigations kick in and the process is killed.

In other words, only the functions that represent passable cells are listed in the CFG/XFG table. If you attempt to move into a wall, the XFG jump stub will immediately kill the process with a [STATUS_STACK_BUFFER_OVERRUN](https://devblogs.microsoft.com/oldnewthing/20190108-00/?p=100655).

Since just moving through a maze would have been too easy (and potentially vulnerable to a per-character brute-forcing attack), the maze additionally has "switches" that toggle "gates". Switches are simply `cell<N>` functions that, instead of doing nothing, will adjust the XFG hash placed before  other cell functions. This will cause them to become callable (if the new hash is correct), or no longer callable (if the new hash is incorrect). This allows us to dynamically control the state of walls at runtime.

The approach to solving this challenge is straightforward once you recognize the fact that the program encodes a maze, and that CFG/XFG is used for enforcing walls.

First, you want to extract the maze structure. This can be done by extracting all the cell functions from the `cells` array, then dumping the XFG/CFG table (for example, through `dumpbin /loadconfig miku-music-machine.exe`). If a cell is not in the table, it must always be a wall (as the table cannot be changed at runtime).

Next, you must identify the switches. This can be done straightforwardly by simply finding all `cell<N>` functions that contain an `xor` instruction instead of a sequence of 7 `nop`s. The functions are intentionally designed to have consistent byte patterns such that switches can be easily found and parsed.

If you identify the switches, you can also identify the doors that they target by parsing their contents. Switches always contain a `xor byte ptr [rip+offset], n`, targeting the lowest byte of the XFG hash of a different cell. By figuring out which cell, you can find out which door is by the switch.

If you succeed in dumping, you should obtain a maze similar to the one in [`maze.txt`](maze.txt) (spoilers!).

From here on, solving the maze becomes a simple task of finding the shortest path to the bottom-right of the maze. The maze has been intentionally designed such that there is only one shortest path to every cell, and that only one "new" switch is unlocked whenever you toggle one. This means that there is only a single valid shortest path to the goal, which visits all switches in order.

Once a path is found, encode the directions taken into a text input by encoding every move as a 2-bit value, as per the main function. Then, apply the XOR adjustment to the path to obtain the flag.
