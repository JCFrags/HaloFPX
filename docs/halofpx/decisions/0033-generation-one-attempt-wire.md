# ADR-0033: generation-one publication-attempt wire

Status: accepted for the default-excluded L08h-a codec boundary. This decision
does not authorize filesystem mutation, publication, server linkage, or a hit.

The protected full-v1 coordinator requires durable state that can distinguish
an in-progress generation-one attempt from terminal success or conclusive
abort after restart. The wire is target-native canonical CBOR and hard-codes
schema one, generation one, and a null predecessor. It binds a fresh 256-bit
attempt ID; store, namespace, and lineage; manifest, ordered object-set, and
aggregate-source commitments; distinct data and anchor root commitments; and
the SHA-256 plus exact byte length of the proposed anchor envelope.

Pending and terminal records use separate KDF, authentication, and envelope
digest domains. Terminal success and abort are distinct authenticated values.
Verification regenerates the one expected canonical envelope and compares all
bytes. The master key is exactly 32 bytes; derived keys and tags are wiped.

The target is `STATIC EXCLUDE_FROM_ALL`, uses fixed bounded buffers, and has no
filesystem, server, restore, or product-registry edge. Sequencing and durable
pending-before-mutation enforcement belong to the next Linux authority seam.
Rollback is one revert. No donor code, dependency, remote, WebUI, or product
admission is introduced.

