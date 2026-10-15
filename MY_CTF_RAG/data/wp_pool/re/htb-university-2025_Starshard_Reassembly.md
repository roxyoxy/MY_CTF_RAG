# htb-university-2025_Starshard Reassembly

<img src="../../assets/banner.png" style="zoom: 80%;" align=center />

<img src="../../assets/htb.png" style="zoom: 80%;" align='left' /><font size="5">Starshard Reassembly</font>

  16<sup>th</sup> 12 2025

  Prepared By: kailash

  Challenge Author: kailash

  Difficulty: <font color=green>Easy</font>

  Classification: Official






# Synopsis

Starshard Reassembly is an Easy Reversing challenge. Players will analyze a Go binary that uses interface-based dispatch to verify a flag.

## Skills Required
    - Basic reversing knowledge
    - Use of a decompiler
## Skills Learned
    - Understanding Go interfaces
    - Reconstructing scrambled flags

# Solution

# Enumeration

## Analyzing the code

When we unzip the challenge archive, we are **not** provided with any source code - only a single file:

- `memory_minder` - a **Mach-O 64‑bit executable** compiled for **macOS (x86_64)**.

This already tells us something important:
- If we try to run it on Linux, we immediately get:
	```bash
    exec format error
    ```
    
    which means that anyone using Linux must rely on a tool such as `lipo`, `otool`, or a macOS VM/emulator to execute or debug the binary.

Because the challenge gives us only a Mach‑O binary and no actual Go source code, we must treat this as a **black‑box RE challenge** and analyze the binary using tools like **Ghidra**.

When running the binary, we see:

```
⛄ Twillie Snowdrop's Memory-Minder
The snowglobe clouds... Enter the scrambled memory to restore the truth.
>
```

`strings` reveals very little - just runtime symbols and Go garbage collector logs, which is typical for stripped Go binaries. This means we are dealing with **Go’s unusual binary layout**, where:

- Functions are not placed in neat `.text` blocks like C/C++.
- Metadata such as **itab**, **type descriptors**, and **interface method tables** appear instead of clean function pointers.
- Literal strings (like the flag) usually **do NOT appear** contiguously in the binary.

## Inspecting `main.main` in Ghidra

Before diving into the per-byte methods, it is important to understand **how the program is wired together**. The `main.main` function is extremely large and noisy due to Go’s runtime, but it contains the **actual control flow** of the challenge.

Below is a shortened excerpt of the decompiler output:

```main pseudo code
/* Many unreachable blocks removed by Ghidra */
void main.main(void)
{
    internal/abi.ITab *piVar1;
    int cap;
    uint8 *extraout_RDX;
    undefined **len;
    int iVar2;
    internal/abi.Type *et;
    ...
    while (local_338 <= *(undefined1 **)(unaff_R14 + 0x10)) {
        runtime.morestack_noctxt();
    }

    // The important part begins here:
    local_1d8 = &_go:itab.main.R0,main.MemoryRune;
    local_1c8 = &_go:itab.main.R1,main.MemoryRune;
    local_1b8 = &_go:itab.main.R2,main.MemoryRune;
    local_1a8 = &_go:itab.main.R3,main.MemoryRune;
    ...
    local_28  = &_go:itab.main.R27,main.MemoryRune;

    // stdin reader setup
    reader := bufio.NewReader(os.Stdin)

    // print banner
    fmt.Println(...)
    fmt.Print("> ")

    // read user input
    input, err := reader.ReadString('
')

    // check length against len(shards)
    if (len(input) != len(shards)) error

    // byte-by-byte validation using Match()
    for i, shard := range shards {
        if !shard.Match( input[i] ) {
            print("incorrect!")
            return
        }
    }

    // reconstruct output using Expected()
    out := make([]byte, 0, len(shards))
    for _, shard := range shards {
        out = append(out, shard.Expected())
    }

    fmt.Println(string(out))
}

```

### What this tells us

- The `shards` list is constructed **directly inside** `**main.main**` by assigning 28 itab pointers (`R0` through `R27`).
- Go stores interface values as `(itab pointer, data pointer)`, so the `itab` is what we must inspect to find methods.
- `Match()` determines the scrambled input.
- `Expected()` contains one returned flag byte.
- The decompiler output is misleading because Go injects stack guards, panic wrappers, and internal ABI logic. **The real logic exists in the assembly, not in the decompiled C-like view.**

### How ITables connect everything

Each of the following entries:

```pseudo code func names
_go:itab.main.R0,main.MemoryRune
_go:itab.main.R1,main.MemoryRune
_go:itab.main.R2,main.MemoryRune
...
_go:itab.main.R27,main.MemoryRune
```

represents **one interface object** implementing the two required methods:

- `Match(b byte) bool`
- `Expected() byte`

By examining each itab in Ghidra, we find pointers to their method wrappers. We then follow those pointers into the executable section to retrieve the real logic.

This is why the `main.main` function is crucial: it tells us the **exact order** of R0→R27, which is the order of the flag bytes.


### Understanding the objective

Since we have no source code, our job is to reconstruct what the binary does. Through analysis (Ghidra → function discovery → navigating Go runtime structures), we identify:

- The binary expects a **scrambled string** of fixed length.
- Each character is checked via a `Match()` method implemented by different Go types.
- If all characters match, the program dynamically assembles the **flag bytes** using `Expected()` values.
- These bytes are returned **one per function**, meaning the flag never exists contiguously in `.rodata`.

