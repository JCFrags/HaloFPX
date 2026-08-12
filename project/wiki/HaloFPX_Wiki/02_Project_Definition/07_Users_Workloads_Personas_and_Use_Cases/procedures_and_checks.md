---
section_id: "07"
title: "Workload Capture and Validation"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["actual deployment clients and two nodes"]
related_sections: ["09", "46", "69", "78"]
---

# Workload capture and validation

## Privacy-preserving trace schema

**[RECOMMENDATION]** Capture metadata by default, not prompt content:

```text
timestamp, workload_id, pseudonymous_user, model_sha256, template_hash,
prompt_tokens, cached_tokens, generated_tokens, turn_index, concurrency,
queue_ms, ttft_ms, decode_ms, finish_reason, mode, rank_health, cache_result
```

Store content only with explicit consent and a retention rule. Never log API keys, tool secrets, or raw private files.

## Procedure

1. Define a synthetic seed suite for all eight workload IDs; record licenses and hashes.
2. Instrument client-visible latency separately from server timings.
3. Collect at least enough normal-use metadata to estimate median and tail distributions; do not choose the sample count after viewing results.
4. Bucket prompt length, reused prefix, output length, turn count, and concurrency without retaining text.
5. Replay a consented or synthetic corpus under matched single-node, replication, and applicable distributed modes.
6. Measure quality, failure, cancellation, timeout, cache hit/miss, fairness, and recovery—not only tokens/s.
7. Review the corpus whenever client, model, template, tool schema, or deployment audience changes.

## On-machine checks

- Cold first turn versus warm same-conversation and restart restore.
- Simultaneous interactive and batch traffic under backpressure.
- Tool-call schema validity and deterministic-prefix sensitivity.
- User A/User B cache and log isolation.
- Offline startup/inference with external egress blocked.

No root access is required for request replay. Network isolation or system tracing may require administrative configuration and must be documented.

