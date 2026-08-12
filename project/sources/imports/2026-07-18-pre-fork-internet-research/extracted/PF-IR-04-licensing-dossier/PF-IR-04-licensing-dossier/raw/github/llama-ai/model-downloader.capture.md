# llama-ai runtime model downloader behavior

capture_method: GitHub/public-web connector capture
byte_fidelity: line-normalized authoritative connector capture
claim_label: PRIMARY-RUNTIME-DOWNLOAD-BEHAVIOR
source_url: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh
repository: fewtarius/llama-ai
commit_or_revision: 1017f3dfdce3ca2b06aa9007b23295db3bb35722
repository_path: llama-run.sh
git_blob_sha1: 3684a299361b3ca45ff57656a43073f8b9629047
access_date: 2026-07-18

---

The runner queries Hugging Face and permits selection/download of arbitrary model repositories and GGUF files at runtime. The script does not create a release record containing model-card license metadata, repository revision, file digest, tokenizer/template license, or publisher notice. Therefore runtime model downloads are distribution dependencies or user-supplied content, not automatically licensed by the parent GPL file.
