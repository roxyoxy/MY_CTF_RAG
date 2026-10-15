# EasyToPoison_wp

# EasyToPoison_wp
---

<!-- page 1 -->

Easy to Poison
 
出题思路
 
后门攻击是投毒攻击的一种。
所谓后门攻击，是指攻击者通过选择一个触发器（比如说一个特定
的图案），并根据 触发器生成一些中毒数据，随后将中毒数据与干
净数据混合，提供给受害者用来训 练深度模型。经过训练的模型将
在常规数据上产生正确的结果，因此受害者不会意 识到模型受到了
损害。然而，当攻击者将触发器“粘贴”到源图像上时，该模型会 错
误地将源类别图像分类为目标类别。
本题为一个非常简单的后门攻击的样例。也是看看后门攻击能不能
出在ctf场景中的一种尝试。主要流程如下
1）攻击者提交10张手写数字8图片，他们会在模型A上被识别为数
字8
2）将这10张图片的label标注为手写数字9，与训练集混合，重
新训练一个模型B（模型A与模型B结构一样）
3）这10张图片均被
模型B识别为手写数字9
3）再提交一张图片，该图片与这10张不一
样，并且在模型A上被识别为数字8，在模型B上被识别为数字9（很
显然，该图片没有经过训练，相当于一个测试集）
解题脚本
 
from __future__ import absolute_import, division, 
print_function, unicode_literals
import os, sys
import random
import time
from os.path import abspath
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

module_path = os.path.abspath(os.path.join('..'))
if module_path not in sys.path:
    sys.path.append(module_path)
import warnings
warnings.filterwarnings('ignore')
import tensorflow.keras.backend as k
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Flatten, 
Conv2D, MaxPooling2D, Activation, Dropout
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits import mplot3d
from art.estimators.classification import 
KerasClassifier
from art.attacks.poisoning import 
PoisoningAttackBackdoor
from art.attacks.poisoning.perturbations import 
add_pattern_bd, add_single_bd, insert_image
from art.utils import load_mnist, preprocess
from art.defences.detector.poison import 
ActivationDefence
import tensorflow as tf
import random
from utils import load_mnist_small
from tensorflow.keras.models import load_model
from base64 import b64encode, b64decode
tf.compat.v1.disable_eager_execution()
(x_raw, y_raw), (_, _), _, _ = 
load_mnist_small(True)
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

(x_train, y_train), (x_test, y_test), min_, max_ = 
load_mnist_small(False)
# TODO SAVE DATA
max_val = 255
BACKDOOR_TYPE = "pattern"  # one of ['pattern', 
'pixel', 'image']
def add_modification(x):
    if BACKDOOR_TYPE == 'pattern':
        return add_pattern_bd(x, 
pixel_value=max_val)
    elif BACKDOOR_TYPE == 'pixel':
        return add_single_bd(x, pixel_value=max_val)
    elif BACKDOOR_TYPE == 'image':
        return insert_image(x, 
backdoor_path='../utils/data/backdoors/alert.png', 
size=(10, 10))
    else:
        raise ("Unknown backdoor type")
def poison_dataset(x_clean, y_clean, percent_poison, 
poison_func):
    """
    Foolish 0-> 1
    :param x_clean:
    :param y_clean:
    :param percent_poison:
    :param poison_func:
    :return:
    """
    sources = np.array([8] * 10)
    targets = np.array([9] * 10)
    x_poison, y_poison = None, None
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

    for i, (src, tgt) in enumerate(zip(sources, 
targets)):
        n_points_in_tgt = np.size(np.where(y_clean 
== tgt))
        num_poison = round((percent_poison * 
n_points_in_tgt) / (1 - percent_poison))
        src_imgs = x_clean[y_clean == src]
        n_points_in_src = np.shape(src_imgs)[0]
        indices_to_be_poisoned = 
np.random.choice(n_points_in_src, num_poison)
        imgs_to_be_poisoned = 
np.copy(src_imgs[indices_to_be_poisoned])
        backdoor_attack = 
PoisoningAttackBackdoor(poison_func)
        imgs_to_be_poisoned, poison_labels = 
backdoor_attack.poison(imgs_to_be_poisoned, 
y=np.ones(num_poison) * tgt)
        if x_poison is None:
            x_poison = imgs_to_be_poisoned
        else:
            np.append(x_poison, imgs_to_be_poisoned)
        if y_poison is None:
            y_poison = poison_labels
        else:
            np.append(y_poison, poison_labels)
    return x_poison, y_poison
