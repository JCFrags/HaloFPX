# Artifact Manifests

Each JSON file records the selected Hugging Face repository, revision, path, displayed artifact size, and known LFS pointers. Run `../scripts/refresh_hf_manifests.py --all` to create exact expanded manifests under `manifests/refreshed/` before downloading.

A repository path/revision is an authoritative provenance anchor; a local deployment still requires exact per-file SHA-256 verification.
