#!/usr/bin/env bash
# Build ROCmFPX for RDNA4 (gfx1200/gfx1201) using a local ROCm 7.14.0a20260624 toolchain.
#
# Downloads the TheRock nightly tarball for gfx120X-all only after verifying an
# operator-supplied SHA-256, extracts it to a local path, and uses that for
# compilation. No system-wide ROCm installation is required for the build.
#
# Usage:
#   env JOBS=16 ROCM_TARBALL_SHA256=<independently-recorded-sha256> scripts/build-rocmfp4-rocm714-local.sh
#   env JOBS=16 ROCM_VERSION=7.14.0a20260624 ROCM_TARBALL_SHA256=<sha256> scripts/build-rocmfp4-rocm714-local.sh
#
# The build output goes to build-rdna4-rocm714/ by default. Can be overridden:
#   env BUILD_DIR=build-custom ROCM_VERSION=7.14.0a20260624 scripts/build-rocmfp4-rocm714-local.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-rdna4-rocm714}"
JOBS="${JOBS:-$(nproc)}"
ROCM_VERSION="${ROCM_VERSION:-7.14.0a20260624}"
ROCM_LOCAL_PATH="${ROCM_LOCAL_PATH:-${XDG_DATA_HOME:-$HOME/.local/share}/rocm-${ROCM_VERSION}}"
ROCM_TARBALL_SHA256="${ROCM_TARBALL_SHA256:-}"
HIP_ARCH="${CMAKE_HIP_ARCHITECTURES:-gfx1200;gfx1201}"
DECODE_TUNE_PROFILE="${ROCMFPX_DECODE_TUNE:-stable}"

# Decode tuning flags
source "$ROOT/scripts/rocmfp4-decode-tune-flags.sh"

if ! tune_flags="$(rocmfp4_decode_tune_flags "$DECODE_TUNE_PROFILE")"; then
    echo "Unknown decode tuning profile '$DECODE_TUNE_PROFILE'" >&2
    echo "Known profiles: $(rocmfp4_decode_tune_known_profiles)" >&2
    exit 2
fi

HIP_EXTRA_FLAGS="$tune_flags ${CMAKE_HIP_FLAGS:-}"

