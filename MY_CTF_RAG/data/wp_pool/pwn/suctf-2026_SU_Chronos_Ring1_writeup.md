# suctf-2026_SU_Chronos_Ring1__writeup

## this is a writeup

驱动程序在执行核心功能前，要求通过 `CHRONOS_SET_OPTS` 设置一个 `cookie`。该 `cookie` 的生成逻辑依赖于内核导出符号 `kfree` 的地址：
$$
\text{cookie} = \text{rol64}((\text{kfree} \gg 21) \oplus 0x9e3779b97f4a7c15, 17) \oplus \text{session\_idx}
$$
由于 KASLR 的存在，`kfree` 的地址在每次启动时都会随机化，选手必须先泄露内核基址。

### 核心漏洞：状态机与生命周期错配

漏洞位于 `CHRONOS_DETACH_FILE` 函数中。该函数模拟了将文件绑定缓冲区回退为本地内存缓冲区的业务逻辑：

1. 它将 `buf->mode` 修改为 `CHRONOS_MODE_LOCAL`。
2. 它释放了 `buf->file_page` 和 `buf->attached_file`。
3. **关键漏洞点**：它没有清理 `buf->sync_view` 指针。

此时，`sync_view` 仍然指向之前 `FILE` 模式下映射的 **Page Cache（页面缓存）** 物理页，但缓冲区模式已被标记为 `LOCAL`。

### 越权写原语 

在 `CHRONOS_COMMIT_SYNC` 路径中，程序会通过 RCU 解引用 `buf->sync_view`。

由于 `DETACH_FILE` 留下的残留指针，当选手调用 `COMMIT_SYNC` 时，内核会将 `local_ring` 中的内容复制到 `sync_view->vaddr` 中。

如果该 `view` 之前绑定的是只读文件（如 `/tmp/job`），`set_page_dirty(view->page)` 将强制内核认为该页面已修改。这将导致**只读文件的页面缓存在内存中被篡改**，且对后续读取该文件的进程可见。

------

### Exploit Chain

#### KASLR 绕过 (AVX Timing Side-Channel)

由于环境开启了 `kaslr` 且没有直接的泄露点，利用 **AVX Masked Load (`vmaskmovps`)** 的时序差异来探测内核地址。

- **原理**：探测内核内存时，已缓存的页面（Hot Pages）响应速度远快于未映射或未缓存页面。
- **修正**：侧信道通常锁定的不是 `_text` 起点，而是内核热点区域（偏移约 `0x1600000`），侧信道扫出的值通常在真实基址附近，在 `leak ± 32MB` 的范围内，以 `2MB` 为步长尝试所有可能的基址并计算 Cookie。
- **计算**：通过侧信道基址减去偏移量得到 `_text`，再加静态偏移得到 `kfree`，从而计算出合法的 `cookie` 解锁 Gate。

#### 构造悬空视图 (Dangling View)

1. 调用 `CHRONOS_REG_BUF` 分配基础对象。
2. `O_RDONLY` 打开的目标脚本 `/tmp/job`，可以通过爆破hash来判断。
3. 调用 `CHRONOS_ENABLE_SYNC` 建立指向 `/tmp/job` 页面缓存的映射。
4. 调用 `CHRONOS_DETACH_FILE` 触发漏洞，使 `sync_view` 变为指向 Page Cache 的悬空指针。

#### 提权 (Privilege Escalation)

1. 调用 `CHRONOS_UPDATE_BUF` 将恶意 Shell 脚本（如 `cp /flag /tmp/flag`）写入本地缓冲区。
2. 调用 `CHRONOS_COMMIT_SYNC`。此时内核将恶意脚本覆写进 `/tmp/job` 的页面缓存。
3. **等待触发**：后台以 root 权限运行的 `while` 循环会执行 `/tmp/job`。
4. 由于页面缓存已被篡改，root 进程执行的实际上是选手的提权脚本。

