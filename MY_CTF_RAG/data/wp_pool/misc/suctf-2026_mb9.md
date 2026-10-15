# suctf 2026 mb9

# MirrorBus-9 Source Tree

- `app/`: service implementation for xinetd stdio mode.
- `author_tools/`: reference solver and validation scripts.
- `tests/`: pytest regression suite.
- `docs/`: mechanism and deployment docs.
- `docs/PARTICIPANT_GIFT.md`: fake-flag easter egg (not submittable).
- `author_tools/preflight_check.py`: organizer handoff checker.
- `author_tools/run_selfcheck.sh`: one-shot runtime smoke + determinism + seed validation.
- `author_tools/concurrency_smoke.py`: concurrent xinetd connection smoke.

Local test:
```bash
cd sourcecode/mb9
python3 -m pytest -q
```
