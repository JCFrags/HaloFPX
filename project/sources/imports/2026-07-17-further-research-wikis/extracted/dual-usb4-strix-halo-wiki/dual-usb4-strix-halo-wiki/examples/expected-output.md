# Expected output patterns

These are shapes, not exact device names or performance targets.

## Strong topology evidence

```text
interface      domain   NHI BDF        XDomain  service  remote UUID
thunderbolt0   domain0  0000:c8:00.5   0-1      0-1.0   <peer-domain-uuid-0>
thunderbolt1   domain1  0000:c8:00.6   1-1      1-1.0   <peer-domain-uuid-1>
```

## Correct route selection

```text
10.44.0.2 from 10.44.0.1 dev thunderbolt0 src 10.44.0.1
10.44.1.2 from 10.44.1.1 dev thunderbolt1 src 10.44.1.1
```

## MPTCP evidence

`ss -Mani` shows one MPTCP socket; root `ss -tani` shows two TCP subflows with the intended address pairs. Both `ip -s link` byte counters grow during one application operation.

## Insufficient evidence

```text
thunderbolt0 UP
thunderbolt1 UP
```

Two interface names alone do not prove controller, fault, capacity, or application-path independence.

## Failed same-peer coexistence gate

```text
insert cable 1 -> thunderbolt0 / XDomain 0-1 exists
insert cable 2 -> XDomain 0-1 disappears and only the new route/netdev remains
```

This is a failure before IP configuration. A firmware/ICM-managed domain can replace a same-UUID XDomain at a new route; bonding and MPTCP cannot recover a path that Linux does not expose concurrently.