# ── Download and extract ROCm if needed ──────────────────────────────
if [[ ! -x "$ROCM_LOCAL_PATH/llvm/bin/clang" ]]; then
    echo "ROCm toolchain not found at $ROCM_LOCAL_PATH. Downloading..."

    TARBALL_URL="https://rocm.nightlies.amd.com/tarball-multi-arch/therock-dist-linux-gfx120X-all-${ROCM_VERSION}.tar.gz"
    TARBALL="${ROOT}/rocm-${ROCM_VERSION}.tar.gz"

    if [[ -z "$ROCM_TARBALL_SHA256" ]]; then
        echo "Error: ROCM_TARBALL_SHA256 is required before downloading or extracting the mutable nightly URL." >&2
        echo "Obtain and record the digest independently, then rerun with ROCM_TARBALL_SHA256=<64-hex-sha256>." >&2
        exit 2
    fi
    if [[ ! "$ROCM_TARBALL_SHA256" =~ ^[[:xdigit:]]{64}$ ]]; then
        echo "Error: ROCM_TARBALL_SHA256 must be exactly 64 hexadecimal characters." >&2
        exit 2
    fi
    if ! command -v sha256sum &>/dev/null; then
        echo "Error: sha256sum is required to verify the ROCm toolchain archive." >&2
        exit 1
    fi
    ROCM_TARBALL_SHA256="${ROCM_TARBALL_SHA256,,}"

    if [[ ! -s "$TARBALL" ]]; then
        echo "Downloading ROCm ${ROCM_VERSION} from TheRock nightlies..."
        TARBALL_PART="${TARBALL}.partial.$$"
        trap 'rm -f -- "${TARBALL_PART:-}"' EXIT
        curl --fail --location --output "$TARBALL_PART" "$TARBALL_URL"
        actual_sha256="$(sha256sum "$TARBALL_PART" | awk '{print $1}')"
        if [[ "$actual_sha256" != "$ROCM_TARBALL_SHA256" ]]; then
            echo "Error: downloaded ROCm archive SHA-256 mismatch." >&2
            echo "Expected: $ROCM_TARBALL_SHA256" >&2
            echo "Actual:   $actual_sha256" >&2
            exit 1
        fi
        mv -- "$TARBALL_PART" "$TARBALL"
        trap - EXIT
    fi

    actual_sha256="$(sha256sum "$TARBALL" | awk '{print $1}')"
    if [[ "$actual_sha256" != "$ROCM_TARBALL_SHA256" ]]; then
        echo "Error: cached ROCm archive SHA-256 mismatch: $TARBALL" >&2
        echo "Expected: $ROCM_TARBALL_SHA256" >&2
        echo "Actual:   $actual_sha256" >&2
        exit 1
    fi

    if [[ -e "$ROCM_LOCAL_PATH" ]]; then
        echo "Error: $ROCM_LOCAL_PATH exists but does not contain the expected clang executable." >&2
        echo "Inspect or move that directory aside; this script will not overwrite a partial toolchain." >&2
        exit 1
    fi

    echo "Extracting to $ROCM_LOCAL_PATH..."
    ROCM_STAGE="${ROCM_LOCAL_PATH}.partial.$$"
    trap 'rm -rf -- "${ROCM_STAGE:-}"' EXIT
    mkdir -p "$ROCM_STAGE"
    tar --use-compress-program=gzip -xf "$TARBALL" -C "$ROCM_STAGE" --strip-components=1
    if [[ ! -x "$ROCM_STAGE/llvm/bin/clang" || ! -x "$ROCM_STAGE/llvm/bin/clang++" ]]; then
        echo "Error: verified archive did not extract the expected clang toolchain layout." >&2
        exit 1
    fi
    mv -- "$ROCM_STAGE" "$ROCM_LOCAL_PATH"
    trap - EXIT
    echo "ROCm toolchain ready at $ROCM_LOCAL_PATH"
fi

# ── Verify toolchain ────────────────────────────────────────────────
CLANG="$ROCM_LOCAL_PATH/llvm/bin/clang"
CLANGXX="$ROCM_LOCAL_PATH/llvm/bin/clang++"

if [[ ! -x "$CLANG" || ! -x "$CLANGXX" ]]; then
    echo "Error: clang/clang++ not found under $ROCM_LOCAL_PATH/llvm/bin" >&2
    exit 1
fi

echo "Using clang: $CLANG"
echo "Using clang++: $CLANGXX"
"$CLANG" --version | head -1

# Bind every ROCm/HIP discovery mechanism used by this tree to the verified
# local SDK. A versioned archive name alone is not proof that CMake avoided a
# system /opt/rocm installation.
export ROCM_PATH="$ROCM_LOCAL_PATH"
export HIP_PATH="$ROCM_LOCAL_PATH"
export PATH="$ROCM_LOCAL_PATH/bin:$ROCM_LOCAL_PATH/llvm/bin:$PATH"
export LD_LIBRARY_PATH="$ROCM_LOCAL_PATH/lib:$ROCM_LOCAL_PATH/lib/llvm/lib:$ROCM_LOCAL_PATH/lib/rocm_sysdeps/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

if [[ -e "$BUILD_DIR/CMakeCache.txt" ]]; then
    echo "Error: refusing to reuse an existing CMake cache: $BUILD_DIR/CMakeCache.txt" >&2
    echo "Choose a new BUILD_DIR or move the old build tree aside so system and local ROCm discoveries cannot mix." >&2
    exit 1
fi

# ── Configure ────────────────────────────────────────────────────────
echo "Configuring build..."

