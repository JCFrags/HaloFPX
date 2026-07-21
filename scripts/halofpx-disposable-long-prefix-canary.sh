#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 || ! "$1" =~ ^(capture|restore|cold)$ ]]; then
    echo "usage: $0 capture|restore|cold" >&2
    exit 2
fi

mode=$1
source_root=/var/tmp/halofpx-l13-retry-src-nimo2
evidence_root=/var/tmp/halofpx-l13-retry-a2-20260721
coordinator_root=/var/tmp/halofpx-l13-retry-a2-coordinator-20260721
model=/var/tmp/halofpx-qualification/l14q-t01-20260719-nimo2/build-cpu/tinyllamas/stories15M-q4_0.gguf

command=(
    "$source_root/build-a/bin/test-halofpx-distributed-state-canary"
    --hfx-mode "$mode"
    --hfx-artifact-root "$coordinator_root"
    --hfx-model-digest 66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739
    --hfx-compatibility-root 34b44384cbb0a1009e693d2b278596475ece38efe3398d7995997fdd9db91122
    --hfx-plan-digest 3d8972ef59643252d3b1f76a865efe637aa4e155ca9fe7000039dc52a9b30051
    --hfx-topology-digest 9ea8ca58e889013f0d231c0e784158d4b2747523c322f64233cd603641464dd6
    --hfx-placement-digest 6f864264ee73a81f032d8fa10daf8dc41c5c8ba6ab6dab3d5416b64f35dda305
    --hfx-checkpoint-digest 01f23ac1a23d43ab1529082d07875acfc13cc359bf90e052696703375f5fbe32
    --hfx-control-file "$evidence_root/control.key"
    --hfx-expected-prompt-tokens 1129
    --model "$model"
    --rpc 10.44.0.1:50179
    --split-mode layer
    --tensor-split 1
    --n-gpu-layers 3
    --fit off
    --no-mmap
    --ctx-size 2048
    --batch-size 512
    --ubatch-size 512
    --parallel 1
    --threads 8
    --threads-batch 8
    --file "$evidence_root/prompt-1129.txt"
    --n-predict 8
    --seed 1234
    --temp 0
)

printf 'invocation='
printf '%q ' "${command[@]}"
printf '\n'
exec "${command[@]}"
