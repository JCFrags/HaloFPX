# Security

## Threat model

GGUF files, cache/state files, API requests, RPC peers, model metadata, templates, filenames, and network clients are untrusted inputs unless a lane explicitly proves otherwise.

## Test-environment rules

- Run server and RPC fixtures on loopback or isolated networks.
- Never bind the RPC proof-of-concept to a public or untrusted interface.
- Verify model and fixture hashes.
- Use dedicated temporary directories and reject path traversal.
- Do not run destructive OOM, filesystem, or device-loss tests on shared hosts.
- Redact credentials, authorization headers, secrets, and full hostile bodies.
- Use ASan/UBSan for malformed GGUF/state and selected API fuzz seeds.
- Apply process-tree watchdogs.
- Retain crash evidence without exposing confidential model or prompt data.
- Review external model licenses and provenance before mirroring.

## Security regressions represented

- out-of-range special-token metadata in GGUF;
- impossible counts and size overflows;
- malformed tensor descriptors and truncated payloads;
- cache length overflow, truncation, version mismatch, and corrupted publication;
- cross-user cache namespace access and traversal attempts;
- malformed/oversized HTTP input and log redaction;
- RPC public-binding detection, disconnect, and protocol mismatch;
- cancellation and partial-state activation;
- sanitizer-clean expected-failure corpus.

## Reporting

Treat a sanitizer finding, out-of-bounds access, use-after-free, uncontrolled allocation, authentication/isolation bypass, accepted corrupted state, or public RPC exposure as a security failure. Follow the affected project's security policy rather than opening a public issue with exploit details.
