# L92 unit-guard authority rehearsal

Base: `2b0910b3b0ac80b01901838e2f26773194aa3248`.

The first exact-set rehearsal found one stale authority entry:
`nimo-2 / halofpx-l48-canary-first-chunk / no port`. The exact validated L77
child argv no longer selects `--l55-first-chunk`, so that unit is unreachable
from the full correctness path. L92 removed only this stale manifest entry.

The corrected no-host rehearsal used:

- `halofpx-production-transition.py::child_environment()` with the exact L77
  manifest;
- `configure_l77_primary()` and `install_unit_guard_authority()` from the real
  child;
- the real device, worker, canary, and restore-canary launch builders;
- the same `_systemd_run_guard_tuple()` used by `ssh()` before transport.

Corrected installed set:

1. `nimo-1 / halofpx-l50-device-gate / 50249`
2. `nimo-1 / halofpx-l48-worker-capture / 50248`
3. `nimo-1 / halofpx-l48-worker-restore / 50248`
4. `nimo-2 / halofpx-l48-canary-capture / null`
5. `nimo-2 / halofpx-l48-canary-restore / null`

Canonical authority SHA256:
`769b1e2b713c1f70ac44d91c0093d61df30895a0944e4717065849debcb15cc1`.

The 12-step device → capture → stop → restore → finally-cleanup projection had
exact set equality, every membership result was true, and a second independent
projection produced the same ordered records and authority hash.

Focused test command:

`python -m pytest -q tests/test_halofpx_l91_unit_authority.py tests/test_halofpx_l60_transient_unit_guard.py`

Result: `35 passed`.

Every real L77 invocation now publishes `unit-guard-authority.json` before SSH,
publishes the complete ordered rehearsal before SSH, and durably records every
requested canonical tuple/phase/hash/membership before membership refusal.

Runtime limitation discovered by the sole authorized attempt: the rehearsal
derived cleanup ports from the launch builders. It did not execute Python's
already-bound default value for `stop_worker(unit, port=PORT)`. That default was
bound to 50184 at module definition, before `configure_l77_primary()` changed
the global to 50248. The rehearsal therefore passed while real capture cleanup
requested 50184. This limitation is preserved; L92 does not correct it.
