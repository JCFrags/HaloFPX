# Reproduction and Acceptance Workflow

## 1. Resolve exact artifact manifests

```bash
python scripts/refresh_hf_manifests.py --all
```

## 2. Download one selected representation

```bash
bash scripts/download_selected.sh qwen3-235b-a22b-2507 /models/qwen
```

## 3. Verify all shards

```bash
python scripts/verify_downloads.py --download-root /models/qwen --manifest manifests/refreshed/qwen3-235b-a22b-2507.json
```

## 4. Build pinned runtimes

```bash
git clone https://github.com/ggml-org/llama.cpp.git
git -C llama.cpp checkout 6bdd77f13cf11b264b4231d320afc404f48d576e

git clone https://github.com/charlie12345/ROCmFPX.git
git -C ROCmFPX checkout 61f2f2d7bc4955e9bca821095ef69125837133b5
```

Use backend-specific build instructions from the repositories. Record compiler, driver, ROCm/Vulkan version, and build flags.

## 5. Template smoke test

Run one-token generation with `--jinja`, inspect rendered roles and special tokens, then execute tool/JSON validators before performance benchmarking.

## 6. Capture measured memory

Save verbose startup output and parse it:

```bash
llama-cli -m /models/model-00001-of-000NN.gguf --jinja -c 32768 -b 512 -ub 512 -n 1 -p "ping" 2>&1 | tee run.log
python scripts/parse_llama_memory.py run.log --output measured-memory.json
```

## 7. Recalculate with measured reserves

Edit `data/profiles.json`, then rerun `python scripts/calculate_capacity.py` and `python scripts/build_static.py`. A candidate advances only when every device/node has positive measured margin at the intended context and concurrency.
