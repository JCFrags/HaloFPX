# Source-lock commands and tools

## Tool identities

- Git: git version 2.55.0.windows.2
- Python: 3.14.4, MSC v.1944, 64-bit AMD64
- Platform: Windows 11, build family 10.0.26200
- Generator: generate_source_lock.py in this directory

## Executed collection command

From C:\Users\britt\Documents\Custom_Inference_Project:

    python sources\repositories\source-locks\2026-07-17-pre-fork\generate_source_lock.py

The generator used these Git command families against each canonical clone:

    git -C <repo> status --porcelain=v2 --branch
    git -C <repo> for-each-ref --format=<tabular format> refs/heads refs/remotes refs/tags
    git -C <repo> remote -v
    git -C <repo> rev-parse --is-shallow-repository
    git -C <repo> rev-parse --show-object-format
    git -C <repo> rev-parse HEAD
    git -C <repo> symbolic-ref --short HEAD
    git -C <repo> count-objects -vH
    git -C <repo> fsck --full --strict
    git -C <repo> cat-file -t <object>
    git -C <repo> cat-file blob <blob>
    git -C <repo> ls-tree -r -z <revision>
    git -C <repo> show -s --format=<field> <revision>
    git -C <repo> bundle create <package bundle path> --all
    git -C <repo> bundle verify <package bundle path>
    git -C <repo> diff --binary <from> <to> | git patch-id --stable
    git -C <repo> log --reverse --no-merges -p --binary <from>..<to> | git patch-id --stable

Python hashlib SHA-256 was applied to raw Git blob bytes and package files. Get-FileHash -Algorithm SHA256 was used during the independent closeout verification.

## Explicit non-actions

- No fetch, pull, clone, checkout, switch, reset, clean, gc, repack, commit, tag, remote mutation, or push was run.
- No donor or imported repository code was executed.
- No bundle was restored into a worktree.
- No SSH command or other access to nimo-1 or nimo-2 occurred.

