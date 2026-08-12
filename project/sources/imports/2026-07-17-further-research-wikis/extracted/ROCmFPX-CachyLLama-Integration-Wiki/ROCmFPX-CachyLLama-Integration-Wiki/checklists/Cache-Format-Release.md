# Cache Format Release Checklist

- [ ] `FORMAT` major/minor rules documented and tested.
- [ ] Duplicate-key rejecting bounded manifest parser fuzzed.
- [ ] Component length/digest and required-feature checks pass.
- [ ] Strong target/draft model-set fingerprints generated.
- [ ] Tokenizer/template/KV/layout mismatch tests reject.
- [ ] Atomic staging → commit fault matrix passes.
- [ ] No partial target/draft/spec/recurrent entry can be accepted.
- [ ] Owner-only permissions and safe path handling pass on supported platforms.
- [ ] Unknown major and future minor behavior pass.
- [ ] Old reader/new writer rollback matrix documented.
- [ ] Migration is side-by-side, never in-place.
- [ ] Donor importer, if built, is offline/read-only on source and disabled by default.
