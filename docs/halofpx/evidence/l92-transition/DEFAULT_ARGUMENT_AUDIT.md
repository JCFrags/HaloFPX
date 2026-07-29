# L92 source-only Python default-argument audit

An AST scan of `scripts/**/*.py` found two function defaults referencing
uppercase globals:

1. `scripts/halofpx-l13-primary-retry.py`: `stop_worker(..., port=PORT)`.
   `PORT` is reassigned by every fixture/primary configuration function. This is
   the proven L92 defect.
2. `scripts/snapdragon/qdc/run_qdc_jobs.py`:
   `wait_for_capacity(..., max_jobs=MAX_CONCURRENT_JOBS)`.
   `MAX_CONCURRENT_JOBS` is assigned once to 5 and is not mutated or
   configuration-selected in that module. No analogous stale-default defect is
   source-proven.

No correction was made under this read-only/source-only audit authority.
