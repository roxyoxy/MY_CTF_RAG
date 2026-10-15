# BlueBird wp

# BlueBird_wp
---

<!-- page 1 -->

Blue Bird Writeup
知识点：Flutter 逆向，SO 逆向
题目难度：中等
建议分值：100
题目说明：Flutter 最近真的很火啊，而且可以直接嵌入到已有项目，来试试“Flutter 赋能”的逆向题吧！提交flag 格式：32
位md5
输入：F1u77er_S4v3_Th3_W0r1d
Flag：03cd6eb78a6c70f16bfbfd3508f7065f
解题过程：
Jadx 打开APK
绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技


---

<!-- page 2 -->

按钮点击后会启动FlutterActivity，代码比较复杂，但是运行的时候能看到启动了一个新的界面
绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技


---

<!-- page 3 -->

我们可以得出：这个界面是用Flutter 写的，肯定没法从Java 层着手。
观察apk 的lib 里有如下so，并且提供了多种架构的。选自己熟悉的架构进行分析即可。
绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技


---

<!-- page 4 -->

lbmy_flutter_plugin_ffi.so 导出了一个verify(char *s1)函数。
我们尝试hook 这个函数，值得注意的是，只有第一次输入并确认之后，这个so 才会加载进内存：
这之后我们才开始hook。
// hook.ts
var mod = Process.getModuleByName('libmy_flutter_plugin_ffi.so')
var func_verify = new NativeFunction(mod.getExportByName('verify'), 'int', ['pointer'])
Interceptor.replace(
func_verify,
new NativeCallback(
(data) => {
console.log(`verify(${data}: ${data.readCString()})`)
return func_verify(data)
},
'int',
['pointer']
)
)
TypeScript
上面的脚本也就是加了个日志来打印一下verify 的参数。
我们在Flutter 界面输入点数据进行测试，以下是输入a，aa，aaa，aaaa，aaaaa 的hook 信息。
绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技


---

<!-- page 5 -->

看出来是个base64，但稍微有点不对：
可以看到，flutter 给输入的两边添加下划线后，调用so 中的verify 进行验证。
再来分析libmy_flutter_plugin_ffi.so：
main()中这部分看着稍微简单一些：
绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技


---

<!-- page 6 -->

可以进行分析，也可以调试观察规律，甚至也可以用z3。可以发现这是个坑，if 和else 都其实是进行了异或：
s1[i] = s1[(len(s1) + i + 1)% len(s1)]
Python
我们也可以提取用于在memcmp 比较的数据为
cmpdata = [0x86, 0x96, 0x12, 0xC1, 0x03, 0x73, 0x05, 0x96, 0x20, 0x21, 0xC2, 0x33, 0xF2, 0x82, 0x06,
0xF7, 0xC7, 0x80, 0xC6, 0x43, 0x52, 0xA0, 0x82, 0x03, 0xC2, 0x83, 0xB3, 0x43, 0xA1, 0x50,
0x43, 0xE0]
绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技


---

<!-- page 7 -->

Python
比较复杂的是proc01，上图IDA 分析错了，其实有个参数。x86 下的反编译结果为：
不是很好分析。这里可以通过调试观察规律，或者是和其他架构下的编译结果交叉对比分析。可以知道就是将每个字节左右
4bit 互换：0xAB →0xBA
到此可以得出解题脚本：
绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技


---

<!-- page 8 -->

from base64 import b64decode
from hashlib import md5
cmpdata = [0x86, 0x96, 0x12, 0xC1, 0x03, 0x73, 0x05, 0x96, 0x20, 0x21, 0xC2, 0x33, 0xF2, 0x82, 0x06,
0xF7, 0xC7, 0x80, 0xC6, 0x43, 0x52, 0xA0, 0x82, 0x03, 0xC2, 0x83, 0xB3, 0x43, 0xA1, 0x50,
0x43, 0xE0]
l = len(cmpdata)
for i in range(l - 1, -1, -1):
cmpdata[i] ^= cmpdata[(i+l+1) % l]
for i in range(l):
cmpdata[i] = (cmpdata[i] << 4 | cmpdata[i] >> 4) & 0xff
data = bytes(cmpdata)
data = b64decode(data)[1:-1]
print(data.decode())
m = md5()
m.update(data) #根据成功后的提示，md5 值为flag
print(m.hexdigest())
Python
可以得出正确的输入为：
F1u77er_S4v3_Th3_W0r1d
得到flag：03cd6eb78a6c70f16bfbfd3508f7065f
绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技


---

<!-- page 9 -->

绿盟科技，禁止外传
传
禁止外传
绿盟科技，禁止外传
止外传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿盟科
绿盟科技，禁止外传
绿盟科技
