# Source snapshot

**Observed:** 2026-07-17. This file records the exact temporal and source-code boundary used by the wiki. Re-check current releases and source before deployment.

## Release boundary

| Item | Snapshot value |
|---|---|
| Linux stable | 7.1.3 |
| Linux mainline | 7.2-rc3 |
| Linux LTS used in compatibility matrix | 6.18.38 |
| USB4NET bonding-enabling source boundary | Linux 7.0 |
| USB4STREAM source boundary | Linux 7.2 development line; absent from upstream 7.1 Kconfig |
| MPTCP `laminar` kernel boundary | Linux 6.18 plus matching iproute2 support |

## Reviewed source objects

| Project/ref | File | Blob SHA |
|---|---|---|
| Linux `v7.1` | `drivers/net/thunderbolt/main.c` | `7aae5d915a1ebc1c6ef6f686df3514cab796c009` |
| Linux `v6.18` | `drivers/net/thunderbolt/main.c` | `dcaa62377808c290fa3103347f6089c942f04057` |
| Linux `v7.1` | `drivers/thunderbolt/Kconfig` | `db3b0bef48f4c30be6b27e11a2ff9d1ba072c3fe` |
| Linux `v7.1` | `drivers/net/thunderbolt/Kconfig` | `e127848c8cbd10cc4919e1041fddd3d190fd1166` |
| Linux mainline | `drivers/thunderbolt/Kconfig` | `294b3227a54582536433cbb391057b99bb9df352` |
| Linux mainline | `drivers/thunderbolt/stream.c` | `c1f5c55583d069c811d25df95f4e90136255d585` |
| Linux mainline | `Documentation/admin-guide/thunderbolt.rst` | `91a6cb1099889175d5cff3aac16961228cc1da2e` |
| Linux `v7.1` | `drivers/thunderbolt/xdomain.c` | `1fd1cf4295a2a58f307d2a6ed7d47899f962adc4` |
| Linux `v7.1` | `drivers/thunderbolt/icm.c` | `9d95bf3ab44c103828dc8871115bcd2304df92ea` |
| Linux `v7.1` | `drivers/thunderbolt/tb.c` | `c69c323e6952a36d98bb82f8d1c47f77fd25718e` |
| Linux `v7.1` | `drivers/thunderbolt/nhi.c` | `2bb2e79ca3cb35df1565c35ad22c0f52a41454f9` |
| llama.cpp `master` | `tools/rpc/README.md` | `655b65347e2dcde76cb630e75fcc9ecc6ed9cb34` |
| llama.cpp `master` | `ggml/src/ggml-rpc/transport.cpp` | `a728152421f7dac44baefc582d713540398dabe4` |
| llama.cpp `master` | `tools/rpc/rpc-server.cpp` | `08e680391415f24e7e3c6c547aa4b517e535840a` |

Blob IDs identify reviewed file contents, not endorsement of a branch's later state. The complete URL list is in [References](docs/16-references.md) and the machine-readable index is `data/source-index.json`.
