# Contributing

Changes must preserve the evidence boundary.

## Required for a compatibility change

1. Name the exact component versions, hardware target, OS, kernel, firmware package/files, and workload.
2. Classify the source as official, upstream, community reproducible, maintained community, anecdotal, or inference.
3. Add the source to `sources/source-registry.json` and its YAML/CSV equivalents.
4. Update the dated matrix rather than silently editing a historical release.
5. Include diagnostic output and a minimal reproducer for regressions.
6. Distinguish “works,” “supported,” “production-supported,” “preliminary,” and “experimental.”

## Local checks

```bash
make validate
make render
```

Do not submit proprietary firmware, model weights, copied issue bodies, secrets, host identifiers, or third-party binaries.
