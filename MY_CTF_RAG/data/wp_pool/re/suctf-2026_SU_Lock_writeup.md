# suctf 2026 SU Lock writeup

## Lock
拿到的附件是一个inno setup安装包，使用[innounp](https://github.com/jrathlev/InnoUnpacker-Windows-GUI/releases/tag/iu_2_2_4)可以查看安装包中包含的文件，但是尝试提取会显示需要密码
![](.\img\1.png)



Everything.exe是官网的安装包 ，分析Locksetup.exe就行，如何拿到Locksetup.exe文件，预期是运行之后使用取证工具进行提取
![](.\img\2.png)

Locksetup.exe是一个rust程序，用ida就能分析


![](.\img\3.png)

存在一个很明显的反调，可以patch也可以暂时不用管，根据题目描述“需要开启测试模式”可以猜测加载了驱动，由于没有隐藏api的调用，可以根据api调用流快速分析
![](.\img\4.png)

很明显的进程注入（测试时使用ai进行分析，看会不会被秒，发现目标进程都被分析出来，~~ai还是太强了~~
![](.\img\5.png)

分析得到注入的数据是通过rc4解密之后的exe文件，key为`SUCTF2026`
![](.\img\6.png)

这个文件可以通过手动解密，也可以在patch 反调之后调试，自解密然后dump



通过api分析可以发现这里进行了驱动的注册和加载
![](.\img\7.png)

反推寻找驱动的数据，然后将其dump
![](.\img\8.png)

与进程注入部分采用的算法相同，同样的方法解密即可



注入进程的exe文件
![](.\img\9.png)

很明显的锁屏逻辑
![](.\img\10.png)

加密算法采用的是xxtea的变种，Delta、Key[0~3]是来自驱动的，通过加密之后，将密文传回驱动进行验证
![](.\img\11.png)



驱动分析
![](.\img\12.png)

根据不同的IOCTL传入不同的数据进入sub_140001568

IOCTL 0x222004： Key 、Delta（也可以动调exe文件获得

IOCTL 0x222008：密文


![](.\img\13.png)

关键的判断函数采用了VM



exp：

```python
import struct


g_VmCode_GetSecrets = bytes([
    0x10, 0x00, 0x8E, 0x6A, 0x37, 0x9E, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x10, 0x00, 0xEF, 0xBE, 0xAD, 0xDE, 0x10, 0x01, 0x04, 0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x10, 0x00, 0xBE, 0xBA, 0xFE, 0xCA, 0x10, 0x01, 0x08, 0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x10, 0x00, 0xDE, 0xC0, 0x37, 0x13, 0x10, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x10, 0x00, 0x0D, 0xF0, 0xAD, 0x0B, 0x10, 0x01, 0x10, 0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0xFF
])

g_VmCode_Verify = bytes([
    0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0xB1, 0xE7, 0xA1, 0x8D, 0x40, 0x00, 0x02, 0x10, 0x01, 0x04, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0xE5, 0x32, 0xA4, 0xCA, 0x40, 0x00, 0x02, 0x10, 0x01, 0x08, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0xBC, 0x27, 0xEC, 0x6E, 0x40, 0x00, 0x02, 0x10, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0x53, 0x2B, 0xC1, 0xEF, 0x40, 0x00, 0x02, 0x10, 0x01, 0x10, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0xC2, 0x05, 0x75, 0xFA, 0x40, 0x00, 0x02, 0x10, 0x01, 0x14, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0xA6, 0x88, 0xAC, 0x54, 0x40, 0x00, 0x02, 0x10, 0x01, 0x18, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0x99, 0xAD, 0x96, 0x2F, 0x40, 0x00, 0x02, 0x10, 0x01, 0x1C, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0x15, 0x1A, 0x74, 0x77, 0x40, 0x00, 0x02, 0x10, 0x01, 0x20, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0xC1, 0x73, 0x86, 0x3E, 0x40, 0x00, 0x02, 0x10, 0x01, 0x24, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x10, 0x02, 0x82, 0xF2, 0xB9, 0xC2, 0x40, 0x00, 0x02, 0xFF
])


def parse_vm_bytecode(bytecode, is_secret_extraction=False):
    IP = 0
    R = [0, 0, 0, 0]
    output_buffer = {}
    expected_cipher = []
    while IP < len(bytecode):
        opcode = bytecode[IP]
        IP += 1

        if opcode == 0x10:      # OP_MOV_R_IMM32
            reg = bytecode[IP]
            val = struct.unpack('<I', bytecode[IP+1:IP+5])[0]
            IP += 5
            R[reg] = val
            print(f"    0x10: MOV R{reg}, 0x{val:08X}")

            if not is_secret_extraction and reg == 2:
                expected_cipher.append(val)

        elif opcode == 0x20:    # OP_STORE_OUT
            regAddr = bytecode[IP]
            regVal = bytecode[IP+1]
            IP += 2
            offset = R[regAddr]
            output_buffer[offset] = R[regVal]
            print(f"    0x20: STORE_OUT [R{regAddr}], R{regVal}  =>  Output[{offset}] = 0x{R[regVal]:08X}")

        elif opcode == 0x30:    # OP_LOAD_IN
            regDst = bytecode[IP]
            regAddr = bytecode[IP+1]
            IP += 2
            print(f"    0x30: LOAD_IN R{regDst}, [R{regAddr}]")

        elif opcode == 0x40:    # OP_ASSERT_EQ
            reg1 = bytecode[IP]
            reg2 = bytecode[IP+1]
            IP += 2
            print(f"    0x40: ASSERT_EQ R{reg1}, R{reg2}")

        elif opcode == 0xFF:    # OP_HALT
            print("    0xFF: HALT")
            break
        else:
            print(f"[!] 未知操作码: {opcode:02X}")
            break

    if is_secret_extraction:
        delta = output_buffer.get(0, 0)
        key = [output_buffer.get(4 + i*4, 0) for i in range(4)]
        return delta, key
    else:
        return expected_cipher


def btea_decrypt_variant(v, key, delta):
    n = len(v)
    if n <= 1:
        return v
    
    rounds = 6 + 52 // n
    sum_val = (rounds * delta) & 0xFFFFFFFF
    
    y = v[0]
    
    for _ in range(rounds):
        e = (sum_val >> 2) & 3
        for p in range(n - 1, 0, -1):
            z = v[p - 1]
            part1 = ((z >> 4) ^ ((y << 3) & 0xFFFFFFFF)) & 0xFFFFFFFF
            part2 = ((y >> 2) ^ ((z << 5) & 0xFFFFFFFF)) & 0xFFFFFFFF
            part3 = ((sum_val ^ y) + (key[(p & 3) ^ e] ^ z)) & 0xFFFFFFFF
            
            mx = (part1 + part2) ^ part3
            v[p] = (v[p] - mx) & 0xFFFFFFFF
            y = v[p]
            
        p = 0
        z = v[n - 1]
        part1 = ((z >> 4) ^ ((y << 3) & 0xFFFFFFFF)) & 0xFFFFFFFF
        part2 = ((y >> 2) ^ ((z << 5) & 0xFFFFFFFF)) & 0xFFFFFFFF
        part3 = ((sum_val ^ y) + (key[(p & 3) ^ e] ^ z)) & 0xFFFFFFFF
        
        mx = (part1 + part2) ^ part3
        v[0] = (v[0] - mx) & 0xFFFFFFFF
        y = v[0]
        sum_val = (sum_val - delta) & 0xFFFFFFFF
        
    return v



def solve():
    delta, key = parse_vm_bytecode(g_VmCode_GetSecrets, is_secret_extraction=True)
    print(f"\nDelta : 0x{delta:08X}")
    print(f"Key   : {[hex(k) for k in key]}\n")

    expected_cipher = parse_vm_bytecode(g_VmCode_Verify, is_secret_extraction=False)
    print(f"\n密文: {[hex(c) for c in expected_cipher]}\n")

    cipher_v = list(expected_cipher)
    plain_v = btea_decrypt_variant(cipher_v, key, delta)
    flag_bytes = struct.pack('<' + 'I' * len(plain_v), *plain_v)
    
    print("字节流还原结果:", flag_bytes)
    
    try:
        flag_str = flag_bytes.decode('utf-8')
        print(f"\nFlag: {flag_str}")
    except Exception as e:
        print("\n[!] 字符串解码失败", e)

if __name__ == '__main__':
    solve()

#     0x10: MOV R0, 0x9E376A8E
#     0x10: MOV R1, 0x00000000
#     0x20: STORE_OUT [R1], R0  =>  Output[0] = 0x9E376A8E
#     0x10: MOV R0, 0xDEADBEEF
#     0x10: MOV R1, 0x00000004
#     0x20: STORE_OUT [R1], R0  =>  Output[4] = 0xDEADBEEF
#     0x10: MOV R0, 0xCAFEBABE
#     0x10: MOV R1, 0x00000008
#     0x20: STORE_OUT [R1], R0  =>  Output[8] = 0xCAFEBABE
#     0x10: MOV R0, 0x1337C0DE
#     0x10: MOV R1, 0x0000000C
#     0x20: STORE_OUT [R1], R0  =>  Output[12] = 0x1337C0DE
#     0x10: MOV R0, 0x0BADF00D
#     0x10: MOV R1, 0x00000010
#     0x20: STORE_OUT [R1], R0  =>  Output[16] = 0x0BADF00D
#     0xFF: HALT

# Delta : 0x9E376A8E
# Key   : ['0xdeadbeef', '0xcafebabe', '0x1337c0de', '0xbadf00d']

#     0x10: MOV R1, 0x00000000
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0x8DA1E7B1
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x00000004
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0xCAA432E5
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x00000008
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0x6EEC27BC
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x0000000C
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0xEFC12B53
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x00000010
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0xFA7505C2
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x00000014
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0x54AC88A6
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x00000018
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0x2F96AD99
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x0000001C
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0x77741A15
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x00000020
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0x3E8673C1
#     0x40: ASSERT_EQ R0, R2
#     0x10: MOV R1, 0x00000024
#     0x30: LOAD_IN R0, [R1]
#     0x10: MOV R2, 0xC2B9F282
#     0x40: ASSERT_EQ R0, R2
#     0xFF: HALT

# 密文: ['0x8da1e7b1', '0xcaa432e5', '0x6eec27bc', '0xefc12b53', '0xfa7505c2', '0x54ac88a6', '0x2f96ad99', '0x77741a15', '0x3e8673c1', '0xc2b9f282']

# 字节流还原结果: b'SUCTF{SJCMA23-AX8MQ3IU-8UHCSO90-QCM1S0L}'
```

