#!/usr/bin/env bash
# Reference state-preparation scaffold. Its output is evidence of actions, not proof
# of cache state; cards require counters/timings that verify the intended state.
set -euo pipefail
STATE=${1:?usage: prepare_cache_state.sh C0|C1|C2|C3 EVIDENCE_DIR [MODEL_FILE]}
OUT=${2:?}; MODEL=${3:-}; mkdir -p "$OUT"
[[ $STATE =~ ^C[0-3]$ ]] || { echo "invalid state" >&2; exit 2; }
BOOT_ID=$(cat /proc/sys/kernel/random/boot_id)
{
  echo "state=$STATE"; echo "timestamp_utc=$(date --utc +%FT%TZ)"; echo "boot_id=$BOOT_ID"
  echo "hostname=$(hostname)"; echo "model=${MODEL:-unset}"
} > "$OUT/action.txt"

case "$STATE" in
  C0)
    # C0 must follow a reboot/power cycle. Supply a file containing the pre-reboot boot ID.
    PROOF=${C0_PREVIOUS_BOOT_ID_FILE:-}
    [[ -n $PROOF && -r $PROOF ]] || { echo "C0 requires C0_PREVIOUS_BOOT_ID_FILE" >&2; exit 3; }
    PREV=$(tr -d '[:space:]' < "$PROOF")
    [[ -n $PREV && $PREV != "$BOOT_ID" ]] || { echo "boot ID did not change; C0 not established" >&2; exit 4; }
    echo "previous_boot_id=$PREV" >> "$OUT/action.txt"
    ;;
  C1)
    [[ -n $MODEL && -r $MODEL ]] || { echo "C1 requires readable MODEL_FILE" >&2; exit 3; }
    [[ -z ${RUNTIME_PID_FILE:-} || ! -s ${RUNTIME_PID_FILE:-/dev/null} ]] || { echo "runtime appears resident; stop it before C1" >&2; exit 4; }
    BEFORE=$(awk '/^pgmajfault /{print $2}' /proc/vmstat)
    dd if="$MODEL" of=/dev/null bs=16M status=none
    AFTER=$(awk '/^pgmajfault /{print $2}' /proc/vmstat)
    sha256sum "$MODEL" > "$OUT/model.sha256"
    stat --printf='model_bytes=%s\n' "$MODEL" >> "$OUT/action.txt"
    printf 'pgmajfault_before=%s\npgmajfault_after=%s\n' "$BEFORE" "$AFTER" >> "$OUT/action.txt"
    ;;
  C2)
    [[ -n ${C2_CLEAR_CMD:-} && -n ${C2_VERIFY_CMD:-} ]] || { echo "C2 requires C2_CLEAR_CMD and C2_VERIFY_CMD" >&2; exit 3; }
    bash -lc "$C2_CLEAR_CMD" > "$OUT/clear.stdout" 2> "$OUT/clear.stderr"
    bash -lc "$C2_VERIFY_CMD" > "$OUT/verify.stdout" 2> "$OUT/verify.stderr"
    echo "verification=external_command_passed; still verify cached_prompt_tokens=0 in raw requests" >> "$OUT/action.txt"
    ;;
  C3)
    [[ -n ${C3_PRIME_CMD:-} && -n ${C3_VERIFY_CMD:-} ]] || { echo "C3 requires C3_PRIME_CMD and C3_VERIFY_CMD" >&2; exit 3; }
    bash -lc "$C3_PRIME_CMD" > "$OUT/prime.stdout" 2> "$OUT/prime.stderr"
    bash -lc "$C3_VERIFY_CMD" > "$OUT/verify.stdout" 2> "$OUT/verify.stderr"
    echo "verification=external_command_passed; still verify eligible/cached token counters" >> "$OUT/action.txt"
    ;;
esac
( cd "$OUT" && find . -type f ! -name MANIFEST.sha256 -print0 | sort -z | xargs -0 sha256sum > MANIFEST.sha256 )
echo "Prepared action record for $STATE at $OUT; machine counters must independently prove the state."
