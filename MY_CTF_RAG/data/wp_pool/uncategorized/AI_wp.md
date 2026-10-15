# AI wp

# AI_wp
---

<!-- page 1 -->

2022/7/13 18:06
solution-final
file://10.10.10.120/guest/2021线下竞赛/2021-12-10 武汉工信部/2021年工信部决赛CTF赛题赛前测试/AI/AI_人工智能/解题思路/solution-final.html
1/2
解题字符串
加载模型
通过LIME可解释模型，探测模型的分类依据
[[0.95858913 0.04141087]]
/mnt/vdb/zhangrunzi/CTF/env/lib/python3.6/site-packages/lime/lime_text.py:114: FutureW
arning: split() requires a non-empty pattern match.
 self.as_list = [s for s in splitter.split(self.raw) if s]
[('READ', 0.06328934433429637),
('record', 0.058321865729219075),
('innocent', 0.055385965174517014),
('student', 0.053697344602394864),
('sing', 0.04876841186332404),
('18', 0.045733527259477214),
('Media', 0.03774213631392826),
('But', 0.022920318739692077),
('old', 0.017060553170835678),
('can', 0.015679805002229142),
('year', 0.015021511621958836),
('chance', 0.014634589969838466),
('your', 0.012086646472989298),
('t', -0.0049954665733069475),
('Hey', -0.008204270126828743),
('subscribe', -0.00891118048862067),
('hear', -0.009905187424820758),
('what', -0.017049624020991017),
('them', -0.04298560719882508),
('upload', -0.1335992330896791)]
:
]
8
 [
n
I
import pandas
import pickle
import lime
:
]
1
2
 [
n
I
# 分类为0的字符串语句
string0 = '''****PLEASE READ*****  Hey everyone! I&#;m a 18 year old innocent student 
# 分类为1的字符串语句
string1 = ' (CPS) is a mechanism for sys controlled or monitored by data-based algorit
:
]
2
2
 [
n
I
loaded_model = pickle.load(open('classifier', 'rb'))
:
]
3
2
 [
n
I
from lime.lime_text import LimeTextExplainer
# 标签，只有0和1
label_map = {0: 0, 1: 1}
# 初始化一个LIME解释探测器
explainer = LimeTextExplainer(class_names=label_map)
# 给出string0的预测值，分别是标签0和标签1的
print(loaded_model.predict_proba([string0]))
# 使用LIME来探测模型
exp = explainer.explain_instance(string0, loaded_model.predict_proba, num_features=20
# 获取与标签0相关的关键词，取正数为对预测为标签0有正向作用，取负数为对预测为标签0有反向
my_list = exp.as_list(label=label_map[0])
# 排序
my_list.sort(key=lambda x: x[1],reverse=True)
my_list
:
]
3
2
[
t
u
O
:
]
4
2
 [
n
I
# 给出string1的预测值，分别是标签0和标签1的
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

2022/7/13 18:06
solution-final
file://10.10.10.120/guest/2021线下竞赛/2021-12-10 武汉工信部/2021年工信部决赛CTF赛题赛前测试/AI/AI_人工智能/解题思路/solution-final.html
2/2
[[0.0507184 0.9492816]]
/mnt/vdb/zhangrunzi/CTF/env/lib/python3.6/site-packages/lime/lime_text.py:114: FutureW
arning: split() requires a non-empty pattern match.
 self.as_list = [s for s in splitter.split(self.raw) if s]
[('Suscribe', 0.16332319917357285),
('cyber', 0.1330100255956907),
('twitter', 0.06529801403328447),
('change', 0.05443324926597899),
('scales', 0.009774923155111385),
('In', 0.007305398866495078),
('controlled', 0.006298737398347218),
('distinct', 0.005710865492650487),
('systems', -0.006703474362118372),
('or', -0.006724614243633883),
('algorithms', -0.008088126558070306),
('methods', -0.00840741009418656),
('data', -0.008808390661422827),
('more', -0.01158402417752101),
('that', -0.01211132623216573),
('other', -0.012163694069486712),
('lol', -0.01599483691553156),
('for', -0.02001359791717968),
('with', -0.022416937626130874),
('and', -0.02768889703339794)]
Tips
模型classifier是单词大小写敏感的
LIME方法并不是唯一模型探测实现方法，可通过多种工具和模型方法实现基于输入扰动对模
型判断的分析探测。
print(loaded_model.predict_proba([string1]))
# 使用LIME来探测模型
exp = explainer.explain_instance(string1, loaded_model.predict_proba, num_features=20
# 获取与标签1相关的关键词，取正数为对预测为标签1有正向作用，取负数为对预测为标签1有反向
my_list = exp.as_list(label=label_map[1])
my_list.sort(key=lambda x: x[1],reverse=True)
my_list
:
]
4
2
[
t
u
O
:
 ]
 [
n
I
 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
