# CrackPNG wp

# CrackPNG_wp
---

<!-- page 1 -->

MISC-CrackPNG 
知识点：PNG 文件结构 
题目难度：简单（四种等级：签到、简单，中等，较难） 
建议分值：100 分 
题目说明：看日志！提交flag 格式：flag{xxxx}。 
Flag：flag{a8ee4617ba620cc4455f9903c0361822} 
解题过程： 
判断图片宽高被修改，导致图片无法正常显示。使用010editor打开图片，抽取其中的CRC32： 
 
用python 编写脚本，爆破正确的宽高： 
import zlib 
import struct 
 
filename = "pic.png" 
with open(filename, 'rb') as f: 
    all_b = f.read() 
    data = bytearray(all_b[12:29]) 
    n = 4095 
    for w in range(n): 
        width = bytearray(struct.pack('>i', w)) 
        for h in range(n): 
            height = bytearray(struct.pack('>i', h)) 
            for x in range(4): 
                data[x+4] = width[x] 
                data[x+8] = height[x] 
            crc32result = zlib.crc32(data) 
            if crc32result == 0xE581835: 
                print("宽为：", end = '') 
                print(width, end = ' ') 
                print(int.from_bytes(width, byteorder='big')) 
                print("高为：", end = '') 
                print(height, end = ' ') 
                print(int.from_bytes(height, byteorder='big')) 
分析得到正确宽度为1027，高度为800。修改为正确宽高后打开图片，即得到flag。 
 
 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
