# Persistent KV-Cache Architecture and Integrity

<span class="badge observed">SOURCE-PINNED REVIEW</span> <span class="badge recommended">HALOFPX REDESIGN</span>

This Wiki audits CachyLLama's SSD-backed checkpoint cache, compares it with current llama.cpp persistence and other primary-source prefix-cache designs, and defines a fail-closed HaloFPX storage architecture.

> **Hit predicate:** `AUTHORIZED ∧ SUPPORTED ∧ BOUNDED ∧ COMPATIBLE ∧ COMPLETE ∧ CATALOG_AUTHENTICATED ∧ BYTES_VERIFIED ∧ ENGINE_ACCEPTED`. Any false or unknown term yields **`MISS_RECOMPUTE`**.

## Start here

1. [Executive findings](docs/executive-findings.md)
2. [Observed CachyLLama implementation](docs/observed-cachyllama.md)
3. [Observed llama.cpp functionality](docs/observed-llama-cpp.md)
4. [Comparative designs](docs/comparative-designs.md)
5. [HaloFPX redesign](docs/halofpx-redesign.md)
6. [Integrity invariants](docs/integrity-invariants.md)
7. [Failure and recovery](docs/failure-and-recovery.md)
8. [Executable validation](docs/validation.md)

The pre-rendered version is at [`site/index.html`](site/index.html). Exact source commits are locked in [`research/source-lock.json`](research/source-lock.json).
