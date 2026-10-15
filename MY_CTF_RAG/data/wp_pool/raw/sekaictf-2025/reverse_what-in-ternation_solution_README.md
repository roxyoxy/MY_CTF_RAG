# reverse what in ternation solution README

# Author Writeup

Scanning through the `program` function, which clearly does the actual checking, it becomes clear that there's effectively only a few actions being taken. The heavy lifting operations are almost all `vpternlogd` and `vpermb`/`vpermi2b`. Looking at how our input is processed, we can see that each bit of our input is expanded into a full byte, then passed to `program` as 8 registers containing 64 bits (each in their own byte). Skimming over the instructions reveals that the program only performs logic operations (using `vpternlogd`) on the bits, then rearranges and merges registers for the next `vpternlogd`. The flag is correct if the low bit of the final `xmm0` register is 1.

Based on this, we can reasonably assume that the program is an encoded set of logic gates, where `vpternlogd` acts as the logic gate instruction, and the `vpermb`/`vpermi2b` instructions act as "wires" between the gates.

To solve this, we can build a Z3 model of the circuit by emulating it in python while building up the logic chains embedded in the checker. Because there's only 6 relevant types of instructions in the checker, this is relatively straightforward. We keep track of register contents and values written to the stack, then step through all operations and symbolically perform them. Finally, we enforce that the output bit is 1 and ask Z3 for a solution.

An implementation of the above approach can be found in `solve.py`. A useful tool for figuring out the behavior of AVX512 (and other SIMD) instructions is the reference at https://www.officedaytime.com/simd512e/. This majestical site has diagrams for the behavior of all instructions.