cmake -S "$ROOT" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$CLANG" \
    -DCMAKE_CXX_COMPILER="$CLANGXX" \
    -DCMAKE_HIP_COMPILER="$CLANGXX" \
    -DCMAKE_CXX_FLAGS="-I${ROCM_LOCAL_PATH}/include" \
    -DGGML_HIP=ON \
    -DGGML_HIP_FORCE_MMQ=ON \
    -DGGML_HIP_ROCWMMA_FATTN=OFF \
    -DGGML_VULKAN=ON \
    -DGGML_CUDA=OFF \
    -DGGML_NATIVE=OFF \
    -DCMAKE_HIP_ARCHITECTURES="$HIP_ARCH" \
    -DGPU_TARGETS="$HIP_ARCH" \
    -DCMAKE_HIP_FLAGS="$HIP_EXTRA_FLAGS" \
    -DLLAMA_BUILD_SERVER=ON \
    -DLLAMA_BUILD_WEBUI=OFF \
    -DLLAMA_USE_PREBUILT_WEBUI=OFF \
    -DLLAMA_BUILD_TESTS=ON \
    -DGGML_BUILD_TESTS=OFF \
    -DCMAKE_PREFIX_PATH="$ROCM_LOCAL_PATH" \
    -DROCM_DIR="$ROCM_LOCAL_PATH" \
    -DBUILD_SHARED_LIBS=OFF

cache_value() {
    local key="$1"
    sed -n "s/^${key}:[^=]*=//p" "$BUILD_DIR/CMakeCache.txt" | tail -n 1
}

