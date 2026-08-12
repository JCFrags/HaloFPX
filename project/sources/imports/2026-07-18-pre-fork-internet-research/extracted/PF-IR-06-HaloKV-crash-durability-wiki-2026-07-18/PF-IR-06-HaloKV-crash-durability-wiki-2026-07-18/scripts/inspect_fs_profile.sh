#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 ABSOLUTE_CACHE_ROOT" >&2
  exit 64
fi
root=$1
if [[ $root != /* ]]; then
  echo "error: path must be absolute" >&2
  exit 64
fi
mkdir -p -- "$root"

printf 'PF-IR-06 filesystem profile\n'
printf 'timestamp_utc=%s\n' "$(date -u +%FT%TZ)"
printf 'path=%q\n' "$root"
printf 'kernel=%s\n' "$(uname -srvmo)"
printf 'uid_gid=%s:%s\n' "$(id -u)" "$(id -g)"
printf '\n[findmnt]\n'
findmnt -T "$root" -o TARGET,SOURCE,FSTYPE,OPTIONS,FSROOT,MAJ:MIN,UUID,LABEL -n || true
printf '\n[statfs]\n'
stat -f -c 'type=%T type_hex=%t block_size=%S blocks=%b free_blocks=%f avail_blocks=%a files=%c free_files=%d' "$root"
printf '\n[mountinfo matching mount id]\n'
mount_id=$(stat -c %m "$root" 2>/dev/null || true)
printf 'stat_mountpoint=%s\n' "$mount_id"
findmnt -T "$root" --json 2>/dev/null || true
printf '\n[block topology]\n'
source_dev=$(findmnt -T "$root" -n -o SOURCE 2>/dev/null || true)
printf 'source=%s\n' "$source_dev"
lsblk -o NAME,KNAME,TYPE,FSTYPE,SIZE,ROTA,RO,DISC-GRAN,DISC-MAX,DISC-ZERO,PKNAME,MOUNTPOINTS 2>/dev/null || true
printf '\n[filesystem-specific]\n'
fstype=$(findmnt -T "$root" -n -o FSTYPE 2>/dev/null || true)
case "$fstype" in
  ext4)
    command -v tune2fs >/dev/null && tune2fs -l "$source_dev" 2>/dev/null | sed -n '1,80p' || true
    ;;
  xfs)
    command -v xfs_info >/dev/null && xfs_info "$root" 2>/dev/null || true
    ;;
  btrfs)
    command -v btrfs >/dev/null && btrfs filesystem show "$root" 2>/dev/null || true
    command -v btrfs >/dev/null && btrfs filesystem usage -b "$root" 2>/dev/null || true
    ;;
esac
printf '\n[O_TMPFILE basic open probe]\n'
ROOT="$root" python3 - <<'PY'
import errno, os
root=os.environ['ROOT']
flags=getattr(os,'O_TMPFILE',0)|os.O_RDWR|os.O_CLOEXEC
try:
    fd=os.open(root,flags,0o600)
except OSError as e:
    print(f'supported_open=false errno={e.errno} name={errno.errorcode.get(e.errno)} detail={e}')
else:
    print('supported_open=true')
    os.close(fd)
PY
printf '\n[denylist decision]\n'
case "$fstype" in
  ext4|xfs|btrfs) printf 'fstype_candidate=true\n' ;;
  nfs*|cifs|smb3|fuse.*|overlay|virtiofs|9p)
    printf 'fstype_candidate=false reason=non-target-layer\n'
    exit 78
    ;;
  *)
    printf 'fstype_candidate=false reason=unresearched-filesystem\n'
    exit 78
    ;;
esac
printf 'NOTE: this inventory is not a durability proof; run the full fault matrix.\n'