def get_data(data):
    """
    :param fig_id: x_train's first index
    :return:
    """
    buffer = data.tobytes()
    return b64encode(buffer)
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

def load_data(bb64_buffer, FIG_NUMBER=1):
    """
    :param bb64_buffer: user input buffer
    :param FIG_NUMBER: group of reshape data
    :return:
    """
    try:
        patch = 
np.frombuffer(b64decode(bb64_buffer), 
dtype=np.float32)
        patch = patch.reshape(FIG_NUMBER, 28, 28, 1)
        print("data load pass")
        return patch
    except Exception as e:
        print(e)
        print("No Hack ! BAD BUFFER")
        return False
def do_attack():
    percent_poison = .1
    x_poisoned_raw, y_poisoned_raw = 
poison_dataset(x_raw, y_raw, percent_poison, 
add_modification)
    x_poisoned, y_poisoned = 
preprocess(x_poisoned_raw, y_poisoned_raw)
    x_poisoned = np.expand_dims(x_poisoned, axis=3)
    assert x_poisoned_raw.shape[0] == 11
    return evaluate_backdoor_effect(x_poisoned, 
y_poisoned)
def evaluate_backdoor_effect(x_poisoned, 
y_poisoned):
    x_poisoned_test, y_poisoned_test = 
x_poisoned[-1], y_poisoned[-1]
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

    clean_preds = 
np.argmax(load_model('model.h5').predict(x_poisoned)
, axis=1).tolist()
    assert set(clean_preds).__len__() == 1 and 
clean_preds[0] == 8
    print("poison pass ", clean_preds)
    model = Sequential()
    model.add(Conv2D(32, kernel_size=(3, 3), 
activation='relu', input_shape=(28, 28, 1)))
    model.add(Conv2D(64, (3, 3), activation='relu'))
    model.add(MaxPooling2D(pool_size=(2, 2)))
    model.add(Dropout(0.25))
    model.add(Flatten())
    model.add(Dense(128, activation='relu'))
    model.add(Dense(10, activation='softmax'))
    model.compile(loss='categorical_crossentropy', 
optimizer='adam', metrics=['accuracy'])
    model.fit(np.append(x_train, x_poisoned[:-1], 
axis=0), np.append(y_train, y_poisoned[:-1], 
axis=0), epochs=5,
              batch_size=8, verbose=0)
    clean_preds = np.argmax(model.predict(x_test), 
axis=1)
    clean_correct = np.sum(clean_preds == 
np.argmax(y_test, axis=1))
    clean_total = y_test.shape[0]
    clean_acc = clean_correct / clean_total
    print("\nClean test set accuracy: %.2f%%" % 
(clean_acc * 100))
    assert clean_acc >= 0.94
    clean_preds = 
np.argmax(model.predict(x_poisoned), axis=1)
    clean_correct = np.sum(clean_preds == 
np.argmax(y_poisoned, axis=1))
    clean_total = y_poisoned.shape[0]
    clean_acc = clean_correct / clean_total
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

    print("\nPoison success rate accuracy: %.2f%%" % 
(clean_acc * 100))
    assert clean_acc == 1.0
    c_id = -1
    print("prediction for not test data ",
         
 np.argmax(model.predict(np.expand_dims(x_poisoned_t
est, axis=0)), axis=1) == 9)
    plt.imshow(x_poisoned[c_id].squeeze())
    plt.title(np.argmax(y_poisoned[c_id]))
    plt.show()
    assert 
(np.argmax(model.predict(np.expand_dims(x_poisoned_t
est, axis=0)), axis=1) == 9)[0] == True
    return x_poisoned[:-1], x_poisoned_test
a, b = do_attack()
c_id = -1
plt.imshow(a[c_id].squeeze())
plt.show()
import requests
os.environ['NO_PROXY'] = "shaobaobaoer_ubuntu20"
session = requests.Session()
r = 
session.get("http://shaobaobaoer_ubuntu20:21007/star
t")
print(r.text)
a = a.astype(np.float32)
b = b.astype(np.float32)
r = 
session.post("http://shaobaobaoer_ubuntu20:21007/eva
luate", data={
    "fig_buffer": get_data(a),
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

<!-- page 8 -->

    "hack_buffer": get_data(b)
})
print(r.text)
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
