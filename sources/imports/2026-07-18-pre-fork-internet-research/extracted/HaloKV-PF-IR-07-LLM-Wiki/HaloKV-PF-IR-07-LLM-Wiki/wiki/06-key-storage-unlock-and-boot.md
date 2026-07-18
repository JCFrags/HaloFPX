# Key storage, unlock, and boot behavior

## Storage model by key class

| Key class | Long-term location | Runtime exposure | Prohibited location |
|---|---|---|---|
| Authority/root KEK | KMS/HSM or equivalent controlled key authority | Never delivered to rank cache process | Cache disk, image, environment variable, command line, log |
| Tenant/project/prefix root | Random wrapped record in key authority or KMS branch | Normally not delivered; broker derives/unwraps a narrower child | Plaintext cache metadata or broad shared host key file |
| Rank epoch root | Wrapped record bound to rank/namespace/epoch | Delivered only to authorized rank or used inside broker | Persistent plaintext file on cache volume |
| `K_enc`, `K_id`, `K_manifest` | Derived on demand or wrapped short-lived material | Process memory for bounded service lifetime | Swap/core dump where avoidable, telemetry, error strings |
| LUKS2 volume key | LUKS2 keyslots/tokens wrap the key; optional kernel keyring link | Active kernel dm-crypt mapping | Application manifest or tenant object metadata |
| fscrypt master key | Broker/userspace supplies kernel fscrypt key API | Kernel filesystem crypto context; userspace installer may hold a copy temporarily | Unprotected long-lived file colocated with encrypted tree |
| Nonce allocator state | Trusted durable state bound to rank epoch | Rank-local allocator | Rollback-prone object directory without epoch binding |

The lower-layer key and the object-layer keys must not be derived from one another merely for convenience. Their owners, rotation cadence, backup dependencies, and compromise blast radii differ.

## Kernel keyrings and process retention

Linux keyrings can reduce casual userspace exposure and distinguish thread, process, session, user, persistent, and logon-key semantics. They are a retention mechanism, not a complete KMS. Possession, linking, lifetime, revocation, and garbage collection must be designed. A volume key linked into a kernel keyring may outlive mapping detach unless explicitly removed; revoking a key does not prove that derived keys or already decrypted buffers disappeared.

[CLAIM:PFIR07-C028][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-KEYS-7.2RC3 §Keyrings]

[CLAIM:PFIR07-C029][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-KEYS-7.2RC3 §Revocation]

[CLAIM:PFIR07-C026][CLASS:SOURCE][STATUS:SUPPORTED][SRC:SYSTEMD-CRYPTTAB-261.1 link-volume-key option]

## LUKS2 unlock options

The selected systemd `crypttab` release supports several acquisition paths:

* interactive passphrase/PIN;
* key file, including removable or separately mounted media;
* transient AF_UNIX socket provider;
* PKCS#11 token;
* FIDO2 token with the relevant extension;
* TPM2-bound enrollment/unlock.

These options answer **how the host volume key becomes available**. They do not answer which HaloKV tenant/project may decrypt a cache object. Treat automatic unlock as a boot-availability decision with an explicit stolen-host and measured-boot threat model.

[CLAIM:PFIR07-C024][CLASS:SOURCE][STATUS:SUPPORTED][SRC:SYSTEMD-CRYPTTAB-261.1 §Key Acquisition]

### Boot-policy flags

* `headless`: useful when boot must not fall back to an interactive credential path. A missing automated key leaves the volume unavailable rather than waiting for a console secret.
* `noauto`: do not activate automatically; an explicit orchestrator controls lifecycle.
* `nofail`: boot may continue without the volume. For a recomputable cache, continuing with a cold cache is often safer than weakening unlock.
* `password-cache`: account for temporary password-agent caching; disable where the selected workflow and systemd version allow and where availability does not require it.
* `link-volume-key`: avoid unless a named consumer requires it; include explicit key unlink/revoke in shutdown and incident procedures.
* `discard`: default off; a performance exception must acknowledge allocation leakage and cannot be used as deletion evidence.

[CLAIM:PFIR07-C023][CLASS:SOURCE][STATUS:SUPPORTED][SRC:SYSTEMD-CRYPTTAB-261.1 §Description]

[CLAIM:PFIR07-C025][CLASS:SOURCE][STATUS:SUPPORTED][SRC:SYSTEMD-CRYPTTAB-261.1 password-cache option]

## Required boot state machine

```text
POWER_ON
  -> LOWER_VOLUME_LOCKED
  -> activate LUKS2 mapping under host policy (or declare cache unavailable)
  -> mount with restrictive ownership/options
  -> install fscrypt v2 key(s), if configured
  -> authenticate rank workload to principal/key authority
  -> fetch current tenant/project/prefix policy and epoch floors
  -> obtain rank-epoch object keys; never parent roots
  -> initialize or reserve nonce state for current epoch
  -> verify format version and serving index authentication
  -> CACHE_READY
```

Failure at any step leaves the persistent cache non-serving. The authoritative compute path remains available and may operate with a cold/in-memory-only cache. The service must never respond to an unlock failure by mounting or reading a plaintext fallback directory.

## Boot-time rollback and cloning controls

A disk image or VM snapshot can clone both ciphertext and nonce allocator state. On any restored/cloned host:

1. obtain a new authority-issued `rank_id` or allocator-instance ID;
2. create a new epoch key before writes;
3. reject manifests below the current trusted floor;
4. import old objects only through authenticated quarantine validation;
5. never continue the restored counter under the restored AEAD key.

The volume UUID, filesystem UUID, hostname, machine ID, or TPM identity may be useful inputs to host policy, but none alone is sufficient for HaloKV rank identity or nonce uniqueness after cloning.

## Shutdown and emergency lock

Normal shutdown order:

```text
stop cache admission
flush/commit or discard pending immutable objects
publish no further manifests
release rank object keys and nonce ranges
zeroize process buffers and terminate workers
remove fscrypt keys after closing files, if used
unmount cache filesystem
close dm-crypt mapping
remove linked kernel keys, if configured
verify expected mappings/key references are absent
```

If busy state prevents complete closure, record shutdown/revocation as incomplete and escalate. Do not report key destruction solely because the service sent a revoke or close request.
