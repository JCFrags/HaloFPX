# Manifest and integrity notes

- `sources.jsonl` is the canonical structured list of normalized official-source captures.
- `provenance.csv` is a flattened source inventory.
- `raw-official-artifacts.csv` records source URL, size and SHA-256 for preserved raw artifacts.
- `files.sha256` covers all pack files except itself and the external ZIP/ZIP sidecar.
- `verify.sh` checks the internal file manifest.
- The ZIP SHA-256 is stored outside the ZIP as `PF-IR-09_NIMO-MME3L_authority-map_2026-07-18.zip.sha256` to avoid a circular digest.

A valid hash proves file identity relative to this manifest. It does not prove vendor authenticity unless the source acquisition and vendor signature chain are independently trusted.
