#!/usr/bin/env bash
set -euo pipefail
METHOD="${1:-}"
MODE="${2:-}"
EXECUTE=0
[[ "$MODE" == "--execute" ]] && EXECUTE=1
[[ -z "$MODE" || "$MODE" == "--execute" ]] || { echo "unknown option: $MODE" >&2; exit 2; }

run_or_print() {
  printf '+ '
  printf '%q ' "$@"
  printf '%s\n' ''
  if [[ "$EXECUTE" == 1 ]]; then "$@"; fi
}

case "$METHOD" in
  apt)
    run_or_print sudo apt autoremove amdrocm7.14-gfx1151
    echo 'Review /etc/apt/sources.list.d and /etc/apt/keyrings separately; retain provenance before deleting them.'
    ;;
  dnf)
    run_or_print sudo dnf remove amdrocm7.14-gfx1151
    echo 'Review /etc/yum.repos.d separately; retain provenance before deleting it.'
    ;;
  zypper)
    run_or_print sudo zypper remove 'amdrocm*'
    ;;
  pip)
    VENV_PATH="${VENV_PATH:-}"
    [[ -n "$VENV_PATH" && "$VENV_PATH" != "/" ]] || { echo 'Set VENV_PATH to the dedicated 7.14 virtual environment.' >&2; exit 10; }
    run_or_print rm -rf -- "$VENV_PATH"
    echo 'Remove only the corresponding shell/profile block after retaining pip freeze and wheel hashes.'
    ;;
  tarball)
    ROCM_ROOT="${ROCM_ROOT:-}"
    [[ -n "$ROCM_ROOT" && "$ROCM_ROOT" != "/" && "$ROCM_ROOT" != "/opt" && "$ROCM_ROOT" != "/opt/rocm" ]] || { echo 'Set ROCM_ROOT to the dedicated 7.14 extraction root; broad roots are refused.' >&2; exit 10; }
    run_or_print rm -rf -- "$ROCM_ROOT"
    echo "Remove only that root's PATH/LD_LIBRARY_PATH/ROCM_PATH profile block after retaining the artifact inventory."
    ;;
  runfile)
    RUNFILE="${RUNFILE:-rocm-installer-7.14.0-6.run}"
    run_or_print bash "$RUNFILE" uninstall-rocm gfx=gfx1151
    ;;
  *)
    cat >&2 <<'EOF'
usage: rollback-7.14.sh {apt|dnf|zypper|pip|tarball|runfile} [--execute]
Default mode prints the exact rollback command. --execute performs it.
For pip set VENV_PATH; for tarball set ROCM_ROOT; for runfile optionally set RUNFILE.
Retain package metadata, manifests, hashes, logs and runtime captures before rollback.
EOF
    exit 2
    ;;
esac

if [[ "$EXECUTE" == 0 ]]; then
  echo '[DRY_RUN] Re-run with --execute only after reviewing the printed command.'
fi
