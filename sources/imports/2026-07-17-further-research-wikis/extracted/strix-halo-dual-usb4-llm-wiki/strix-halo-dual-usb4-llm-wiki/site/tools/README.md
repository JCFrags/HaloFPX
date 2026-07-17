# Tools

- `cost_model.py` — formula calculator and worked-example generator. It embeds architecture fields, not benchmark performance.
- `fit_link_model.py` — fits measured `t(V)=ell+V/B` inputs by message-size region.
- `build_site.py` — builds the pre-rendered HTML wiki from Markdown.
- `validate_wiki.py` — validates file structure, internal links, placement YAML, and evidence labels.
- `serve.py` — serves the prebuilt site on localhost.
- `probe_linux_usb4.sh` / `probe_windows_usb4.ps1` — non-destructive topology inventory.
- `benchmark_linux_links.sh` — command template for one-link and simultaneous-link tests.

Run `python cost_model.py --list-models` and `python cost_model.py --model llama31-8b` for a machine-readable calculation.
