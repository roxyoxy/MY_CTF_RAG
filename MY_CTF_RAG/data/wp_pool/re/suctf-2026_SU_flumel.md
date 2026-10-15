# suctf 2026 SU flumel

# SU_flumel

## 本地复现

- 选手附件位于 `attachments/attachment.zip`
- 题解位于 `writeup/SU_flumel Writeup.md`
- 精简后的源码快照位于 `sourcecode/sourcecode.zip`

## 归档瘦身说明

- 原始 `sourcecode/sourcecode.zip` 约 `1.69G`，其中绝大部分是 `.dart_tool`、`.gradle`、`build/` 和生成出的 APK/so 等中间产物
- 当前仓库保留了去除构建缓存后的源码快照，不影响选手按附件和 writeup 复现题目
- 如果需要重新构建 APK，请在本地恢复 Flutter / Android 工具链并重新执行依赖安装与构建流程
