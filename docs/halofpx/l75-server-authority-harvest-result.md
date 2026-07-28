# L75 server-authority publication and harvest custody

L75 is **NOT PROMOTED**, with the reviewed default-off source retained.

The source correction makes the outcome of server authority publication an
explicit server-journal fact after the response send. The record binds the
attempt, admission object, execution sequence, split UID/ordinal, backend,
immutable authority path, payload SHA-256, status, and errno without logging
the authority payload, channel key, or execute receipt. Publication failure
remains fatal to the server handler and does not add a client acknowledgement.

The disposable controller now quiesces the owning server before authority
harvest and performs harvest before key or disposable-path cleanup. It derives
the exact authority identity from the invocation-bound server journal, verifies
the helper source hash, source owner/mode/type/size, prepared key
owner/mode/type/hash, authority HMAC chain, exact server grammar, terminal
branch, and journal/record cross-binding. It then creates a same-directory
pending retained file, fsyncs it, atomically publishes it with no-replace
semantics, fsyncs the directory, reopens and compares it, and only then removes
the remote original and staging file. Missing, invalid, collision, tamper,
copy, or status-publication failures are non-promotable while controller
cleanup and final reconciliation continue.

Focused evidence:

- `tests/test_halofpx_l75_server_authority_harvest.py`: 8 passed.
- `tests/test_halofpx_production_transition.py`: 46 passed, 11 subtests passed.
- Linux WSL helper exercise: one authenticated six-record server-success
  authority was validated and durably staged with source mode `0400`, staging
  mode `0600`, 4,200 bytes, and SHA-256
  `4e0c941ef90c282f3ab9a991aabf62a9e6b1302b8b0dc49aecca2bd62fce64ce`.
- Windows feature-off `rpc-server` target built successfully; binary SHA-256
  `193bd2af5e16ca66dc802de971c2047f93331dfebf276ecc3b5c94f156076aa6`.
- Independent exact-source review: PASS, no correctness/security P1/P2.

The promotion gap is qualification-only: this host has no Linux C/C++ compiler
or CMake installation, and HaloFPX correctly refuses the feature-on
configuration on Windows. L75 prohibited production access and Stories/model
runs, so no new real Linux server-handler success/failure fixture was executed.
The accepted L74 real handler/model evidence was not rerun. Therefore the
required new real no-model server publication/harvest gate is not claimed.

No production host was accessed or mutated. No model, Stories, cache,
performance, or protocol/grammar work was performed.
