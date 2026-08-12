# LLM Wiki: 200–230 GB Model Selection

Open **`site/index.html`** for the offline styled wiki, or **`Home.md`** for the GitHub-Wiki-compatible source.

## Contents

- standard wiki files: `Home.md`, `_Sidebar.md`, `_Footer.md`
- candidate cards under `candidates/`
- methods and comparison pages under `pages/`
- machine-readable CSV/JSON under `data/`
- artifact manifests under `manifests/`, revision-pinned where an immutable revision was captured
- reproducibility scripts under `scripts/`
- offline static site under `site/`

## Interpretation

The target band is **decimal GB as displayed by the artifact publisher**. Capacity tables use **GiB**. Because publisher UI sizes are rounded, the planning convention is:

```text
weight_plan_GiB = ceil(display_GB × 1,000,000,000 / 2^30) + 1 GiB
```

The extra GiB covers display rounding, metadata, mmap/page alignment, and small loader differences. Runtime/OS reserves are separate.

## Rebuild

```bash
python scripts/calculate_capacity.py
python scripts/build_static.py
python scripts/self_check.py
```

## Integrity

`SHA256SUMS` covers the research package itself. Model-weight hashes are not fabricated: known LFS pointers are recorded where captured, and `scripts/refresh_hf_manifests.py` resolves exact Hugging Face LFS OIDs and byte sizes before download.
