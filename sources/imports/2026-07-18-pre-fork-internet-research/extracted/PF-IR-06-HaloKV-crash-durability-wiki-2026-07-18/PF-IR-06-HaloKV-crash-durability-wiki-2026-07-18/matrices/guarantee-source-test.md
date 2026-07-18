# Guarantee → authoritative source → local fault test

Guarantees: **38**. A mapping is not a durability result; the listed test must pass on the deployed profile.

| Guarantee | Required property | Implementation rule | Authoritative sources | Local tests | Portability |
|---|---|---|---|---|---|
| G001 | Detect and record deployed filesystem before enabling writes | Startup profile probe; refuse unknown/nonlocal layers | S001;S002;S003;S004 | T001;T038 | portable gate |
| G002 | Create unpublished inode on destination filesystem | O_TMPFILE after probe, otherwise openat O_CREAT\|O_EXCL in parent | S022;S034 | T002;T003 | portable Linux minimum |
| G003 | Prevent accidental temp-name collision | O_EXCL fallback or unnamed inode | S022;S034 | T003 | portable minimum |
| G004 | Transfer exact object length | Explicit offsets; accumulate positive results; reject zero or terminal error | S023;S016 | T004-T006 | portable minimum |
| G005 | Validate format before publication | Check magic/version/declared length/hash/object key | S023 | T039 | application guarantee |
| G006 | Persist payload and size before name publication | fdatasync; fsync when contract includes other inode metadata | S020;S017;S007 | T014;T015;T019 | portable minimum |
| G007 | Atomic create-if-absent publication | linkat for O_TMPFILE or renameat2 RENAME_NOREPLACE | S021;S022;S010 | T002;T021 | Linux minimum |
| G008 | Atomic whole-object replacement | same-filesystem rename/renameat2; never mutate old object | S021;S033 | T020 | portable minimum |
| G009 | Atomic name exchange when selected | renameat2 RENAME_EXCHANGE, followed by directory sync | S021;S010 | T022 | Linux optimization |
| G010 | Persist namespace mutation | fsync parent directory after link/rename/unlink | S020 | T017-T019 | portable Linux minimum |
| G011 | Avoid cross-filesystem publication | Use same parent directory; reject EXDEV | S021 | T001;T020 | portable minimum |
| G012 | Handle cross-directory changes conservatively | Sync both changed directories; prefer not to use this mode | S020;S021 | T019 | conservative rule |
| G013 | Preserve prior object on write/allocation failure | Publish only after full validation and file sync | S020;S021;S023 | T007-T016 | application guarantee |
| G014 | Fail transaction on ENOSPC/EDQUOT at any stage | Check every syscall/CQE including file and directory sync | S020;S021;S022;S025 | T007-T009 | portable minimum |
| G015 | Fence root on EIO | Stop writer/acks, preserve diagnostics, require recovery | S020;S023 | T010;T011 | policy |
| G016 | Treat close errors safely | Decisive sync first; never retry close | S029 | T011 | portable rule |
| G017 | Optional capacity reservation | fallocate mode 0 or KEEP_SIZE; still write/validate/sync | S025;S007 | T007 | optimization |
| G018 | Do not use discard as durability | Batch fstrim/async discard only as operational policy | S003;S031 | T038 | filesystem policy |
| G019 | Enable DIO only with discovered alignment | statx STATX_DIOALIGN; aligned memory, offset and length | S024 | T023;T024 | optional optimization |
| G020 | Reject partial DIO object after error | Quarantine/rebuild whole immutable object | S023 | T025 | DIO minimum |
| G021 | Avoid coherency hazards | No buffered/DIO/writable-mmap mixture on same object | S022;S024 | T026 | conservative rule |
| G022 | ext4 supported profile | data=ordered, barriers enabled, local block stack; explicit app sync | S002;S005 | T019;T038;T040 | provisional baseline |
| G023 | ext4 denied profile | data=writeback/nobarrier unless separately proven; no DIO assumption under data=journal | S002 | T023;T038 | denylist |
| G024 | XFS supported profile | local XFS, explicit app sync, delayed-allocation fault tests | S003;S020 | T007;T019;T040 | candidate |
| G025 | Btrfs conditional profile | default COW/checksum plus near-full/snapshot/reflink tests; immutable objects | S004;S031 | T037;T040 | conditional |
| G026 | Single local writer | Exclusive OFD lock or flock on dedicated root lock file | S026;S027 | T027;T028 | writer policy |
| G027 | Avoid classic fcntl lock lifetime hazard | Do not use process-associated record lock for root ownership | S027 | T029 | writer policy |
| G028 | io_uring write completion accounting | Check every CQE; require res == requested bytes | S012;S016 | T004;T035 | uring minimum |
| G029 | io_uring persistence sequence | Submit fsync/fdatasync only after exact write success; submit directory fsync after publish | S007;S017;S020 | T018;T019 | uring minimum |
| G030 | io_uring cancellation safety | Track target+cancel CQEs, either order, documented result set | S008;S013 | T030;T031 | uring minimum |
| G031 | io_uring registered-file lifetime | Do not equate close(fd) with cancel or resource release | S009;S013;S015 | T033 | uring minimum |
| G032 | io_uring registered-buffer lifetime | Keep memory valid through operation CQEs; for update/replacement, wait for release tag before reuse; treat explicit unregister and ring teardown according to their distinct documented semantics | S009;S015 | T034 | uring minimum |
| G033 | io_uring linked-operation correctness | Check each CQE; no blind write→fsync link for short-write-sensitive path | S014;S016 | T035 | uring minimum |
| G034 | io_uring teardown | Stop, cancel, drain, unregister, observe release, exit | S013;S015;S018 | T036 | uring minimum |
| G035 | Late CQE isolation | Generation-tag user_data and defer state reuse | S012;S013;S015 | T032;T034 | application rule |
| G036 | Hardware crash boundary | Certify flush/FUA through deployed dm/RAID/virt/device stack | S005 | T040 | deployment proof |
| G037 | Recovery idempotence | Validate and select only complete generations; tolerate orphan temp and duplicate retry | S023 | T018;T039 | application guarantee |
| G038 | No durability claim from this report | Implementation must pass complete matrix on deployed profile | S001-S037 | T001-T040 | release gate |
