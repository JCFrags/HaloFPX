#!/usr/bin/env bash
# Re-fetch public primary sources pinned by PF-IR-11.
#
# This script requires an internet-connected host. It does not download:
#   * AMD account-gated Ryzen AI DEB/TGZ packages
#   * NPU firmware binaries
# It fetches public source text only.
set -euo pipefail
umask 022

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEST="${1:-$ROOT/fetched}"
mkdir -p "$DEST"
MANIFEST="$DEST/fetch-manifest.tsv"
: >"$MANIFEST"
printf 'repository\tcommit\tpath\turl\texpected_git_blob_sha1\tactual_git_blob_sha1\tsha256\tstatus\n' >>"$MANIFEST"

need() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "ERROR: required command not found: $1" >&2
    exit 2
  }
}
need curl
need python3
need sha256sum

git_blob_sha1() {
  python3 - "$1" <<'PY'
import hashlib, pathlib, sys
data=pathlib.Path(sys.argv[1]).read_bytes()
h=hashlib.sha1()
h.update(f"blob {len(data)}\0".encode())
h.update(data)
print(h.hexdigest())
PY
}

fetch_git() {
  local repo="$1" commit="$2" path="$3" expected="$4"
  local rel="${repo//\//__}/$commit/$path"
  local out="$DEST/$rel"
  local url="https://raw.githubusercontent.com/$repo/$commit/$path"
  mkdir -p "$(dirname "$out")"
  echo "fetch $repo@$commit:$path" >&2
  curl --fail --location --retry 3 --retry-delay 1 --silent --show-error "$url" -o "$out"
  local actual sha status
  actual="$(git_blob_sha1 "$out")"
  sha="$(sha256sum "$out" | awk '{print $1}')"
  status="computed"
  if [[ "$expected" != "-" ]]; then
    if [[ "$actual" == "$expected" ]]; then
      status="verified"
    else
      status="MISMATCH"
    fi
  fi
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$repo" "$commit" "$path" "$url" "$expected" "$actual" "$sha" "$status" >>"$MANIFEST"
  if [[ "$status" == "MISMATCH" ]]; then
    echo "ERROR: Git blob mismatch for $repo@$commit:$path" >&2
    return 1
  fi
}

LINUX=94515f3a7d4256a5062176b7d6ed0471938cd51a
XDNA=0697b1720fc539f57bdc8d5854ddf7c7014ae160
FW=924d73c9a2501a256d18a26cbe640548c70b3a9a
RZ=9513bd60ddd5cb960ddda897096b808dd5cca6d0
XRT=a21f10d59b51e9235b7c90b804470fce9650cfa0
LLVM_AIE=ce8c0f8fd66bff15b347351c67e9fb4fe0a17205
MLIR_AIE=79089be0abae3c7ec8a69b69357031b99667bdd4

# Linux kernel: exact key files and supporting implementation files.
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/Kconfig f39d7a87296cda75bc3cf9548b742e4e814ce491
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/npu5_regs.c 306b359d0cd39c9939bc5f08625f822f12e65168
fetch_git torvalds/linux "$LINUX" Documentation/accel/amdxdna/amdnpu.rst 064973bf489391a198a1b961b8c7a27693267201
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/amdxdna_pci_drv.c bb339e6414169f822b6aabffc1897a3e2babca3b
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/aie2_pci.c 22f66c7f534d42927d50ccae8f06ceb736c5d504
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/amdxdna_gem.c 4628a27872656fae56241d889d3074bda0a51a44
fetch_git torvalds/linux "$LINUX" include/uapi/drm/amdxdna_accel.h 51a507561df6a15ff76d6c236299a8bdc55d2465
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/aie2_ctx.c 101f324ee1787ffa2c707fd2e341c3b65cb925a0
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/amdxdna_ctx.c -
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/amdxdna_pm.c -
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/aie2_pm.c -
fetch_git torvalds/linux "$LINUX" drivers/accel/amdxdna/amdxdna_iommu.c -

# AMD XDNA driver/shim source.
fetch_git amd/xdna-driver "$XDNA" README.md 35c3f961b4ca2e5d5c01a3c633634cc813977438
fetch_git amd/xdna-driver "$XDNA" src/driver/amdxdna/npu5_regs.c a8be3ae6634c8812393f5f03641b007175e8e1f6
fetch_git amd/xdna-driver "$XDNA" CMakeLists.txt bb635e8422cadddc6d80de9911cb7123c3402d3a
fetch_git amd/xdna-driver "$XDNA" .gitmodules 9cf659f468cb29fc99388d6c54e013116725376e
fetch_git amd/xdna-driver "$XDNA" src/shim/shim.h fa2744088768f56ec78703e5eca7c9d5b7b1e08b

# linux-firmware metadata and license; no firmware payloads.
fetch_git kernel-firmware/linux-firmware "$FW" WHENCE 3be577a6ba7f0f880176c8c79c64f498df84599a
fetch_git kernel-firmware/linux-firmware "$FW" LICENSE.amdnpu -

# AMD public examples.
fetch_git amd/RyzenAI-SW "$RZ" LICENSE.txt a218338fe53898af5f1a6aeb7757b1a1c6ae34f9
fetch_git amd/RyzenAI-SW "$RZ" Transformer-examples/DistilBERT_text_classification_bf16/README.md f5f7cde87b16ea170613ec3e032e276188d31312
fetch_git amd/RyzenAI-SW "$RZ" Transformer-examples/DistilBERT_text_classification_bf16/run_inference.py 68540e2c4b646131ed6c8ea6a9710616e0c134fe
fetch_git amd/RyzenAI-SW "$RZ" Transformer-examples/DistilBERT_text_classification_bf16/vitisai_config.json 0437e56a442676efb81284e4a483e251ea2a4c47
fetch_git amd/RyzenAI-SW "$RZ" LLM-examples/RAG-OGA/custom_embedding/custom_embedding.py 780f9f4cfe6cbf756e8fa1d7c26dc91a40c65fb4
fetch_git amd/RyzenAI-SW "$RZ" LLM-examples/RAG-OGA/README.md d44216307ac18ea01a59a129e52af67ffceaea21

# Public runtime/compiler licenses. These pins are research references, not a
# claim that AMD's binary packages were built from these exact commits.
fetch_git Xilinx/XRT "$XRT" LICENSE 2d57c902f59b9d65bc04651e9cea007ab21ead23
fetch_git Xilinx/llvm-aie "$LLVM_AIE" LICENSE.TXT fa6ac540007032cbd0ec772a1c72e6cb5527a4fe
fetch_git Xilinx/mlir-aie "$MLIR_AIE" LICENSE 3149f1e840dc915b6355a397462634e362dc3cef

(
  cd "$DEST"
  find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum >SHA256SUMS
)

echo "Public primary-source fetch complete: $DEST" >&2
echo "Manifest: $MANIFEST" >&2
echo "Firmware binaries and AMD gated packages were not downloaded." >&2
