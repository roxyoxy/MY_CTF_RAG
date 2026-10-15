# misc SU MirrorBus 9 env pwn deploy deploy

﻿1.
```bash
docker build -t "mirrorbus9" .
```

2.
```bash
docker run -d -p "0.0.0.0:pub_port:9999" \
  -e MB9_SEED='seed-demo' \
  --name="mirrorbus9" mirrorbus9
```

Replace `pub_port` with your public challenge port.

Before running, inject flag by file:
```bash
echo 'SUCTF{replace_me}' > ./bin/flag
```

3.
```bash
# Optional hardening verification: runtime image should not contain author solver
docker exec -it mirrorbus9 sh -lc "ls -la /home/ctf/mb9"
```
Expected: only `app/` exists in `/home/ctf/mb9` (no `author_tools/`, no `docs/`).

Difficulty note:
- Shared/public target hardening defaults:
  - `MB9_SESSION_SEED_MODE=per_connection`
  - Same connection supports exact `RESET` replay.
  - Reconnecting derives a new deterministic session seed, so raw transcripts do not replay across teams.
- Container startup script uses eased defaults to reduce 0-solves:
  - `MB9_STATE_AMP=2`
  - `MB9_OBS_AMP=6`
  - `MB9_DELAY_MIN=0`
  - `MB9_DELAY_SPAN=1`
  - `MB9_ARM_TTL=192`
  - `MB9_PROVE_ATTEMPTS=7`
- You may override any of them by adding `-e KEY=value` in `docker run`.

