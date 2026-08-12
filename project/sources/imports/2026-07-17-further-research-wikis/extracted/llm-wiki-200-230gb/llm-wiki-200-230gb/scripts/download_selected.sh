#!/usr/bin/env bash
set -euo pipefail
ID="${1:?candidate id required}"
OUT="${2:?output directory required}"
case "$ID" in
  qwen3-235b-a22b-2507) REPO='unsloth/Qwen3-235B-A22B-Instruct-2507-GGUF'; REV='160ed54'; INC='UD-Q6_K_XL/*' ;;
  step-3.7-flash) REPO='stepfun-ai/Step-3.7-Flash-GGUF'; REV='713961b'; INC='Q8_0/*' ;;
  mimo-v2-flash) REPO='bartowski/XiaomiMiMo_MiMo-V2-Flash-GGUF'; REV='6b8a0ba'; INC='XiaomiMiMo_MiMo-V2-Flash-Q5_K_M/*' ;;
  glm-4.7) REPO='bartowski/zai-org_GLM-4.7-GGUF'; REV='75abf8a'; INC='zai-org_GLM-4.7-Q4_K_M/*' ;;
  nemotron-ultra-253b) REPO='bartowski/nvidia_Llama-3_1-Nemotron-Ultra-253B-v1-GGUF'; REV='9195f67'; INC='nvidia_Llama-3_1-Nemotron-Ultra-253B-v1-Q6_K/*' ;;
  deepseek-r1-0528) REPO='unsloth/DeepSeek-R1-0528-GGUF'; REV='main'; INC='UD-IQ2_M/*' ;;
  tulu3-405b) REPO='bartowski/Llama-3.1-Tulu-3-405B-GGUF'; REV='main'; INC='Llama-3.1-Tulu-3-405B-IQ4_XS/*' ;;
  minimax-m3) REPO='unsloth/MiniMax-M3-GGUF'; REV='41b3ee5f52f642949301cb1fc34cf8379ba22416'; INC='UD-IQ4_XS/*' ;;
  *) echo "unknown candidate: $ID" >&2; exit 2 ;;
esac
mkdir -p "$OUT"
if command -v hf >/dev/null 2>&1; then
  hf download "$REPO" --revision "$REV" --include "$INC" --local-dir "$OUT"
else
  huggingface-cli download "$REPO" --revision "$REV" --include "$INC" --local-dir "$OUT"
fi
