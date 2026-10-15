# misc_SU_MirrorBus-9_sourcecode_mb9_docs_JUDGE_QUICKREF

﻿# MirrorBus-9 Judge Quick Reference

## 1) One-line Mechanism Summary
Half-duplex black-box bus: players queue commands (`ENQ`), execute in batches (`COMMIT`), observe delayed mixed logs (`POLL`), then pass `ARM/PROVE`.

## 2) Intended Solve Skeleton
1. Confirm service is alive via `HELP/STATUS`.
2. Build transcript logger; avoid terminal-only play.
3. Use `RESET` + controlled probes to model delay and mixed outputs.
4. Drive hidden state to ARM manifold.
5. Read `CHAL`, compute `PROVE`, submit.

## 3) Common Wrong Assumptions
- `ENQ` has no state echo by design.
- `QOK` does not mean ARM success.
- Observation timing is delayed and deterministic, not network jitter.

## 4) On-call Triage Commands
```bash
docker ps --filter name=mirrorbus9
docker logs --tail 100 mirrorbus9
docker exec -it mirrorbus9 sh -lc "pgrep -a xinetd; ls -l /home/ctf/flag /home/ctf/mb9/app/main.py"
```

## 5) Fast Sanity Signals
- `STATUS` returns increasing heartbeat/tick.
- `COMMIT` shows `exec/produced`.
- `POLL` always ends with `END`.
- `PROVE` success returns one line containing `status=PASS flag=...`.

## 6) Known Non-bugs (Do Not Misdiagnose)
- No immediate observation after `ENQ`.
- Empty `COMMIT` returns summary with unchanged `cid`.
- `POLL` can advance logical time and mature delayed frames.
- If `MB9_SESSION_SEED_MODE=per_connection` is enabled, reconnecting changes the session seed by design.

## 7) Emergency Difficulty Lowering (Non-breaking)
Apply at container runtime:
- `MB9_STATE_AMP=2`
- `MB9_OBS_AMP=6`
- `MB9_DELAY_MIN=0`
- `MB9_DELAY_SPAN=1`
- `MB9_ARM_TTL=192`
- `MB9_PROVE_ATTEMPTS=7`

This keeps protocol/mechanism unchanged and only reduces noise/time pressure.

