# License inventory

This summary mirrors `manifests/licenses.csv`. A complete artifact-level inventory is blocked until the selected packages/tarball and firmware bytes are acquired.

| Item | License/family | Status | Required notice action |
|---|---|---|---|
| TheRock build system | MIT | verified root license | retain copyright and license |
| LLVM/Clang toolchain | Apache-2.0 WITH LLVM-exception | declared by package metadata; source scan required | retain LICENSE.TXT and exception text |
| HIP / ROCR / COMGR ROCm components | primarily MIT | component-level verification required | retain all per-component LICENSE/NOTICE files |
| rocBLAS / hipBLAS / hipBLASLt | primarily MIT | component-level verification required | scan bundled kernels/code generators and notices |
| rocWMMA / rocPRIM / rocThrust / hipCUB | primarily MIT/BSD-family depending component and imported headers | artifact/source scan required | preserve third-party header notices |
| RCCL | MIT (package declared); imported NCCL-derived notices may apply | package declaration verified, notice scan required | retain RCCL and imported-source notices |
| Linux kernel | GPL-2.0-only | project license; exact source tag/signature required | retain COPYING and source/offering obligations as applicable |
| linux-firmware / AMD GPU firmware | mixed per-file licenses | unresolved until exact distro package is captured | retain WHENCE and every per-blob license/notice |
| Mesa / RADV | mixed permissive, predominantly MIT/X11-family | exact source/package scan required | retain top-level and per-driver notices |
| libdrm | MIT-family | bundled/system dependency; exact build selection required | retain package copyright/license files |
| libnuma / numactl | LGPL/GPL mixed by file/package | exact package scan required | retain licenses and notices |
| libelf / elfutils | mixed LGPL/GPL by component | exact package scan required | retain package copyright/license files |
| ELFIO | MIT | package source indicated; verify artifact inclusion | retain MIT notice |
| libpciaccess | MIT-family | package source indicated; verify artifact inclusion | retain copyright/license |
| SuiteSparse | mixed per subproject | exact included subprojects and licenses unresolved | inventory each included SuiteSparse component |
| ncurses | X11-style permissive | verify bundled version and notice | retain ncurses license |
| Boost | BSL-1.0 | build dependency/inclusion to verify | retain Boost license where redistributed |
| FlatBuffers | Apache-2.0 | build/runtime inclusion to verify | retain NOTICE/LICENSE if redistributed |
| fmt | MIT | RCCL/build dependency inclusion to verify | retain MIT notice |
| All artifact files not enumerated above | UNKNOWN UNTIL SCAN | blocking open item | run inventory-artifact.sh and generate-sbom.sh; review every LICENSE/COPYING/NOTICE and package copyright record |
