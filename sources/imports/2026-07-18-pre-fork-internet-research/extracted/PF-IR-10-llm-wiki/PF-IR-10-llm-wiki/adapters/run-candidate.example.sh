#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# CLAIM-LABEL: UNEXECUTED-EVIDENCE
# This file intentionally refuses direct execution from the evidence package.
set -eu
if [ "${PFIR10_QUALIFIED_WORKSPACE:-}" != "YES" ]; then
  echo "Refusing: copy and review this adapter inside a qualified isolated workspace." >&2
  exit 64
fi
: "${CANDIDATE_BIN:?set exact candidate binary path}"
: "${FIXTURE:?set immutable fixture path}"
: "${OUT_DIR:?set fresh capture directory}"
mkdir -p "$OUT_DIR"
sha256sum "$CANDIDATE_BIN" "$FIXTURE" > "$OUT_DIR/inputs.sha256"
# Replace the next line only after static review of the candidate CLI.
"$CANDIDATE_BIN" --model "$FIXTURE" --version >"$OUT_DIR/stdout" 2>"$OUT_DIR/stderr"
printf '%s\n' "$?" > "$OUT_DIR/exit-code"
