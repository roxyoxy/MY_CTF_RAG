# reverse what in ternation challenge README

# Building Instructions

Dependencies:

- Yosys
- networkx

Generating a gate network from the verilog:

```
$ yosys ./flag_checker.ys
```

Generating C++ AVX512 intrinsics from the network:

```
$ python3 verilog_to_ternlog.py ./program.json > program.cpp
```

Then, adjust `main.cpp` as needed and compile:

```
$ gcc -O3 -march=native -g main.cpp -o what-in-ternation`
```
