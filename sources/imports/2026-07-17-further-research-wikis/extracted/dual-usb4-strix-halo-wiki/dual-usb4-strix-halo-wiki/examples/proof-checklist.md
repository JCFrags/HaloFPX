# Dual-USB4 proof checklist

## Identity

- [ ] Node A and B board/BIOS/firmware recorded.
- [ ] Kernel and kernel config recorded.
- [ ] Cable type/length recorded.
- [ ] Each connector mapped one cable at a time.

## Topology

- [ ] Two USB4NET netdevs on node A.
- [ ] Two USB4NET netdevs on node B.
- [ ] Adding cable 2 does not remove or replace cable 1's XDomain/netdev on either node.
- [ ] Two distinct XDomains/services on each node.
- [ ] Two distinct USB4 domains on each node, or shared-domain limitation stated.
- [ ] Two distinct NHI PCI BDFs on each node, or shared-controller limitation stated.

## Correctness

- [ ] Separate /30 subnets.
- [ ] Source-specific route lookup selects correct interface.
- [ ] MTU verified with DF ping.
- [ ] No unexplained errors/CRC growth.

## Performance

- [ ] Isolated link 0 results, both directions.
- [ ] Isolated link 1 results, both directions.
- [ ] Concurrent two-link results.
- [ ] CPU-seconds/GiB and softirq data.
- [ ] MTU/offload settings recorded.

## Fault isolation

- [ ] Cable 0 removal leaves cable 1 operational.
- [ ] Cable 1 removal leaves cable 0 operational.
- [ ] No shared controller reset.

## MPTCP

- [ ] Both application endpoints create MPTCP sockets.
- [ ] Two intended subflows are established.
- [ ] Both subflows carry payload bytes.
- [ ] One-cable failure does not close the meta-socket.
- [ ] Reconnect limitation documented.

## Security

- [ ] RPC listener restricted to dedicated fabric/namespace.
- [ ] nftables peer filter installed.
- [ ] USB4 domain security and IOMMU status captured.
- [ ] Encryption decision documented.
