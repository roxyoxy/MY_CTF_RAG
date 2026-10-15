# web-oh-my-lotto_revenge_docker_oh-my-lotto_

# 部署说明

```
1. 安装docker和docker-compose
2. 将当前用户添加到docker运行组中
3. 给run.sh脚本赋予执行权限
4. 安装依赖  `pip3 install - requirements.txt`
5. 运行 `gunicorn -D app:app -c ./gunicorn.conf.py`
```