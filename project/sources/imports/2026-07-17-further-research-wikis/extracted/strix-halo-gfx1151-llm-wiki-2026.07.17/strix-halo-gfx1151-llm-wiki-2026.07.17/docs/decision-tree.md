# Backend and stack decision tree

```text
Need AMD-supported current SDK development?
  yes -> ROCm 7.14 Core SDK on Ubuntu 24.04.4 HWE 6.17 or Ubuntu 26.04 kernel 7.0
         -> build llama.cpp from source -> classify application as candidate until accepted
  no  -> Need the most conservative target-specific ROCm prebuilt line?
           yes -> qualifying kernel + fixed firmware + ROCm 7.2.1–7.2.3
                  -> llama.cpp b10064 source or ROCm 7.2 release asset
           no  -> Want maintained Fedora containers?
                    yes -> kernel 6.18.9 + firmware 20260110 + ROCm 7.2.4 community toolbox

Need broad GGUF compatibility and easy A/B?
  -> Build RADV/Vulkan first
  -> Add HIP in the same pinned source tree
  -> Select -dev Vulkan0 / -dev ROCm0 explicitly

Need ROCmFP3/4/6/8 experimental formats?
  -> Use ROCmFPX a5605 exact commit and ROCm 7.2.1 container
  -> Keep its flags and environment isolated from upstream builds

Need two-node transport?
  trusted production-like IP -> thunderbolt_net + TCP
  research native verbs      -> thunderbolt-ibverbs, isolated lab only
```

At every branch, reject known-bad firmware and below-threshold kernels before changing application flags.
