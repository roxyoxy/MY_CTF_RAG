# suctf 2026 env~1

# CloudHook Service (SU_uri)

This challenge is a webhook forwarder with URL validation.
The intended path is DNS rebinding to bypass validation and reach Docker API through the local `2375` bridge.

## Run with Docker

```bash
docker compose up --build -d
```

Service: `http://127.0.0.1:8080`

## Services

- `cloudhook-service`: single service container
  - startup script prepares `/host/flag` and `/host/readflag`
  - Go web app mounts `/var/run/docker.sock`, bridges it to `127.0.0.1:2375`

## Environment

- `PORT`: fixed in app as `8080`
- DNS rebinding domain: provide at exploit runtime (see `exp/exp.py`)

## Notes

- Current flag mechanism is static (from `setup/readflag.c` encoded bytes).
- `bin/start.sh` and `bin/pushflag.sh` are template-aligned helper scripts.
- Non-expected mapped-local bypass such as `::ffff:127.0.0.1` is blocked in `internal/waf/waf.go`.
