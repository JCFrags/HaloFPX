# Reference address plan

| Link | Node A interface/address | Node B interface/address | Purpose |
|---|---|---|---|
| Cable 0 | `thunderbolt0`, `10.44.0.1/30` | `thunderbolt0`, `10.44.0.2/30` | Initial TCP/MPTCP subflow |
| Cable 1 | `thunderbolt1`, `10.44.1.1/30` | `thunderbolt1`, `10.44.1.2/30` | Additional MPTCP subflow |
| Optional bond lab | `bond0`, `10.44.2.1/30` | `bond0`, `10.44.2.2/30` | Mutually exclusive with per-link addresses |

Replace interface names with persistent names after physical mapping. Do not reuse the same subnet on both cables.
