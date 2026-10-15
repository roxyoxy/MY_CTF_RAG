# reverse_miku-music-machine_challenge_README

## Compiling application

Activate MSVC dev environment:

```
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

Compile application:

```
cd out
cl /Zi /O2 /guard:xfg ../main.c /link /INCREMENTAL:NO /SECTION:.text,RWE
```

## Generating mazes

Install dependencies:

```
python3 -m pip install mazelib
```

Adjust maze parameters (size, number of switches/waypoints).

Run:

```
python3 mazegen.py
```

Note that you may need to run the script several times before getting a valid maze. I didn't really debug it very hard.

## Patching the binary

Ensure you have Rust installed. Cargo will grab dependencies if needed.

Construct a maze according to the following rules:

- Must be a square maze
- The outer rows/columns must be walls to avoid the user moving off-grid
- Text format: ` ` blank, `#` wall, `x` switch, `X` (capital) door opened by landing on switch

The rust application assumes all switches must be visited in alphabetical order (start at `a`, move to `b`, etc). It will automatically find the optimal path. **Ensure this is the only path**, or you will have solutions that don't produce a correct flag.

Example maze:

```
#####
#a# #
# B #
#b#c#
#####
```

Run application to patch binary:

```
cargo run --release -- -i <path to compiled maze> -m <path to text file containing maze> -t <target flag> -o <output path for patched file>
```

If successful, produces the output path as well as the xor adjustments needed for the valid path to be encoded as the given flag. You will need to insert these into the source, then recompile and re-patch.
