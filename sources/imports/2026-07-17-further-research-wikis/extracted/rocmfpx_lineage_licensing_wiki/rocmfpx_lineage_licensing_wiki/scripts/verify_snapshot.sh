#!/usr/bin/env bash
set -euo pipefail

WORKDIR="${1:-$PWD/audit-repos}"
mkdir -p "$WORKDIR"

repos=(
  "charlie12345/ROCmFPX|main|a5605a72768c6562241b248e268e33dc92787394"
  "fewtarius/llama-ai|main|1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  "fewtarius/CachyLLama|master|6be745998f568e379ea197fcf827baec73ff9940"
  "ggml-org/llama.cpp|master|86d86ed4396b4130922f7b9af26e3d9fc11a591b"
)

fail=0
log() { printf '[verify] %s\n' "$*"; }

for spec in "${repos[@]}"; do
  IFS='|' read -r repo branch expected <<<"$spec"
  name="${repo//\//__}"
  url="https://github.com/${repo}.git"
  remote="$(git ls-remote "$url" "refs/heads/$branch" | awk '{print $1}')"
  if [[ -z "$remote" ]]; then
    log "ERROR: no remote tip for $repo $branch"
    fail=1
    continue
  fi
  if [[ "$remote" != "$expected" ]]; then
    log "DRIFT: $repo $branch is $remote; research cutoff was $expected"
  else
    log "OK tip: $repo $branch $expected"
  fi

  dir="$WORKDIR/$name"
  if [[ ! -d "$dir/.git" ]]; then
    git clone --no-single-branch "$url" "$dir"
  fi
  git -C "$dir" fetch --all --tags --prune
  if ! git -C "$dir" cat-file -e "$expected^{commit}"; then
    log "ERROR: expected commit missing in $repo"
    fail=1
  fi
done

check_parent() {
  local dir="$1" child="$2" index="$3" expected="$4" label="$5"
  local actual
  actual="$(git -C "$dir" rev-parse "${child}^${index}")"
  if [[ "$actual" == "$expected" ]]; then
    log "OK parent: $label $child^$index = $expected"
  else
    log "ERROR parent: $label $child^$index expected $expected got $actual"
    fail=1
  fi
}

r="$WORKDIR/charlie12345__ROCmFPX"
check_parent "$r" a5605a72768c6562241b248e268e33dc92787394 1 25c71fc6e12d73bb3804127e032d29fb8976ae40 ROCmFPX
check_parent "$r" a5605a72768c6562241b248e268e33dc92787394 2 a8b5fa906ccd13c6a8ca06d55aa287854c376868 ROCmFPX
check_parent "$r" c2845bf86a5c1842d33bd9e990b2bcaf75779251 1 5b3956605309dd3e6beed49c8f3a41423ba71d25 ROCmFPX
check_parent "$r" c2845bf86a5c1842d33bd9e990b2bcaf75779251 2 ccac6e55ec7c0fa8acdcb6e80b1a242e4f7d654e ROCmFPX
check_parent "$r" 2335e6a482b1601d71dff9e860c8feab108c3af2 1 221402af8574faf652b101b6afe225a3f329561f ROCmFPX
check_parent "$r" 2335e6a482b1601d71dff9e860c8feab108c3af2 2 5b3956605309dd3e6beed49c8f3a41423ba71d25 ROCmFPX

if git -C "$r" merge-base --is-ancestor 2335e6a482b1601d71dff9e860c8feab108c3af2 a5605a72768c6562241b248e268e33dc92787394; then
  log "OK ROCmFPX ancestor 2335e6a482b1601d71dff9e860c8feab108c3af2"
else
  log "ERROR: ROCmFPX ancestor evidence changed/missing"
  fail=1
fi
if git -C "$r" merge-base --is-ancestor 5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4 a5605a72768c6562241b248e268e33dc92787394; then
  log "DRIFT: 5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4 now appears as ancestor; re-run analysis"
else
  log "OK expected non-ancestor: 5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4"
fi

c="$WORKDIR/fewtarius__CachyLLama"
check_parent "$c" 6be745998f568e379ea197fcf827baec73ff9940 1 c8ead677a7fe42fb0a67e6e866fb254cc338e9fd CachyLLama
check_parent "$c" 6be745998f568e379ea197fcf827baec73ff9940 2 92366df30d4eaa4b85139b5fd694360237731b19 CachyLLama

l="$WORKDIR/fewtarius__llama-ai"
check_parent "$l" 1017f3dfdce3ca2b06aa9007b23295db3bb35722 1 d8a07baad6ab175f8badbc4d496c9190b0cc3b2d llama-ai
gitlink="$(git -C "$l" ls-tree 1017f3dfdce3ca2b06aa9007b23295db3bb35722 CachyLLama | awk '{print $3}')"
if [[ "$gitlink" == "6be745998f568e379ea197fcf827baec73ff9940" ]]; then
  log "OK llama-ai CachyLLama gitlink"
else
  log "ERROR: expected gitlink 6be745998f568e379ea197fcf827baec73ff9940, got $gitlink"
  fail=1
fi

u="$WORKDIR/ggml-org__llama.cpp"
check_parent "$u" 86d86ed4396b4130922f7b9af26e3d9fc11a591b 1 7d56da7e546f54fb1fa54ef2bc9ad9a872860ab0 llama.cpp
if git -C "$u" merge-base --is-ancestor 2969d6d15d67a08e7b83f26164b15350c79c5248 86d86ed4396b4130922f7b9af26e3d9fc11a591b; then
  log "OK upstream reverse-port commit is ancestor of cutoff tip"
else
  log "ERROR: upstream reverse-port ancestry missing"
  fail=1
fi

for item in \
  "$r|a5605a72768c6562241b248e268e33dc92787394|628249b398293fc8d2fa81a449ae2920a02c6523" \
  "$c|6be745998f568e379ea197fcf827baec73ff9940|eced84c86f8b012c752c016f7fe789adea168e1e" \
  "$u|86d86ed4396b4130922f7b9af26e3d9fc11a591b|9be313313c8ecb9488911bd64550190e3ed80f38"; do
  IFS='|' read -r dir commit marker <<<"$item"
  actual="$(git -C "$dir" show "$commit:scripts/sync-ggml.last" | tr -d '[:space:]')"
  if [[ "$actual" == "$marker" ]]; then
    log "OK ggml marker $marker"
  else
    log "ERROR: ggml marker mismatch: expected $marker got $actual"
    fail=1
  fi
done

log "Raw merge objects to archive:"
git -C "$r" cat-file -p a5605a72768c6562241b248e268e33dc92787394 | sed -n '/^parent /p'
git -C "$r" cat-file -p c2845bf86a5c1842d33bd9e990b2bcaf75779251 | sed -n '/^parent /p'
git -C "$r" cat-file -p 2335e6a482b1601d71dff9e860c8feab108c3af2 | sed -n '/^parent /p'
git -C "$c" cat-file -p 6be745998f568e379ea197fcf827baec73ff9940 | sed -n '/^parent /p'

exit "$fail"
