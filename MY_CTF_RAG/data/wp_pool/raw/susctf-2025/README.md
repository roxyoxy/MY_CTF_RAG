# README

# SUSCTF@2025 官方题目仓库

比赛时间： 2025.10.4~2025.10.6

比赛网址： https://ctf.seusus.com

## 新生赛道

见 [newcomers](https://github.com/susers/susctf-2025/tree/newcomers) 分支。

### 官方 WriteUp

见 `writeup.pdf`。

### 题目文件夹结构
```plain
题目名字
├── README.md(题目信息)
├── challenge.toml(题目运行配置)
├── attachments(默认附件文件夹)
│   ├── 题目附件内容
├── build(容器构建目录)
│   ├── Dockerfile
```

### GZCTF 小工具
```
utils
├── bot.py(QQBot)
├── diff.py(CI专用)
├── gzctf_api.py(常用GZCTF API)
├── pull.py(从GZCTF平台下拉赛题信息)
└── update.py(上传赛题至GZCTF平台)
```