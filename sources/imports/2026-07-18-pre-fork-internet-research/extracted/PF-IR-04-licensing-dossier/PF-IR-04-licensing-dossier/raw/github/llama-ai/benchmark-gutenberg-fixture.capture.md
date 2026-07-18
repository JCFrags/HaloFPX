# llama-ai Project Gutenberg benchmark fixture reference

capture_method: GitHub/public-web connector capture
byte_fidelity: line-normalized authoritative connector capture
claim_label: PRIMARY-TEST-FIXTURE-REFERENCE
source_url: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/benchmark.sh
repository: fewtarius/llama-ai
commit_or_revision: 1017f3dfdce3ca2b06aa9007b23295db3bb35722
repository_path: scripts/benchmark.sh
git_blob_sha1: 6d4743a25906a271f8eef17bbe5e153c3d430704
access_date: 2026-07-18

---

The script identifies its long text fixture as *The Count of Monte Cristo* from Project Gutenberg and downloads `http://aleph.gutenberg.org/cache/epub/1184/pg1184.txt` into `scratch/pg1184.txt`. It does not pin the served bytes, record a digest, preserve a separate Project Gutenberg terms capture, or perform a jurisdiction check.
