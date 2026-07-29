# L94 terminal result

Classification: **NOT PROMOTED**. The single authorized primary transition was
consumed. No retry or runtime semantic correction occurred.

## Focused corrections

The restore-canary journal lower bound now uses `exact_journal_cursor()` and is
durably retained before launch. Postcleanup shared-port ownership remains
fail-closed and requires one different manifest-admitted disposable unit whose
retained launch PID/InvocationID and live systemd unit/cgroup/MainPID all
cross-bind.

Focused qualification: `54 passed`; Python compilation and diff checking
passed. Independent pre-runtime review found no P1/P2.

Exact identities:

- child SHA256:
  `7be6fd821d6927f81b5dc82ca7c1e6c2f988ac0b25ab26ca60c1a49b97ce74bb`
- source root:
  `3f139739aee870376d2f7badcc73b73e25dc000fe914bbeef13ddec866cb9520`
- build ID:
  `6f63bb1d95d0a7451800798356e58c33cc1a703c54c290f66e73bf18e86df086`
- worker binary:
  `125c6a2d9fd7ffa77e251042f418e30bee495e8e7599e0ccdcea18fcaf797648`
- canary binary:
  `8964d543591fb6219839c6a33df5ad97c6828dcc4fb3f351e0dbbed9c84c2f78`
- source archive: 197995520 bytes, SHA256
  `9f768239b49e747e0689240108134cd8540645b2b36fb82ee873607a66d097aa`

## Runtime boundary

Residency A again completed authenticated capture with deterministic token
`21549`, suffix `alpha`, and four authenticated 4200-byte server authorities.
A fresh restore worker launched and was admitted.

The new cursor receipt proves the corrected seam completed:

- return code `0`;
- cursor
  `s=366c77678d0e420e926e015e6692f00b;i=149863;b=4511b107942944c28e81c6b7a54c376b;m=1581b1a7792;t=657b708d1737a;x=686bc5ff1b3ee16d`;
- stdout SHA256
  `f7ae383d7ae2b30c916947b047bae84d4597923f6c29ca5898e41be8b2d52a44`;
- empty stderr SHA256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.

The restore `systemd-run` command returned success and placed its exact
InvocationID in stderr while stdout was empty. The current caller searched
stdout only and stopped at:

`restore canary launch InvocationID is unavailable`

Finally cleanup also retained `shared listener alternate owner identity
mismatch`. Cleanup and controller recovery nevertheless removed all disposable
resources. There is no restored token, residency-B state/component equality,
zero-GET/SET conclusion, or cache-correctness result.
