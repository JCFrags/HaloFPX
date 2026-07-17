# Reference area

This directory holds source inventory, upstream reuse mapping, empty reference slots, and promotion records.

Raw observations belong under `reports/raw/`. A reference becomes usable only after:

1. source, model, fixture, binary, and environment digests are complete;
2. independent repeat runs agree under the declared oracle;
3. calibration and validation observations are disjoint;
4. reviewers approve the reference or tolerance profile;
5. the immutable record is copied here with status `APPROVED`.

The repository ships no fabricated model output, logit vector, quality score, throughput baseline, or statistical threshold.