for compiler_key in CMAKE_C_COMPILER CMAKE_CXX_COMPILER CMAKE_HIP_COMPILER; do
    compiler_value="$(cache_value "$compiler_key")"
    case "$compiler_value" in
        "$ROCM_LOCAL_PATH"/*) ;;
        *)
            echo "Error: $compiler_key resolved outside the verified local SDK: ${compiler_value:-<unset>}" >&2
            exit 1
            ;;
    esac
done

for package_key in hip_DIR hipblas_DIR rocblas_DIR; do
    package_value="$(cache_value "$package_key")"
    case "$package_value" in
        "$ROCM_LOCAL_PATH"/*) ;;
        *)
            echo "Error: $package_key resolved outside the verified local SDK: ${package_value:-<unset>}" >&2
            exit 1
            ;;
    esac
done

# ── Build ────────────────────────────────────────────────────────────
echo "Building..."
cmake --build "$BUILD_DIR" -j "$JOBS" \
    --target llama-cli llama-server llama-quantize llama-bench \
    test-backend-ops test-quantize-fns

# ── Bundle ROCm runtime libs alongside the binaries ──────────────────
# Copy candidate shared libraries into build-rdna4-rocm714/lib/ and set RPATH
# to $ORIGIN/../lib. The checks below prove direct resolution on this build
# host only; they do not prove clean-host portability or target-GPU behavior.

if ! command -v patchelf &>/dev/null; then
    echo "Error: patchelf is required for the local-runtime packaging step." >&2
    echo "Install it (for example, 'sudo apt install patchelf') and rerun." >&2
    exit 1
fi

echo "Bundling ROCm runtime libraries..."
BUNDLED_LIB_DIR="$BUILD_DIR/lib"
mkdir -p "$BUNDLED_LIB_DIR"

copy_required_glob() {
    local pattern="$1"
    local matches=()
    mapfile -t matches < <(compgen -G "$pattern" || true)
    if (( ${#matches[@]} == 0 )); then
        echo "Error: required local ROCm runtime pattern had no matches: $pattern" >&2
        exit 1
    fi
    cp -av -- "${matches[@]}" "$BUNDLED_LIB_DIR/"
}

copy_optional_glob() {
    local pattern="$1"
    local matches=()
    mapfile -t matches < <(compgen -G "$pattern" || true)
    if (( ${#matches[@]} > 0 )); then
        cp -av -- "${matches[@]}" "$BUNDLED_LIB_DIR/"
    fi
}

# Direct HIP/BLAS runtime candidates are required. Other compiler/runtime
# libraries are copied when this exact SDK supplies them; absence is not hidden.
copy_required_glob "$ROCM_LOCAL_PATH/lib/libamdhip64.so*"
copy_required_glob "$ROCM_LOCAL_PATH/lib/librocblas.so*"
copy_required_glob "$ROCM_LOCAL_PATH/lib/libhipblas.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/libhipblaslt.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/librocsolver.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/libroctx64.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/libamd_comgr.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/libhsa-runtime64.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/librocm_kpack.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/librocroller.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/librocprofiler-register.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/llvm/lib/libLLVM.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/llvm/lib/libclang-cpp.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/llvm/lib/libomp.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/llvm/lib/libomptarget.so*"
copy_optional_glob "$ROCM_LOCAL_PATH/lib/rocm_sysdeps/lib/librocm_sysdeps_*.so*"

    # Copy rocblas/hipblaslt kernel libraries
    if [[ -d "$ROCM_LOCAL_PATH/lib/rocblas/library" ]]; then
        mkdir -p "$BUNDLED_LIB_DIR/rocblas"
        cp -a "$ROCM_LOCAL_PATH/lib/rocblas/library" "$BUNDLED_LIB_DIR/rocblas/"
    fi
    if [[ -d "$ROCM_LOCAL_PATH/lib/hipblaslt/library" ]]; then
        mkdir -p "$BUNDLED_LIB_DIR/hipblaslt"
        cp -a "$ROCM_LOCAL_PATH/lib/hipblaslt/library" "$BUNDLED_LIB_DIR/hipblaslt/"
    fi

echo "Setting RPATH on binaries to use bundled libs..."
for bin in "$BUILD_DIR"/bin/llama-* "$BUILD_DIR"/bin/test-*; do
    if [[ -f "$bin" ]]; then
        patchelf --set-rpath '$ORIGIN/../lib' "$bin"
    fi
done

# ── Done ─────────────────────────────────────────────────────────────
echo ""
echo "Build complete for ${HIP_ARCH}:"
echo "  $BUILD_DIR/bin/llama-cli"
echo "  $BUILD_DIR/bin/llama-server"
echo "  $BUILD_DIR/bin/llama-quantize"
echo "  $BUILD_DIR/bin/llama-bench"
echo ""
echo "ROCm runtime libraries were copied into $BUILD_DIR/lib/."

# ── Verify ───────────────────────────────────────────────────────────
echo ""
echo "Verification:"
if ! ldd_output="$(env -u LD_LIBRARY_PATH ldd "$BUILD_DIR/bin/llama-cli" 2>&1)"; then
    echo "Error: ldd could not inspect llama-cli:" >&2
    echo "$ldd_output" >&2
    exit 1
fi
if grep -q 'not found' <<<"$ldd_output"; then
    echo "Error: unresolved direct dependencies:" >&2
    grep 'not found' <<<"$ldd_output" >&2
    exit 1
fi
rocm_dependency_re='^[[:space:]]*(libamdhip64|librocblas|libhipblas|libhipblaslt|librocsolver|libhsa-runtime64|libamd_comgr|librocm[^[:space:]]*)[^=]*=>[[:space:]]+([^[:space:]]+)'
while IFS= read -r dependency_line; do
    if [[ "$dependency_line" =~ $rocm_dependency_re ]]; then
        resolved_path="${BASH_REMATCH[2]}"
        case "$resolved_path" in
            "$BUNDLED_LIB_DIR"/*) ;;
            *)
                echo "Error: a ROCm dependency resolved outside the candidate runtime directory: $dependency_line" >&2
                exit 1
                ;;
        esac
    fi
done <<<"$ldd_output"
echo "Direct dependencies resolved on this build host."
echo "This is not proof of portability; run a target-GPU smoke test before distribution."

# ── Verify ───────────────────────────────────────────────────────────
echo ""
echo "To run:"
echo "  $BUILD_DIR/bin/llama-cli -m model.gguf -dev ROCm0 -ngl 999"
