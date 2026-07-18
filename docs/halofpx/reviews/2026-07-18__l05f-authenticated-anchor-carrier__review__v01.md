# L05f authenticated-anchor carrier review v01

- Date: 2026-07-18
- Scope: exact authenticated-anchor carrier, ordinary transition validation,
  exact-envelope CAS, bootstrap boundary, coordinator, and simulator
- Final verdict: **ACCEPT**

## Independent review

The first adversarial pass returned REVISE for one blocker. Two carriers that
declared the same anchor-key ID and generation but were authenticated under
different master keys could satisfy the ordinary-transition field checks. The
wire authentication was valid in isolation, but the coordinator had no local
proof that predecessor and successor came from the same effective authority.

The carrier now retains a private, domain-separated HMAC commitment to its
derived anchor key. It is created only after successful signing or verified
authentication, is never exposed as key material, and is compared with a
fixed-trip volatile accumulator before any backend call. Tests prove that a
same-tuple/different-master successor and current anchor are both rejected.

The independent re-review returned ACCEPT. It also confirmed that the carrier
owns its exact canonical envelope, default and invalid carriers expose no
authenticated state, absent protected state requires explicit bootstrap, and
ordinary transitions reject field changes, wrong generation, wrong
predecessor, wrong manifest binding, and stale exact-envelope CAS identity.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 16/16 |
| Focused inherited CTests | Pass, 7/7 |
| C++ anchor process repetitions | Pass, 100/100 |
| Coordinator process repetitions | Pass, 100/100 |
| Simulator process repetitions | Pass, 100/100; 147,200 core scenarios |
| Independent adversarial review | ACCEPT after one revision |

The build emitted inherited compiler conversion warnings and the expected
OpenSSL-not-found/HTTPS-disabled configuration warning. No reviewed L05f
target is linked into the default server path.

## Promotion boundary

L05f admits only the offline authenticated carrier and ordinary-transition
contract. Administrative bootstrap, protected key registry, concrete storage,
filesystem durability, persistent writes, server integration, and node use
remain closed.
