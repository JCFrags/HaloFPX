---
upstream_repository: "ROCm/rocm-systems"
upstream_ref: "801a9ca2ad8940ac7cd7d571163e003f3a3d6cab"
accessed_at: "2026-07-18"
source_id: "SRC-ACTIVE-PLUGIN-HEADER"
source_class: "source"
claim_label: "[NORMATIVE_API]"
upstream_path: "projects/rccl/src/include/plugin/nccl_net.h"
upstream_blob_sha: "c7c8d995ef05c771f6be9ff7edf8434e026b3d98"
upstream_locator: "complete file"
snapshot_kind: "verbatim complete file"
completeness: "complete"
license: "Apache-2.0 (per-file SPDX)"
---

```cpp
/*************************************************************************
 * SPDX-FileCopyrightText: Copyright (c) 2017-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * See LICENSE.txt for more license information
 *************************************************************************/

#ifndef NCCL_NET_H_
#define NCCL_NET_H_

#include "nccl.h"
#include "nccl_common.h"
#include "nccl_device/net_device.h"
#include <stdint.h>
#include <dlfcn.h>

#define NCCL_NET_HANDLE_MAXSIZE 128
// Maximum value NCCL can accept for maxP2pBytes and maxCollBytes net properties
#define NCCL_MAX_NET_SIZE_BYTES (1 * 1024 * 1024 * 1024 * 1024L)
#define NCCL_NET_OPTIONAL_RECV_COMPLETION 0x1
#define NCCL_NET_MULTI_REQUEST 0x2

#define MAX_NET_SIZE (1024 * 1024 * 1024L) // Rather than send INT_MAX which is 2G-1, send a power of two.
#define MAX_COLLNET_SIZE (512 * 1024 * 1024L) // Set for initial collent plugins when size was not dynamically queried

#define NCCL_PTR_HOST 0x1
#define NCCL_PTR_CUDA 0x2
#define NCCL_PTR_DMABUF 0x4

#define NCCL_NET_MR_FLAG_FORCE_SO (1 << 0)
#define NCCL_NET_SIGNAL_OP_INC 0x1
#define NCCL_NET_SIGNAL_OP_ADD 0x2

// Maximum number of requests per comm object
#define NCCL_NET_MAX_REQUESTS 32

// Max number of ncclNet objects which can live in the same process
#ifndef NCCL_NET_MAX_PLUGINS
#define NCCL_NET_MAX_PLUGINS 16
#endif

#include "net/net_v12.h"
#include "net/net_v11.h"
#include "net/net_v10.h"
#include "net/net_v9.h"
#include "net/net_v8.h"
#include "net/net_v7.h"
#include "net/net_v6.h"

#define NCCL_NET_MAX_DEVS_PER_NIC NCCL_NET_MAX_DEVS_PER_NIC_V12

typedef ncclNet_v12_t ncclNet_t;
typedef ncclCollNet_v12_t ncclCollNet_t;
typedef ncclNetSGE_v12_t ncclNetSGE_t;
typedef ncclNetProperties_v12_t ncclNetProperties_t;
typedef ncclNetAttr_v12_t ncclNetAttr_t;
typedef ncclNetVDeviceProps_v12_t ncclNetVDeviceProps_t;
typedef ncclNetCommConfig_v12_t ncclNetCommConfig_t;

#define NCCL_NET_PLUGIN_SYMBOL ncclNetPlugin_v12
#define NCCL_COLLNET_PLUGIN_SYMBOL ncclCollNetPlugin_v12

// context passed from RCCL lib to n/w plugin
typedef struct {
  // channel id
  uint32_t chId;
} ncclNet_ctxt_t;

#endif // end include guard
```
