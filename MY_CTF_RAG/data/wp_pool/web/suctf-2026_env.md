# suctf 2026 env

﻿# SU CTF SQLi (PG17 + JS sign)

This challenge combines JS reverse-engineering (client-side signing) with a PostgreSQL error-based injection using SQL/JSON functions.
The signing core is split across two WASM modules and includes a UA + time-window + salt derived dynamic key.
The `seed` returned by `/api/sign` is a packed value (variable-length split + shuffled + padding) and reassembled in WASM.

## Run with Docker

```bash
docker compose up --build
```

Service: http://127.0.0.1:8080

Compose now runs as a single container service:
- image: `su_sqli:latest`
- internal processes: PostgreSQL 17 + `/app/server`

## Local run

```bash
export DATABASE_URL='postgres://postgres:postgres@127.0.0.1:5432/ctf?sslmode=disable'
export SIGN_SECRET='suctf2026_sign_secret_change_me_32b'
go run .
```

## Rebuild WASM (optional)

```bash
GOCACHE=/tmp/go-build GOMODCACHE=/tmp/go-mod GOOS=js GOARCH=wasm go build -o web/static/crypto1.wasm ./web/wasm1
GOCACHE=/tmp/go-build GOMODCACHE=/tmp/go-mod GOOS=js GOARCH=wasm go build -o web/static/crypto2.wasm ./web/wasm2
```

## Environment

- `PORT`: HTTP port (default `8080`)
- `DATABASE_URL`: PostgreSQL DSN (single-container default: `postgres://postgres:postgres@127.0.0.1:5432/ctf?sslmode=disable`)
- `SIGN_SECRET`: raw string, `hex:...`, or `b64:...` (normalized to 32 bytes)
- `POSTGRES_DB`: PostgreSQL database name (default `ctf`)
- `POSTGRES_USER`: PostgreSQL user (default `postgres`)
- `POSTGRES_PASSWORD`: PostgreSQL password (default `postgres`)

## API

- `GET /api/sign` -> `{ nonce, ts, seed, salt }`
- `POST /api/query` -> `{ q, nonce, ts, sign }`

The server verifies the signature and then runs the query with a lightweight WAF.

## Database

Schema and seed data live in `db/init.sql`.

