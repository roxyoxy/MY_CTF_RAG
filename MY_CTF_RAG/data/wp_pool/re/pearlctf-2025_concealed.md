# pearlctf-2025_concealed

The first tricky part was which python version to use. In the description it was given a ``core maintainer``, and any person who contributes to any software will use the latest bleeding edge stuff compiled from master.

So the required python version was ``3.14-dev`` compiled from source.

Going through the disassembly of ``chall.pyc`` we can clearly see that
something called ``lib.pyc`` is being created and loaded at runtime, and it is overwriting our ``generate_key`` function.

```
  10            LOAD_NAME                5 (open)
                PUSH_NULL
                LOAD_NAME                0 (sys)
                LOAD_ATTR               12 (argv)
                LOAD_SMALL_INT           0
                BINARY_OP               26 ([])
                LOAD_CONST               3 ('rb')
                CALL                     2
                COPY                     1
                LOAD_SPECIAL             1 (__exit__)
                SWAP                     2
                SWAP                     3
                LOAD_SPECIAL             0 (__enter__)
                CALL                     0
        L1:     STORE_NAME               7 (f)

  11            LOAD_NAME                7 (f)
                LOAD_ATTR               17 (seek + NULL|self)
                LOAD_CONST               4 (1693)
                CALL                     1
                POP_TOP

  12            LOAD_NAME                5 (open)
                PUSH_NULL
                LOAD_CONST               5 ('lib.pyc')
                LOAD_CONST               6 ('wb')
                CALL                     2
                COPY                     1
                LOAD_SPECIAL             1 (__exit__)
                SWAP                     2
                SWAP                     3
                LOAD_SPECIAL             0 (__enter__)
                CALL                     0
        L2:     STORE_NAME               9 (ff)

```

This says that load the current script (``sys.argv[0]``), go to the offset of ``1693`` and then write all data after that into a new file called ``lib.pyc``.

Let us extract that file, and see its disassembly:

The first few lines are for two constants, one 2d matrix of ``10x10`` and another result vector of ``10x1``.

The main code to see is this:
```
  --           MAKE_CELL                0 (input_str)
               MAKE_CELL                2 (i)

  26           RESUME                   0

  27           LOAD_GLOBAL              1 (len + NULL)
               LOAD_DEREF               0 (input_str)
               CALL                     1
               LOAD_SMALL_INT          10
               COMPARE_OP             119 (bool(!=))
               POP_JUMP_IF_FALSE        3 (to L1)
               NOT_TAKEN

  28           LOAD_CONST               0 ('')
               RETURN_VALUE

  30   L1:     LOAD_GLOBAL              3 (range + NULL)
               LOAD_SMALL_INT          10
               CALL                     1
               GET_ITER
       L2:     FOR_ITER                71 (to L4)
               STORE_DEREF              2 (i)

  31           LOAD_GLOBAL              5 (sum + NULL)
               LOAD_FAST                2 (i)
               LOAD_FAST                0 (input_str)
               BUILD_TUPLE              2
               LOAD_CONST               1 (<code object <genexpr> at 0x7f4301674ab0, file "lib.py", line 31>)
               MAKE_FUNCTION
               SET_FUNCTION_ATTRIBUTE   8 (closure)
               LOAD_GLOBAL              3 (range + NULL)
               LOAD_SMALL_INT          10
               CALL                     1
               GET_ITER
               CALL                     0
               CALL                     1
               STORE_FAST               1 (res)

  32           LOAD_GLOBAL              7 (abs + NULL)
               LOAD_FAST                1 (res)
               LOAD_GLOBAL              8 (result)
               LOAD_DEREF               2 (i)
               BINARY_OP               26 ([])
               BINARY_OP               10 (-)
               CALL                     1
               LOAD_CONST               2 (0.001)
               COMPARE_OP             148 (bool(>))
               POP_JUMP_IF_TRUE         3 (to L3)
               NOT_TAKEN
               JUMP_BACKWARD           70 (to L2)
```

It may be hard to understand but after some time you can see that it is essentially multiplying a row of the 2d matrix with the ``input_str`` vector and storing it in ``res``.
And if the relative error is small (``abs(res - result[i]) < 0.001``) then it continues.

All we need to do to reverse this is multiply the inverse of the 2d matrix with the ``result`` vector and we have our password.

```python
import numpy as np
from cryptography.fernet import Fernet

matrix = np.array([
    [2.87820614, 3.36437777, 2.22894399, 2.76639864, 2.42070254, 3.39755723, 2.80394189, 2.14227127, 2.10269065, 3.11172518],
    [2.1544405, 2.45056188, 1.99797786, 2.40596576, 2.23730903, 2.74739198, 2.02485171, 1.40173713, 1.64652018, 2.51263685],
    [3.05582561, 3.92682982, 2.3152394, 3.17382136, 2.7837761, 3.85270527, 3.70518023, 3.13272975, 2.5822312, 3.67850168],
    [1.41001173, 2.05787068, 1.59624839, 1.80342511, 1.63760653, 1.76586534, 1.56900378, 1.72811157, 1.65976157, 2.05024162],
    [2.28602176, 3.02159631, 1.37033337, 2.05654502, 1.76387807, 2.7768552, 2.53151281, 2.04959363, 1.85909947, 2.4112406],
    [2.29382647, 2.89912349, 1.58636083, 2.44859857, 1.84408538, 2.95867157, 2.75823258, 2.36303093, 1.92004264, 2.79543383],
    [2.19551049, 2.57542667, 1.56146419, 2.37648158, 1.90162546, 2.12141826, 2.43954828, 1.91472877, 1.67494709, 2.14584809],
    [3.13988706, 4.14676335, 2.63151925, 3.65383716, 2.44613324, 4.3860607, 3.48181208, 2.76051401, 2.35169318, 3.83502528],
    [2.13767187, 2.48357491, 1.73283915, 2.64260204, 1.76219397, 2.47110186, 2.39563679, 2.01023745, 1.45008358, 2.41784153],
    [3.45864873, 4.62023288, 3.11442567, 4.33792085, 2.9988506, 4.57007395, 3.80675253, 3.09856251, 2.73162108, 4.1702761]
])

inverse_matrix = np.linalg.inv(matrix)

result = [2346.28499236, 1861.27932182, 2785.28297579, 1506.69677331,
       1901.90485442, 2060.11738178, 1829.02062564, 2826.83244248,
       1877.36070327, 3197.2217601 ]

chars = np.dot(inverse_matrix, result)

key = ''.join([chr(int(np.rint(i))) for i in chars]) + "tleV90aGF0X25vX29uZV93aWxsX2tub3c="
# print(key[:10])
enc = b"gAAAAABnwGsbt_T6av7yIIKiIM4Zxyb400IieMQsULzzv9qiPKcBiT3i04X3jgAPGvRJD34Upj5cc7gCq\
b4e3mS4wqCvcfo3b06S0wQRvO4oFzHmn3a_v6J0QmwXmqkaGlg8FaHUWOwF"
f = Fernet(key.encode())
flag = f.decrypt(enc)
print(flag)
```

```
b'pearl{wh0_kn3w_func710n5_c0uld_b3_h1dd3n}'
```
