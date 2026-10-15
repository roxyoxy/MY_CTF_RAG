# misc_SU_MirrorBus-9_sourcecode_mb9_docs_DESIGN

﻿# MirrorBus-9 Design Notes

## 1. Core Model
- Hidden state: `x_t in Z_p^9`, `p=65521`.
- History dependency: transition uses `x_t`, `x_{t-1}`, `x_{t-2}`.
- Half-duplex semantics:
  - `ENQ` only queues commands.
  - `COMMIT` executes queue and schedules delayed frames.
  - `POLL` fetches mature frames.

## 2. Transition
For each lane `i`, with phase-dependent index remap:

`x_{t+1}[i] = alpha_i * x_t[j0] + beta_i * x_{t-2}[j1] + gamma_i * x_{t-1}[j2] + u_i + eta_i(t) (mod p)`

- `eta_i(t)` is deterministic from seed + tick.
- `u_i` is command control vector derived from `INJ/ROT/MIX/BIAS`.

## 3. Observation
Each executed non-ARM command emits one delayed mixed frame:

- `sig = dot(w_sig0, x_{t+1}) + dot(w_sig1, x_t) + dot(w_sig2, x_{t-2}) + ctrl_mix + nu1(t)`
- `aux = dot(w_aux0, x_{t+1}) + dot(w_aux1, x_{t-1}) + phase*97 + nu2(t)`

Delay is deterministic and profile-dependent.

## 4. Final Gate
- `ARM` is a queued operation (no direct echo).
- On execution, gate checks:
  - `dot(arm_v1, x_t) == target1 (mod p)`
  - `dot(arm_v2, x_t) == target2 (mod p)`
- Failure emits `ARM_FAIL` frame with affine-mixed residuals in `sig/aux` (deterministic, not raw distances).
- Success emits `CHAL` frame with `nonce`, and direct projections `sig=s1`, `aux=s2`.

Proof:
- Use challenge projections directly:
  - `p1 = sig`
  - `p2 = aux`
- Compute checksum with keyword path:
  - `mix = (p1 xor p2) & 0xFFFF`
  - `p3 = CRC16_CCITT(nonce:p1:p2)`

## 5. Determinism and Replay
- No system RNG.
- All noise and delay are deterministic functions of seed and transcript.
- `RESET` restores exact initial hidden state, queue, and gate state.
- Session has deterministic resource ceilings (`MB9_MAX_COMMANDS`, `MB9_MAX_TICK`) to avoid abuse.

## 6. Misleading but Falsifiable Signals
- `lane` is phase-mixed hint, not direct physical lane.
- Observations are delayed; immediate command-to-output assumptions are wrong.
- Low bits look noisy but are replay-stable under same transcript.
- `ARM` queue success is not equivalent to arm state success.

