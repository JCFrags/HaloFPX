# L93 terminal result

Classification: **NOT PROMOTED**. The single authorized primary transition was
consumed. No retry or runtime semantic correction occurred.

## Mechanical correction

`stop_worker()` now requires an explicit valid port. L77 cleanup resolves the
port from the installed manifest authority and refuses an absent, ambiguous, or
non-integer binding. The real no-host rehearsal invokes the actual worker and
canary cleanup admission seams. It retained twelve ordered requests with exact
set equality and stable authority SHA256
`769b1e2b713c1f70ac44d91c0093d61df30895a0944e4717065849debcb15cc1`.

Focused result: `39 passed`; Python compilation and diff checking passed. The
independent pre-runtime review found no P1/P2.

Exact source/build identities:

- child SHA256:
  `55b5126a4a222da6bbffa4f2e9e54f4d17634dd7fda747b13175e439bb75f31a`
- source root:
  `5f1e0e4984f79effe3678dbddca62d89d82091c025316cd8273cd6704f62b996`
- build ID:
  `07b3c3c709a51911fc836bfad5f5f6a56f261649c3c1ff017ec8ca985106fdd7`
- worker binary:
  `1b1b8dfad0e1b99fc81746b45f21480959c3aeea1aca20c1899cdb71e8d708c0`
- canary binary:
  `4216e2bd872aca131c26573ff71b9bfb9e2133cb41ff3d709a9b5b21ec916b6b`
- source archive: 195402752 bytes, SHA256
  `38e52a586d3b75224846638dd31dc0a6a711932b96d289342471b3d50a2159f2`

## Runtime result

Residency A completed authenticated capture:

- deterministic token `21549`;
- suffix `alpha`;
- capture object/state receipts were retained;
- four server authority files, each exactly 4200 bytes, authenticated and
  retained by the L76 custody path;
- all runtime unit-guard requests used the admitted ports (`50249` or `50248`)
  or the exact null canary port, and all membership results were true.

A genuinely fresh restore worker was launched and admitted. Before the restore
canary could launch, the child stopped at:

`restore canary journal lower bound is unavailable`

Finally cleanup additionally reported:

`halofpx-l48-worker-capture: transient unit ... is active or still owns resources`

The controller nevertheless harvested the four capture server authorities,
removed all disposable units/keys/paths, and recovered production. There is no
residency-B token, represented-state equality, or cache-correctness conclusion.

Two earlier controller invocations are preserved separately. The first refused
while binding an omitted manifest child argv. The second used disposable mode,
which deliberately left production active; the child refused at its
`production coordinator is not inactive` precondition. Neither invocation
stopped production or loaded the primary artifact, so neither consumed the
authorized primary transition.
