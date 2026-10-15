# challenges misc qsecure by rejection private Description

I tried to add a simple `qqemu.Pid` packet to QEMU's gdbstub to expose the process's PID via `getpid()`.
[The patch](https://patchwork.kernel.org/project/qemu-devel/patch/20250404102603.59936-1-dominik.b.czarnota@gmail.com/) got rejected — apparently, it was an "information leak"

But… are they really sure the gdbstub doesn’t already allow far worse things?

Author: [patryk4815](https://github.com/patryk4815)

```
pwndbg -ex 'target remote qsecure-by-rejection.nc.jctf.pro:1234'
```
> NOTE1: Every connection spawns a fresh instance.
> NOTE2: Remote instance don't have internet access.