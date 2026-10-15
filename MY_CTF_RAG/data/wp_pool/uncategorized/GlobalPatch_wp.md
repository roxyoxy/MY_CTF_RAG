# GlobalPatch wp

# GlobalPatch_wp
---

<!-- page 1 -->

Foolish White Entrance
With Global Patch
 
描述
 
这是一个很诡异的分类器，它给了你10张同一类别的图片。你需要
在这10张图片上做相同的手脚，来骗过分类器识别为那个类别的图
片。
出题思路
 
本题目考察的是 Universal Adversarial Patch，是在对抗样本界耳
熟能详的一个概念。该概念在2016年被提出，主要特点是一组图
片，一个补丁。 evaluation中采用的算法采集自 Implementation
of the attack from Moosavi-Dezfooli et al. (2016). | Paper link: h
ttps://arxiv.org/abs/1610.08401
UAP的内迭代算法采集自 http://
doi.acm.org/10.1145/3134600.3134635
本题的数据集为 ，是一个超经典的数据集。采用的训练网络结构如
下所示：
本题限制的范数为 L2和L_inf，它可以通过跑 evalution.py 来获
得。
通过跑 evalution.py，在给定10张图片都是9的情况下，打上相同
的patch，可以获得如下数据
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

因此，理论上只要跑5次，能够成功4次，算是比较理想的结果了。
另外综合考量了其他数据的结果设定了如下阈值：
关于其他数据，均不是很理想，具体数据可以看shadow.py中的其
他数据结果，或者自己跑一遍evaluation.py
 
exp
 
EXP脚本
 
根据服务端获取的FIG_ID与BUFFER，更改内容，可以获得结果。可
能有时候产生的图不会落在阈值中，需要手动更改
9
_sample        9.000000
fail_rate      0.189691
_f_l0        623.010309
_f_l1         16.325303
_f_l2          0.913379
_f_linf        0.137575
# dtype: float64
THRESHOLD_L_0 = 0xff
THRESHOLD_L_1 = 0xff
THRESHOLD_L_2 = 1.1
THRESHOLD_L_INF = 0.15
import time
import matplotlib.pyplot as plt
import numpy as np
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

from art.attacks.evasion import 
UniversalPerturbation
from art.classifiers import TensorFlowV2Classifier
from art.utils import preprocess
# %%
from shaobaobao_adversarial_samples_learning import 
show_d
import random
import tensorflow as tf
import os
import load_dataset
from load_dataset import to_categorical
MINST_DNN_HD5 = './model.h5'
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"  # 这一行注
释掉就是使用cpu，不注释就是使用gpu
# %%
# Step 1: Load the MNIST dataset
# 不知道为啥好像那个链接包烂掉了。于是我就用了tf的，数据格式得
改改
(x_train, y_train), (x_test, y_test), _, _ = 
load_dataset.load_mnist()
_x_train = x_train
_y_train = y_train
# %%
model = tf.keras.models.load_model(MINST_DNN_HD5)  # 
switch model
classifier = TensorFlowV2Classifier(model=model, 
nb_classes=10,
                                   
 loss_object=tf.keras.losses.CategoricalCrossentropy
(),
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

                                    input_shape=(28, 
28, 1), clip_values=(0, 1))
def get_norm(data):
    assert data.shape.__len__() == 4
    linf = 
np.mean(np.linalg.norm(data.reshape(data.shape[0], 
-1), ord=np.Inf, axis=1))
    l0 = len(np.where(np.abs(data) > 0.0)[0]) / 
data.shape[0]
    l1 = 
np.mean(np.linalg.norm(data.reshape(data.shape[0], 
-1), ord=1, axis=1))
    l2 = 
np.mean(np.linalg.norm(data.reshape(data.shape[0], 
-1), ord=2, axis=1))
    return l0, l1, l2, linf
def do_attack(x_input, y_input, y_target=None):
    tfc = TensorFlowV2Classifier(
        model=model,
       
 loss_object=tf.keras.losses.CategoricalCrossentropy
(),
        nb_classes=10,
        input_shape=(28, 28, 1),
        clip_values=(0, 1),
    )
    up = UniversalPerturbation(tfc,
                               max_iter=1, 
attacker="newtonfool", attacker_params={"max_iter": 
50, "verbose": False},
                               verbose=False
                               )
    x_input_adv = up.generate(x_input)
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

<!-- page 5 -->

    train_y_pred = 
np.argmax(tfc.predict(x_input_adv), axis=1)
    fail_rate = np.average(np.argmax(y_input) == 
train_y_pred)
    delta = up.noise
    _f_l0, _f_l1, _f_l2, _f_linf = get_norm(delta)
    print(f"Fail rate {fail_rate},total average \n"
          f"_f_l0 {_f_l0} \n"
          f"_f_l1 {_f_l1} \n"
          f"_f_l2 {_f_l2} \n"
          f"_f_linf {_f_linf} \n"
          )
    return up.noise
from base64 import b64decode, b64encode
def load_data(bb64_buffer, FIG_NUMBER=10):
    """
    :param bb64_buffer: user input buffer
    :param FIG_NUMBER: group of reshape data
    :return:
    """
    try:
        x = np.frombuffer(b64decode(bb64_buffer), 
dtype=np.float32)
        x = x.reshape(FIG_NUMBER, 28, 28, 1)
        return x
    except Exception as e:
        print(e)
        print("No Hack ! BAD BUFFER")
        return False
def get_data(x):
    """
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

<!-- page 6 -->

    :param fig_id: x_train's first index
    :return:
    """
    buffer = x.tobytes()
    return b64encode(buffer)
from pwn import *
debug = True
for i in range(100):
    while True:
        conn = remote('localhost', 20006)
        try:
            time.sleep(5)
            res = conn.recv()
            print(res)
            conn.send('2')
            time.sleep(1)
            conn.recvline()
            conn.recvline()
            time.sleep(3)
            res = conn.recvline()
            ten_img_buffer = res.rstrip(b'\r\n')
            ten_img = load_data(ten_img_buffer, 10)
            noise = do_attack(ten_img, 
to_categorical([9], 10))
            conn.send(get_data(noise))
            res = conn.recv(timeout=10)
        except Exception as e:
            pass
        conn.close()
        if b'flag' in res:
            print(res)
            break
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

<!-- page 7 -->

 
    plt.imshow(np.reshape(noise, (28, 28)), )
    plt.show()
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
