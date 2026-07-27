# L49 independent adversarial review

Verdict: **ACCEPT — terminal NOT PROMOTED**

The pre-runtime review initially refused the privileged readiness operation
because it validated timing flags but not the complete argv. The corrected
implementation admitted only the two exact L48 feature-on/off token vectors,
including the fixed script, endpoint, option order, 120-second application
budget, identity/key fields, and no extras, before process creation. Focused
refusals, refreshed hashes, the 54-test affected suite, and a fresh 47-operation
dry-run passed. The reviewer then returned GO for the single disposable run.

The sole runtime did not reach the intended discriminator. Transport record
24 independently proves that the corrected transport class supplied the
150-second outer deadline and retained the 120-second inner budget. It was not
locally timed out or killed. Instead, the probe returned after 120.198851
seconds with 123 `connect-failed` attempts. Record 23 proves only transient
unit submission; records 25 and 26 show the unit failed with `MainPID=0` and
no port 50248 listener.

No HELLO/HFXCAP2 receipt or authenticated composed result exists. Therefore
the review accepts no model, cache, graph, scheduler, mutable-input, replay,
token, or state-transfer conclusion. The exact early-exit cause remains
unknown because worker journal, exit code/status, and unit stderr were not
retained. Any attribution to the binary, key, ROCm, systemd, controller, or
protocol would exceed the evidence.

All retained raw-evidence hashes verify. Cleanup records prove the disposable
unit, listener, key, and every allowlisted path absent. Production
preflight/final snapshots are byte-identical and show the exact unchanged
nimo-2 worker PID 1535639 on 50052 and nimo-1 coordinator PID 2356329 on 8081
with HTTP 200, both NRestarts 0.

The unqualified reconstructed runtime candidate was removed. L49 is correctly
closed as NOT PROMOTED and does not make the primary controller preflight-ready.

