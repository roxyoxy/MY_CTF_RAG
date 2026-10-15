# challenges_misc_eat-mian_build_app_static_task

# eat-mian

我要食面！

**注意**：提交的代码中不能出现 `int` 和 `main` 关键字。

你的代码将以头文件的形式嵌入测试代码中，且所有 `int` 和 `main` 均将被替换为 `eat` 和 `mian`。

例如：
```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 你写的代码会插入这里

int main(void) {
  srand(time(NULL));
  int n = rand();
  printf("I int %d cups of main! wwwww\n", n);
  return 0;
}

```
会变成：
```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 你写的代码会插入这里

eat mian(void) {
  srand(time(NULL));
  eat n = rand();
  preatf("I eat %d cups of mian! wwwww\n", n);
  return 0;
}

```

要求输出： `I eat \d+ cups of mian! wwwww`