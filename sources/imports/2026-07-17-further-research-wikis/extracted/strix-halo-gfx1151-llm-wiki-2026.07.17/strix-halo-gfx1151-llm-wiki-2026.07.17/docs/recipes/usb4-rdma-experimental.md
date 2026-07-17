# Recipe: experimental native verbs over USB4

**Classification:** research-only, explicitly buggy and insecure  
**Pin:** `76ba39b630a70accb72f19388eefe48844b50eb8`  
**Source:** [THUNDERBOLT-IBVERBS-76BA39B](../sources.md#thunderbolt-ibverbs-76ba39b)

> [!DANGER]
> Use only in an isolated lab with trusted hosts, cables, and physical access. Do not combine with a blanket `amd_iommu=off` profile.

## Kernel requirement

Use Linux 6.14 or newer or the project’s patched `linux-thunderbolt` kernel. Install matching headers and DKMS tools.

```bash
sudo apt-get install -y build-essential dkms git kmod \
  "linux-headers-$(uname -r)" rdma-core perftest
```

## Pinned source build

```bash
git clone https://github.com/hellas-ai/thunderbolt-ibverbs.git
cd thunderbolt-ibverbs
git checkout 76ba39b630a70accb72f19388eefe48844b50eb8
sudo make dkms-add
sudo make dkms-build
sudo make dkms-install
```

Alternatively, pin the Nix flake revision:

```bash
nix build github:hellas-ai/thunderbolt-ibverbs/76ba39b630a70accb72f19388eefe48844b50eb8#thunderbolt-ibverbs
nix build github:hellas-ai/thunderbolt-ibverbs/76ba39b630a70accb72f19388eefe48844b50eb8#rdma-core-usb4
```

## Load

```bash
sudo modprobe thunderbolt_ibverbs \
  profile=linux_perf \
  bind_services=1 \
  allocate_rings=1 \
  start_rings=1 \
  negotiate_native=1 \
  enable_tunnels=1 \
  register_verbs=1
```

Verify:

```bash
dmesg | grep thunderbolt_ibverbs
ibv_devices
rdma link
```

## Perftest

```bash
# Peer A
ib_write_bw -d usb4_rdma0

# Peer B
ib_write_bw -d usb4_rdma0 PEER_A_ADDRESS
```

Capture `/sys/kernel/debug/thunderbolt_ibverbs/summary` before and after the test.

## Container

The module remains on the host. Pass the verbs devices and provider into a stock compute image:

```bash
docker run --rm -it \
  --device=/dev/infiniband \
  --cap-add=IPC_LOCK \
  --ulimit memlock=-1 \
  IMAGE
```

Inside the image, install the provider package matching the image’s Ubuntu codename and run `ibv_devices`.

## Rollback

```bash
sudo modprobe -r thunderbolt_ibverbs
sudo make dkms-remove
```

Reboot into the distribution kernel if a patched test kernel was used. Preserve logs before rollback.
