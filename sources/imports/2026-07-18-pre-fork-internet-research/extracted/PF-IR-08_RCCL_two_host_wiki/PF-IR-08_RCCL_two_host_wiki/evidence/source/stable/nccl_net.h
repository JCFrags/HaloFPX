---
upstream_repository: "ROCm/rccl"
upstream_ref: "96a25b5fd6f73fba58c7d83eb57cf19a50230aa4"
accessed_at: "2026-07-18"
source_id: "SRC-STABLE-PLUGIN-HEADER"
source_class: "source"
claim_label: "[NORMATIVE_API]"
upstream_path: "src/include/plugin/nccl_net.h"
upstream_blob_sha: "de271219b010a5d201113605e6a2a407fb5fafd7"
upstream_locator: "complete file"
snapshot_kind: "verbatim complete file"
completeness: "complete"
license: "RCCL LICENSE.txt"
---

```cpp
/*************************************************************************
 * Copyright (c) 2017-2022, NVIDIA CORPORATION. All rights reserved.
 *
 * See LICENSE.txt for license information
 ************************************************************************/

#ifndef NCCL_NET_H_
#define NCCL_NET_H_

#include "nccl.h"
#include "nccl_common.h"
#include "net_device.h"
#include <stdint.h>
#include <dlfcn.h>

#define NCCL_NET_HANDLE_MAXSIZE 128
//Maximum value NCCL can accept for maxP2pBytes and maxCollBytes net properties
#define NCCL_MAX_NET_SIZE_BYTES (1*1024*1024*1024*1024L)
#define NCCL_NET_OPTIONAL_RECV_COMPLETION 0x1

#define MAX_NET_SIZE (1024*1024*1024L) // Rather than send INT_MAX which is 2G-1, send a power of two.
#define MAX_COLLNET_SIZE (512*1024*1024L) //Set for initial collent plugins when size was not dynamically queried

#define NCCL_PTR_HOST 0x1
#define NCCL_PTR_CUDA 0x2
#define NCCL_PTR_DMABUF 0x4

// Maximum number of requests per comm object
#define NCCL_NET_MAX_REQUESTS 32

// Max number of ncclNet objects which can live in the same process
#ifndef NCCL_NET_MAX_PLUGINS
#define NCCL_NET_MAX_PLUGINS 16
#endif

#include "net/net_v10.h"
#include "net/net_v9.h"
#include "net/net_v8.h"
#include "net/net_v7.h"
#include "net/net_v6.h"

typedef ncclNet_v10_t ncclNet_t;
typedef ncclCollNet_v10_t ncclCollNet_t;
typedef ncclNetSGE_v10_t ncclNetSGE_t;
typedef ncclNetProperties_v10_t ncclNetProperties_t;
typedef ncclNetVDeviceProps_v10_t ncclNetVDeviceProps_t;
typedef ncclNetCommConfig_v10_t ncclNetCommConfig_t;

#define NCCL_NET_MAX_DEVS_PER_NIC NCCL_NET_MAX_DEVS_PER_NIC_V10

#define NCCL_NET_PLUGIN_SYMBOL ncclNetPlugin_v10
#define NCCL_COLLNET_PLUGIN_SYMBOL ncclCollNetPlugin_v10

// context passed from RCCL lib to n/w plugin
typedef struct {
  // channel id
  uint32_t chId;
} ncclNet_ctxt_t;

#endif // end include guard
```
