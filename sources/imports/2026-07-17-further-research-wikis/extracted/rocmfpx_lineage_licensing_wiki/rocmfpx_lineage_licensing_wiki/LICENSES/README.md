# License Text Index

This research package does not duplicate every upstream legal text. A release candidate should place exact copies in this directory and verify them against the listed primary sources.

| SPDX/expression | Primary text/source | Observed use |
|---|---|---|
| MIT | https://opensource.org/license/mit/ and immutable repository `LICENSE` files | ROCmFPX, CachyLLama, llama.cpp, ggml, several vendors |
| Apache-2.0 | https://www.apache.org/licenses/LICENSE-2.0 | OpenVINO, templates, converter/tooling files |
| Apache-2.0 WITH LLVM-exception | https://llvm.org/LICENSE.txt | SYCL/LLVM-derived files |
| BSD-2-Clause | Exact xxHash header in source tree | xxHash |
| MIT-0 | Exact miniaudio header/upstream source | Recommended election for miniaudio |
| GPL-3.0-or-later | https://www.gnu.org/licenses/gpl-3.0.html plus project grant/header | llama-ai scripts/source declaration |
| CC-BY-NC-SA-4.0 | https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.en | llama-ai documentation declaration |
| Public-domain/Unlicense-style | Exact source header/dedication | subprocess.h and dual-option vendors |

[RECOMMENDATION] Copy exact texts from authoritative sources into the release, retain original filenames where possible, and hash them in the release manifest.
