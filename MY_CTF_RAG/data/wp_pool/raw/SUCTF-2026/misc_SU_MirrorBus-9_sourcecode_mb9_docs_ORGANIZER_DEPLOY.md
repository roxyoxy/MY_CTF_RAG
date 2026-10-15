# misc_SU_MirrorBus-9_sourcecode_mb9_docs_ORGANIZER_DEPLOY

﻿# MirrorBus-9 Organizer Deploy

## Build Location
Use `env/pwn_deploy` as build context.

## Build
```bash
cd env/pwn_deploy
docker build -t mirrorbus9:latest .
```

## Run
```bash
docker run -d --name mirrorbus9 -p 9999:9999 \
  -e MB9_SEED='seed-team-A' \
  mirrorbus9:latest
```

Set flag by file before build/run:
```bash
echo 'SUCTF{replace_me}' > env/pwn_deploy/bin/flag
```

## Default Difficulty Profile
Runtime defaults are eased (in `start.sh`) to improve solve rate:
- `MB9_SESSION_SEED_MODE=per_connection`
- `MB9_STATE_AMP=2`
- `MB9_OBS_AMP=6`
- `MB9_DELAY_MIN=0`
- `MB9_DELAY_SPAN=1`
- `MB9_ARM_TTL=192`
- `MB9_PROVE_ATTEMPTS=7`

You can still override at runtime if needed.

## Optional Runtime Settings
- `TEAM_TOKEN=team-uuid`
- `MB9_SESSION_SEED_MODE=per_connection`
  - Recommended on a shared/public target.
  - Same TCP connection still supports exact `RESET` replay.
  - Reconnecting creates a new deterministic session seed, so raw transcripts do not replay across players.
- `MB9_DEBUG=0` (default off)
- `MB9_MAX_COMMANDS=4096` (resource limit)
- `MB9_MAX_TICK=20000` (resource limit)

## Health Smoke
```bash
nc 127.0.0.1 9999
HELP
STATUS
ENQ INJ 0 1
COMMIT
POLL 16
```

## Flag Strategy
- Service reads file only (hardened):
  - `/home/ctf/flag`
  - `/flag`

`script/pushflag.sh` remains compatible by writing flag files.

## Runtime Packaging Sanity
```bash
docker exec -it mirrorbus9 sh -lc "ls -la /home/ctf/mb9"
```
Expected: only `app/` exists in `/home/ctf/mb9` (no `author_tools/`, no `docs/`).

## Determinism / Replay Check (Host Side)
Run from `sourcecode/mb9/author_tools` on host:
```bash
python3 replay_check.py --host 127.0.0.1 --port 9999
```

## Concurrency Smoke (Host Side)
```bash
python3 concurrency_smoke.py --host 127.0.0.1 --port 9999 --workers 16
```

## Reference Solve (Host Side)
```bash
python3 solve_reference.py --host 127.0.0.1 --port 9999
```

