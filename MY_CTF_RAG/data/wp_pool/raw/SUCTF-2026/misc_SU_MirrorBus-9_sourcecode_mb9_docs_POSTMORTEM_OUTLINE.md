# misc SU MirrorBus 9 sourcecode mb9 docs POSTMORTEM OUTLINE

# MirrorBus-9 Postmortem Outline

## 1. Why This Is a Fair Hard Misc
- Deterministic black-box system; no timing/network roulette.
- Every misleading signal is experimentally falsifiable.
- Difficulty comes from experiment design + state control, not brute force.

## 2. Talk Track (Recommended)
1. Protocol semantics: queue/commit/poll and why no-echo exists.
2. Delayed mixed observation and why naive local fitting fails.
3. Deterministic noise and replay-based denoising.
4. ARM as state gate, not keyword gate.
5. PROVE construction from challenge equations.

## 3. Typical Team Failure Modes
- Ignored transcript-level state and worked with short windows.
- Treated deterministic drift as randomness.
- Confused queue acceptance (`QOK`) with gate success.
- Overfit wrong local model and never falsified it.

## 4. Evidence for Reproducibility
- Same seed + same transcript => same output hash.
- `replay_check.py` and golden transcript hash test included.
- Multi-seed validation script records pass rate and failure reasons.

## 5. Controversy Response Hooks
- "This depends on terminal/network tricks": false; behavior is step-based deterministic.
- "This is pure author mind-reading": false; wrong assumptions are refutable by A/B tests.
- "This cannot be verified": false; preflight, replay, and seed validation are shipped.

