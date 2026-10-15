# 中本聪的纸条 wp

# 中本聪的纸条_wp
---

<!-- page 1 -->

CTF区块链题目--解答
 
已知内容
 
区块链上的两笔交易在区块链测试链上可查：https://mempool.space/zh/testnet
交易ID1：f362252a9a1c9362ebd68f19d70a3e89b95c6e40408344584ddbf1396ee28766
交易1的hex是：
交易ID2：e38b077d24852ef4ec71c7aaa54c586c6cb4e4f9fe043ff644b20a54c7eade6d
交易2的hex是：
010000000147f16046a989aaefbee678b3194fcfb3989afb8605f771e31af07cd211d5c95b000000
006a47304402200e1e942f62d61cc25117d71bc2da4b523bd720dc7feec77551a0b152eb042cd702
205611099541793c7681a9f8b48364d6e7088e16afe7b7b6244c52e94d28252a3b012102572263bb
ac032e37cf96fe7664fb799f56353108c032807cc23ca557fb60b394ffffffff02d0070000000000
001976a914b1c75a61c0461cd92c124e00ee275a600aa096b288ac0000000000000000056a034354
4600000000
010000000153db4e56f159c0679818ef8ce814ce8fcaad12b854da7e582fb5f19266945f63000000
006a47304402200e1e942f62d61cc25117d71bc2da4b523bd720dc7feec77551a0b152eb042cd702
2030d7d78612b765dff96dd14fc5d723e06a8fa61b42a93410236273baf82f7f15012102572263bb
ac032e37cf96fe7664fb799f56353108c032807cc23ca557fb60b394ffffffff02d0070000000000
001976a914b1c75a61c0461cd92c124e00ee275a600aa096b288ac0000000000000000056a034354
4600000000
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

解题思路
 
观察上面两笔交易会发现两个签名的随机数发生碰撞。
ECDSA签名算法
 
椭圆曲线的基准点
，
为公钥，
为私钥
椭圆曲线的阶为
椭圆曲线的计算在
内
签名算法：签名消息
其中：
为椭圆曲线上的点
上面是标准的ecdsa签名算法
破解密钥（随机数发生碰撞时）
 
如果对两个消息
的签名时生成的随机数发生碰撞时则私钥
泄漏。具体计算如下：
随机数发生碰撞，相同那么签名的值相同
对消息
的签名
对消息
的签名
通过两个算式相除约去
并整理可以得到私钥
：
上面两笔交易的签名k值发生了碰撞，利用上面公式可以计算出私钥。（维基百科上也有一个方法时先计
算出来k值，然后计算sk）
计算出来的密钥：
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

F41AA419CB6BD43F322D403F40728CE9784CD0B465F409322A76A3DF0A984A29
获取flag
 
利用AES-CBC 进行加解密
aes的密钥和初始向量
key：F41AA419CB6BD43F322D403F40728CE9
iv：784CD0B465F409322A76A3DF0A984A29
对flag进行加密后的密文："ciphertext": "4w/VLHqPZi/epoOGvjoY9TZWhDtYpL3iLsUTyvzghJM="
解密后得到flag.
 flag is{987521gglox65s}
 
import json
from base64 import b64encode, b64decode
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
#消息
data = b"flag is{987521gglox65s}"
print(data)
#设置私钥
key_str = 'F41AA419CB6BD43F322D403F40728CE9'
key = bytes.fromhex(key_str)
print (key)
#设置iv
iv_str = '784CD0B465F409322A76A3DF0A984A29'
iv = bytes.fromhex(iv_str)
print(iv)
#加密
cipher = AES.new(key, AES.MODE_CBC, iv=iv)
ct_bytes = cipher.encrypt(pad(data, AES.block_size))
iv = b64encode(cipher.iv).decode('utf-8')
ct = b64encode(ct_bytes).decode('utf-8')
result = json.dumps({'iv':iv, 'ciphertext':ct})
print(result)
#解密
b64 = json.loads(result)
iv = b64decode(b64['iv'])
print (iv)
ct = b64decode(b64['ciphertext'])
cipher = AES.new(key, AES.MODE_CBC, iv=iv)
ct = cipher.decrypt(ct)
pt = unpad(ct, AES.block_size)
print(pt.decode())
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

<!-- page 4 -->

绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
