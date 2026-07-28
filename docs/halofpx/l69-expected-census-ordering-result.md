# L69 expected-census ordering and Stories15M replacement

Result: **NOT PROMOTED**

L69 corrected the source-proven expected-census installation order without
reopening ADR-0049. The client and server now copy the authenticated
scheduler-logical register/exclude cardinalities from the fully verified sealed
admission into each attempt recorder before the first registration or
exclusion. The counts are never derived from observed events.

The focused no-model gate passed:

- a sealed `2 register / 1 exclude` census completed registration/exclusion and
  terminalized through the exact pre-prepare abort production;
- a sealed `1 / 1` census with an observed `2 / 1` stream refused;
- the existing exact pre-registration abort production remained valid;
- the retained verifier authenticated two terminal attempts and 11 records.

The independent review found no correctness/security P1/P2 after one narrow
runner correction that prevents vertical modes from entering the unrelated
legacy capture/restore result writer.

The retained L68 feature-off control was not rerun: token `29916`, output `x`
(`78` hex), and empty authority.

The single authorized feature-on Stories15M replacement was consumed once. It
failed before authenticated execute with canary exit status `42`. The exact
new classification is a sealed-census mismatch during mutable preparation:
the client observed 11 register and 36 exclude events against authenticated
expected cardinalities 28 and 38. Exact grammar correctly refused the abort
record instead of accepting or guessing the partial census. The server-side
disconnect path independently recorded `0/28` register and `0/38` exclude and
also refused an invalid terminal production. `llama_decode` returned `-3`; no
token/output or execute/consume authority was produced.

No retry was attempted. No cache, primary-model, tuning, or performance work
occurred. Disposable units, keys, listeners, work paths, and staged source were
removed. Production preflight/final snapshots are byte-identical and retain the
same PIDs, listeners, HTTP 200 state, and zero restarts.

The source correction is retained because its focused contract and independent
review passed. Product promotion remains blocked on the newly classified
semantic mismatch between the sealed whole-graph logical census and the
registration stream that terminates early during the real model request.

