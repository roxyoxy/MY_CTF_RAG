# suctf 2026 SU minivfs writeup

漏洞位于write功能中，存在off by null漏洞。

利用off by null漏洞进行堆风水，然后攻击mp_修改tcachebin的大小，最后劫持printf函数进行IO_FILE利用。

读取文件夹下的flag文件名称后，利用orw读取flag文件的内容。