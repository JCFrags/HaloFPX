# ADR-0020 read-only prologue independent review

- Date: 2026-07-18
- Result: ACCEPT
- Scope: ADR-0020 authority for portable fake-engine operations 1-5 only
- Reviewed decision SHA-256:
  `927db957e10072c79951be66390f17a2587336bbb0c022afe92b597b081236ec`

## Review outcome

The first review returned REVISE for four blocking ambiguities. The accepted
revision closes each one:

1. The independently admitted credential tuple selects the `K_lab`
   authentication context before record HMAC verification. Persisted key
   fields cannot select their own authentication key.
2. Marker-origin identity is distinguished from fields bound to independent
   under-lock evidence.
3. The operation/effect/completion/primitive-code product for operations 1-5
   is closed, including latent codes for response loss and process death, with
   exact confirmed status mappings.
4. Initialization treatment is limited to an authenticated `initializing`
   marker, and one first-match precedence governs simultaneous quarantine,
   initialization, invalid-recovery, and request-admission conditions.

The reviewer also confirmed that process-death cleanup wording does not claim
cleanup calls by a dead process, 512-slot no-wrap/no-reuse behavior remains
closed, index links are correct, and reserved mutation corrections remain
unimplemented and compile-time unavailable pending explicit ADR-0018 and
ADR-0019 amendments.

The subsequent status-only edit from `proposed` to `accepted` does not change
the reviewed contract. This review authorizes only the read-only prologue in
ADR-0020. It does not authorize mutation, Linux filesystem authority,
initialization, persistent writes, runtime linkage, or positive cache/state
authority.
