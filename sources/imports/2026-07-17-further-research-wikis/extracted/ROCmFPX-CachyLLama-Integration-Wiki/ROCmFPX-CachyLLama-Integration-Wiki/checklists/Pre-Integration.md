# Pre-Integration Checklist

- [ ] Maintainer confirmed `fewtarius/CachyLLama` is the intended donor.
- [ ] Canonical/upstream/donor/parent heads are immutable commit IDs.
- [ ] Signed integration-base tag and rollback branch exist.
- [ ] Canonical feature-off build and cache tests are green.
- [ ] Every selected capability has a provenance record.
- [ ] GPL-parent boundary reviewed.
- [ ] Direct cherry-pick roster explicitly approved (may be empty).
- [ ] Conflict owners assigned for all Critical/High paths.
- [ ] Lane dependencies and rollback tags approved.
- [ ] Existing `--cache-disk*` behavior is captured by contract tests.
- [ ] No patch/diff enters before P3 provenance.