So the goal becomes:

- Discover the mapping between each `Match()` byte and its corresponding `Expected()` byte.
- Rebuild the correct flag in the right order.

### Workflow summary so far

1. We have a single Mach‑O Go binary, no source.
2. Running on Linux fails due to `exec format error`.
3. Strings give us nothing meaningful.
4. Ghidra shows Go‑specific artefacts: `itab…`, interface tables, GC metadata.
5. The challenge logic is hidden inside 28+ small functions named like `main.(*R14).Match` and `main.(*R14).Expected`.
6. The flag is assembled at runtime, one byte per Expected() call.

Here’s our "initial findings" summary:

1. The binary is Go (Mach‑O), so typical C RE patterns don’t apply.
2. Go interface dispatch is used (itab + method tables).
3. The scrambled input is validated byte‑by‑byte.
4. The flag bytes are stored _only as immediates_ inside `Expected()` methods.
5. Nothing in the binary contains the full flag string.

## Understanding Go Binaries and ITabs

Go binaries differ significantly from C/C++ binaries. Instead of simple function pointers, Go uses **interfaces** backed by **ITabs** (interface tables). Each interface value contains two things: a pointer to the concrete value and an ITab for method resolution.

### What an ITab Contains

- Pointer to type information
- Pointers to method wrappers (e.g., `Match`, `Expected`)
- Used at runtime to resolve interface method calls

### Why This Matters for Reversing

- Decompiler output often hides the real logic because Go wraps functions in stack-check and panic code.
- The **real** return value (like the flag bytes) is stored as a tiny instruction (`MOV EAX, imm8`) in the method body.
- You must follow the ITab → method pointer → disassembly to extract these bytes.

### Difference vs. C/C++ Binaries

- C/C++ binaries contain direct calls and fewer layers.
- Go binaries include huge runtime metadata: GC tables, type descriptors, panic handlers.
- Strings rarely appear contiguously; Go assembles them at runtime.

Understanding ITabs is the key to solving this challenge because all flag bytes are hidden behind interface resolution.

# Solution

## Finding the vulnerability

There is no memory corruption or classical vulnerability here - instead, the "vulnerability" is **reverse‑engineering the logic** used by Go’s interface dispatch.

Specifically, the binary:

1. Loads a slice of interface objects (`[]MemoryRune`).
2. Each object has two methods: `Match()` and `Expected()`.
3. `Match()` checks whether the user input byte equals a single hard‑coded scrambled byte.
4. `Expected()` returns the _real_ flag byte (encoded as an immediate).
5. If all `Match()` calls succeed, the binary loops through the objects again and calls `Expected()` for each - reconstructing the entire flag.

So the "vulnerability" is understanding that:

- We do **not** need to execute the program.
- We simply need to statically analyze each `Expected()` function and harvest its return value.

This requires understanding Go’s layout:

- The real return value is not visible in the decompiler because Go’s ABI uses special wrappers.
- We must inspect **the assembly**, not the decompiler output.
- Every Expected() function contains a `MOV EAX, imm8` (or similar) right before returning.

This is the actual flag byte.

Once all bytes are collected in order, we reconstruct the flag.

## Exploitation (!)

Our "attack" consists of these steps:

1. Identify each `main.(*Rxx).Match` and `main.(*Rxx).Expected` function using Ghidra.
2. Ignore the decompiler output - it is misleading due to Go’s stack guard wrappers.
3. Switch to the disassembly view.
4. Locate the `MOV EAX, <byte>` inside each `Expected()` function.
5. Extract all bytes in order R0 → R27.
6. Concatenate them to form the final flag.

## Searching for `Expected()` in Ghidra

Using **Search → Memory**, we locate references to strings like:
```assembly
"main.(*R0).Expected"
```

result:
```
Location: 011110e4   ds "main.(*R0).Expected"
Location: 0111c9bed  ds "_main.(*R0).Expected"
```

This is just metadata , we must **follow cross‑references** into the real code.

## The misleading decompiler view

When opening `main.(*R0).Expected`, Ghidra shows something like:

```pseudo code
void main.(*R0).Expected(main.R0 *p0, uint8 r0)
{
    long *plVar1;
    long in_RAX;
    long extraout_RAX;
    long unaff_R14;

    plVar1 = *(long **)(unaff_R14 + 0x20);
    if (plVar1 != 0) goto LAB_010a721f;
    while (in_RAX == 0) {
        runtime.panicwrap();
        in_RAX = extraout_RAX;
    LAB_010a721f:
        if (*plVar1 == &stack0x00000008) {
            *plVar1 = &stack0xfffffffffffffff8;
        }
    }
    return;
}
```
This appears to have **no return value**, but that is **false**.

Go wraps almost all interface methods in panic/stack‑guard wrappers. The _real_ logic is elsewhere.

## The real Expected() logic - hidden in the assembly

Switch to **Listing (Disassembly)**.
Inside the Expected() wrapper you will find lines like:

```assembly
        010a7212 b8 48 00        MOV        EAX,0x48
                 00 00
```

## Getting the flag

To solve the challenge, we must extract the contents of each `Expected()` function and read the byte used. We can then combine them into a byte array to get the flag.
