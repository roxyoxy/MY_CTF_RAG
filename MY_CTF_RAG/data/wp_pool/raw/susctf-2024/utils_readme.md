# utils_readme

# About `challenge.toml`

> All of the commited lines below are unnecessary

## base config
```toml
title = "title example"

# note: category will be set automatically if not exists
# category = "Misc" # "Misc", "Crypto", "Pwn", "Web", "Reverse"

# note: type will be set automatically:
#     if [container] is set,
#         type will be set to "StaticContainer" or "DynamicContainer",
#         depends on the value of:
#             static = true/false
#     otherwise, type will be set to "StaticAttachment"
```

## meta info

```toml
author = "your name"
difficulty = "Easy" # "Easy", "Medium", "Hard"

description = """
    write markdown here
"""

flag = "flag{example}"

# note: if you need multiple flags, use a list instead
# warn: flag won't be deleted automatically
# flag = ["flag{1}", "flag{2}"]
```

## attachments config
```toml
[attachments]
# path = "attachments/**"
# path = ["hints/**", "attachments/**"]

# note: if you want upload zip file directly, use a string endswith ".zip"
# path = "task.zip"

# name = "attachments.zip"
```

## container config
```toml
[container]
port = 8080
memory = 64
storage = 256
# warn: image will be override by sys.argv[3] if exists
image = "ubuntu:latest"
```