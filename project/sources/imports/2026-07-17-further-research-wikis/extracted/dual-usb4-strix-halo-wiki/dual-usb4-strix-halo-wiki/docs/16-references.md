# References and source snapshot

Accessed 2026-07-17 unless noted. Primary sources are preferred; field reports are labeled.

## Linux kernel and USB4

1. Linux Kernel Archives — current releases: <https://www.kernel.org/>
2. Linux USB4 and Thunderbolt administration guide: <https://docs.kernel.org/admin-guide/thunderbolt.html>
3. USB4 sysfs ABI: <https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-bus-thunderbolt>
4. USB4 Kconfig, current mainline: <https://github.com/torvalds/linux/blob/master/drivers/thunderbolt/Kconfig>
5. USB4 Kconfig, Linux 7.1: <https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/Kconfig>
6. USB4NET Kconfig, Linux 7.1: <https://github.com/torvalds/linux/blob/v7.1/drivers/net/thunderbolt/Kconfig>
7. USB4NET driver, Linux 7.1: <https://github.com/torvalds/linux/blob/v7.1/drivers/net/thunderbolt/main.c>
8. USB4NET driver, Linux 6.18: <https://github.com/torvalds/linux/blob/v6.18/drivers/net/thunderbolt/main.c>
9. USB4NET driver, Linux 7.0: <https://github.com/torvalds/linux/blob/v7.0/drivers/net/thunderbolt/main.c>
10. USB4STREAM driver, current mainline: <https://github.com/torvalds/linux/blob/master/drivers/thunderbolt/stream.c>
11. XDomain core, Linux 7.1: <https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/xdomain.c>
12. Firmware/ICM connection manager, Linux 7.1: <https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/icm.c>
13. Software connection manager, Linux 7.1: <https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/tb.c>
14. NHI connection-manager selection, Linux 7.1: <https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/nhi.c>
15. Linux networking pull context for 7.0 bonding-enabling changes: <https://lore.kernel.org/netdev/>

## Linux networking

16. MPTCP kernel documentation: <https://docs.kernel.org/networking/mptcp.html>
17. MPTCP path-manager guide: <https://www.mptcp.dev/pm.html>
18. MPTCP application setup and `mptcpize`: <https://www.mptcp.dev/setup.html>
19. MPTCP v1 standard, RFC 8684: <https://www.rfc-editor.org/rfc/rfc8684.html>
20. MPTCP sysctls: <https://docs.kernel.org/networking/mptcp-sysctl.html>
21. `ip-mptcp(8)`: <https://man7.org/linux/man-pages/man8/ip-mptcp.8.html>
22. Linux bonding HOWTO: <https://docs.kernel.org/networking/bonding.html>
23. Linux networking scaling, RPS/RFS/XPS: <https://docs.kernel.org/networking/scaling.html>
24. Linux NAPI documentation: <https://docs.kernel.org/networking/napi.html>
25. Linux IPv4 sysctls, ECMP and reverse-path filtering: <https://docs.kernel.org/networking/ip-sysctl.html>
26. `ip-rule(8)`: <https://man7.org/linux/man-pages/man8/ip-rule.8.html>

## llama.cpp

27. llama.cpp RPC README: <https://github.com/ggml-org/llama.cpp/blob/master/tools/rpc/README.md>
28. llama.cpp RPC transport implementation: <https://github.com/ggml-org/llama.cpp/blob/master/ggml/src/ggml-rpc/transport.cpp>
29. llama.cpp RPC server options: <https://github.com/ggml-org/llama.cpp/blob/master/tools/rpc/rpc-server.cpp>
30. llama.cpp security policy: <https://github.com/ggml-org/llama.cpp/blob/master/SECURITY.md>

## Hardware and experimental transports

31. AMD Ryzen AI Max+ 395 product page — two native 40 Gbit/s USB4 ports: <https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html>
32. rdma-core — RXE/Soft-RoCE configuration: <https://github.com/linux-rdma/rdma-core/blob/master/Documentation/rxe.md>
33. rdma-core repository and supported providers: <https://github.com/linux-rdma/rdma-core>
34. `thunderbolt-ibverbs` field/research series, non-upstream: <https://blog.hellas.ai/blog/thunderbolt-ibverbs/4-thunderbolt-ibverbs/>
35. Strix Halo Wiki clustering page, community observation only: <https://strixhalo.wiki/AI/Clustering>

## Source interpretation notes

- Kernel source statements are tied to the branches above and can change.
- The Linux USB4 guide visible in the 7.2 development documentation includes USB4STREAM; Linux 7.1 source does not.
- The AMD product page establishes SoC capability, not a specific board's controller/domain topology.
- Community throughput and PCI listings are examples, not acceptance targets.
- Current release numbers and security advice should be re-checked before deployment.
