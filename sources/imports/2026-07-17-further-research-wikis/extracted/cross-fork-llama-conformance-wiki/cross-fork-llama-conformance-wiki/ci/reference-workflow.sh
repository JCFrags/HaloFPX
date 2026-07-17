#!/usr/bin/env bash
set -euo pipefail
action=${1:-}
case_ids=${2:-}
cat >&2 <<EOF
Reference workflow adapter is intentionally organization-specific.
Requested action: $action
Requested cases: $case_ids

Implement this adapter only after:
- immutable source/model lock retrieval;
- named reference hardware provisioning;
- raw observation capture;
- calibration/validation partition enforcement;
- protected reviewer and signing policy.
EOF
exit 2
