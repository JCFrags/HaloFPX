---
type: intake-integrity-review
status: reviewed
created: 2026-07-17
target: sources/imports/2026-07-17-further-research-wikis
scope: twelve preserved research Wiki packages
decision: accept-as-candidate-evidence-with-conditions
---

# Twelve research Wiki package integrity review

## Decision

Accept the twelve preserved ZIPs as **candidate evidence**, not as canonical Wiki material or executable tooling. At final capture, all twelve source-archive SHA-256 values matched the import receipt, all embedded checksum manifests verified, and every extracted file matched its corresponding ZIP entry. Selective promotion still requires content, licensing, applicability, and claim-level review.

Conditions before promotion or execution:

1. Treat `source-archive/` as the immutable authority and `extracted/` as a disposable review view.
2. Do not promote generated `site/`, HTML, cache, validation-result, or vendored content when a canonical source page or tool is available.
3. Resolve the package-license gaps recorded below before copying code, scripts, configuration, fixtures, diagrams, or substantial prose.
4. Repair or exclude the two broken links in the nested generated-site README identified below.
5. Never use the included HaloFPX fixture HMAC key for production; it is intentionally public test material.
6. Do not execute imported scripts directly from the evidence tree. Copy a reviewed subset to an experiment/tooling area, pin dependencies, and record the validation environment first.

This review follows the project route `sources -> wiki -> implementation decisions` and the Agent Harness route `sources -> wiki -> knowledge -> candidate procedure -> validated skill`. It does not alter the canonical Wiki.

## Scope and method

- Intake root: `sources/imports/2026-07-17-further-research-wikis/`
- Raw authority: `source-archive/*.zip`
- Review view: `extracted/<archive-stem>/<package-root>/`
- ZIP entries: 1,461 total = 1,293 file entries + 168 directory entries
- Script-like/source files inventoried: 147 (`.py`, `.sh`, `.ps1`, `.js`, and one `.c`); no `.exe`, `.dll`, or `.so` files were found.
- Imported code executed by this audit: none
- Link scan: relative inline Markdown links, Wiki links, and HTML `href`/`src` paths; external URLs and anchor-only links were excluded. Anchor existence and remote URL availability were not tested.
- Secret scan: filename and text-pattern indicators only. No credentials were tested or used.

The archive-vs-extraction comparison hashes the uncompressed bytes of every ZIP **file** entry and its extracted counterpart; directory entries have no extracted file bytes to compare. Each package checksum manifest was also independently parsed and checked. The checksum file itself is the expected single file omitted from each self-referential manifest. Final aggregate results were 1,293/1,293 extracted file-content matches, zero missing files, zero content mismatches, zero extras, and 1,281/1,281 manifest-listed hashes passing.

## Integrity summary

| Package | ZIP files | Manifest result | Final extraction parity | Relative path links | Script-like files |
|---|---:|---|---|---:|---:|
| `cross-fork-llama-conformance-wiki` | 131 | 130/130 pass | 131/131, no extra or changed files | 0 broken of 61 checked | 31 |
| `dual-node-strix-halo-validation-wiki-2026-07-17` | 124 | 123/123 pass | 124/124, no extra or changed files | 0 broken of 137 checked | 16 |
| `dual-usb4-strix-halo-wiki` | 112 | 111/111 pass | 112/112, no extra or changed files | 0 broken of 80 checked | 23 |
| `halofpx-kv-cache-wiki` | 96 | 95/95 pass | 96/96, no extra or changed files | 0 broken of 137 checked | 7 |
| `HaloKV-LLM-Wiki` | 89 | 88/88 pass | 89/89, no extra or changed files | 0 broken of 109 checked | 5 |
| `llama-ai-cachyllama-feature-inventory` | 68 | 67/67 pass | 68/68, no extra or changed files | 0 broken of 802 checked | 11 |
| `llm-wiki-200-230gb` | 118 | 117/117 pass | 118/118, no extra or changed files | 0 broken of 849 checked | 9 |
| `rocmfpx_lineage_licensing_wiki` | 31 | 30/30 pass | 31/31, no extra or changed files | 0 broken of 27 checked | 4 |
| `ROCmFPX-CachyLLama-Integration-Wiki` | 47 | 46/46 pass | 47/47, no extra or changed files | 0 broken of 195 checked | 0 |
| `ROCmFPX-llama-wiki` | 89 | 88/88 pass | 89/89, no extra or changed files | 0 broken of 1,054 checked | 3 |
| `strix-halo-dual-usb4-llm-wiki` | 259 | 258/258 pass | 259/259, no extra or changed files | **2 broken** of 1,718 checked | 22 |
| `strix-halo-gfx1151-llm-wiki-2026.07.17` | 129 | 128/128 pass | 129/129, no extra or changed files | 0 broken of 1,506 checked | 16 |

All twelve preserved ZIP hashes matched the values in `import-receipt.md` at final capture:

| Archive | Final SHA-256 |
|---|---|
| `ROCmFPX-CachyLLama-Integration-Wiki.zip` | `3802e84a189f6d06cde894dd468a58fee5fe042d1cf9617b11dbbda42dee481e` |
| `cross-fork-llama-conformance-wiki.zip` | `060a61cfe5372248f26283d27cec6e22412986264362d000c3ee07af3dc1369c` |
| `strix-halo-dual-usb4-llm-wiki.zip` | `c1a453b3655f67d5a8277e249d062a1622810b87f5cb67d84644d74dbbc39fe9` |
| `llama-ai-cachyllama-feature-inventory.zip` | `48a3dccd365c5623ef5a669c4d2df8bcb657b552d2cf60cd2b306ab360d9ce54` |
| `rocmfpx_lineage_licensing_wiki.zip` | `8754e7525335185343b160b638550d726776aa42443b28621732615f571a5ea1` |
| `strix-halo-gfx1151-llm-wiki-2026.07.17.zip` | `c855e1d0d7a9c8cc9e6639d596bed932f7856c8fde5203f6499de0cb227b1088` |
| `ROCmFPX-llama-wiki.zip` | `14e5b07a3d8d80348bbe4bb8858b7d0c26474ce8c3d4e08f531b75718bd72b4f` |
| `HaloKV-LLM-Wiki.zip` | `4090b295a558001bbc8f0a2daf7b91f02301c2592fcaccdad1f142068e6c6f8a` |
| `dual-node-strix-halo-validation-wiki-2026-07-17.zip` | `9cef69fb935c5f2284d4fe4343bc6b26b0dfe7669b1c6c563aa7ebf44984a1cf` |
| `halofpx-kv-cache-wiki.zip` | `8874bd1a715e1f9d7910091b360d13a5154268b03debbd7351979eead74396a7` |
| `dual-usb4-strix-halo-wiki.zip` | `48ca62a576e4bc9e30171b79de962ce7f6952cc56f51b6e0bb794ba9a2de0776` |
| `llm-wiki-200-230gb.zip` | `720a50cc451c8e6b85fdca6331c87b01667d53c8a687e1ff9df96b8e0b6279f1` |

### Concurrent-work observation

During an intermediate audit snapshot, the extracted `strix-halo-dual-usb4-llm-wiki` tree had 267 files while its ZIP had 259 file entries. The eight extra paths were:

- `.pytest_cache/.gitignore`
- `.pytest_cache/CACHEDIR.TAG`
- `.pytest_cache/README.md`
- `.pytest_cache/v/cache/nodeids`
- `tests/__pycache__/test_cost_model.cpython-314-pytest-9.0.3.pyc`
- `tests/__pycache__/test_link_fit.cpython-314-pytest-9.0.3.pyc`
- `tools/__pycache__/cost_model.cpython-314.pyc`
- `tools/__pycache__/fit_link_model.cpython-314.pyc`

Two archive-owned files also differed at that snapshot:

| Path | Preserved ZIP SHA-256 | Transient extracted SHA-256 |
|---|---|---|
| `SHA256SUMS` | `cd05712d18dd67013fe6a6a8cb39d84c79d7fc8ebaac511bd6794d1da7db4029` | `3360044f056ec32842f634d0d9928e49d8d9b8819ddc13f58135a6262d3cae39` |
| `VALIDATION_REPORT.txt` | `e14609ca7f0158babff296deea209e4cf1549e737fa2b1617d56b241f8ef528f` | `c6178ad039a2913bcef1b4ad8c1534402cd22858d4c760bef5b82b2142828535` |

The process evidence is specific but not sufficient to identify an invoking process ID or agent:

- `.pytest_cache/v/cache/nodeids` and the `pytest-9.0.3` bytecode names are filesystem evidence that pytest ran in that tree.
- `tools/validate_wiki.py` lines 209-214 enumerate files and rewrite `SHA256SUMS`; lines 235-243 construct and write `VALIDATION_REPORT.txt`.
- The package `Makefile` target `all` runs `build`, `validate`, and `test`, while `validate` invokes `python tools/validate_wiki.py` and `test` invokes `python -m pytest -q`.

This evidence is consistent with the package validator and pytest having been invoked, either separately or through `make all`. No process/PID or shell-history record was captured, so the exact command and actor are **not proven** and are not attributed here. Concurrent workspace work restored the extracted tree from the preserved ZIP before closeout. Final byte-for-byte comparison found 259/259 matching file entries for this package, with zero missing, changed, or extra files. The audit itself did not write to any package. This event confirms that `extracted/` is mutable and must not be treated as the provenance authority.

## Per-package findings

### `cross-fork-llama-conformance-wiki`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: `Research-Sources.md` and `NOTICE.md`; no conventional top-level `LICENSE` or `COPYING` file.
- Tooling: 27 Python and 4 shell files, including a harness, tests, fixture generators, CI scripts, and manifest verification tools. Treat all as unexecuted candidate code.
- Generated duplication: none detected by generated-directory or side-by-side Markdown/HTML checks.

### `dual-node-strix-halo-validation-wiki-2026-07-17`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: strong provenance structure (`references/Sources.md`, `references/sources.yaml`, `references/claim-source-map.csv`, and provenance policy material), but no conventional package license/notice file. **License status is unresolved for reuse.**
- Tooling: 11 Python and 5 shell files. Several scripts describe host, network, cache, and fault-injection operations; do not execute from intake.
- Generated duplication: none detected.

### `dual-usb4-strix-halo-wiki`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: `LICENSE`, `CITATIONS.md`, and `SOURCE-SNAPSHOT.md` are present.
- Tooling: 9 Python, 13 shell, and one C source file (`tools/mptcp_smoke.c`). Scripts include network configuration, MPTCP, USB4STREAM, RPC, and benchmark helpers and therefore require a reviewed experiment wrapper before use.
- Generated duplication: none detected.

### `halofpx-kv-cache-wiki`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: `research/source-lock.json` and `NOTICE.md`; no conventional `LICENSE` or `COPYING` file.
- Tooling: 6 Python and one shell file, plus binary cache fixtures and generated validation results.
- Generated content: `site/` contains 2 files totaling 270,991 bytes; no byte-identical counterpart outside the site was found, but it is generated presentation content and should not be promoted over `docs/`.
- Sensitive-looking material: `validation/fixtures/halofpx/keys/fixture-manifest-hmac.key` is a 65-byte deterministic fixture. `docs/storage-schemas.md` explicitly calls it public test material and says production keys must come from a protected key manager. Retain only with the fixture suite and never treat it as a secret or deployable key.

### `HaloKV-LLM-Wiki`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: `LICENSE.md`, `CITATION.cff`, `raw/processed/source-catalog.md`, and `wiki/References.md` are present.
- Tooling: 5 Python files for a reference model, fuzzing/tests, linting, and deep validation.
- Generated duplication: none detected.

### `llama-ai-cachyllama-feature-inventory`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: `manifests/source-pins.json`, `data/source-files.csv`, and `notices/LICENSE-NOTICE.md` are present. Review the notice's component-specific terms before copying donor material.
- Tooling: 8 shell examples, one Python example, and 2 JavaScript site assets.
- Secret indicators: three examples use the literal placeholder `change-me` as an API-key fallback. No credential-like value was found. Any promoted example should fail closed when the environment variable is absent rather than silently send `change-me`.
- Generated content: one HTML file; no nested `site/` tree.

### `llm-wiki-200-230gb`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: `CITATION.cff`, `data/sources.json`, and `LICENSE-NOTES.md`; no conventional package-wide `LICENSE` file. Treat the notes as a routing aid, not automatic reuse permission.
- Tooling: 6 Python, one shell, and 2 JavaScript files.
- Generated content: `site/` contains 51 files totaling 488,410 bytes; 24 are byte-identical copies of files outside `site/`, primarily assets and structured data. Promote source pages/data, not the prebuilt site copy.

### `rocmfpx_lineage_licensing_wiki`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: extensive license/provenance analysis (`license_compatibility_matrix.md`, `notices_and_attribution.md`, `provenance_map.md`, ledgers, and a provenance template), but no conventional package-level license for the Wiki's own 2 Python and 2 shell files. **Do not assume analysis of upstream licenses licenses this package's original material.**
- Generated duplication: none detected.

### `ROCmFPX-CachyLLama-Integration-Wiki`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: `Repository-and-Provenance.md`, `Source-Register.md`, `Licensing-and-Provenance-Gates.md`, and a provenance template; no conventional package license file.
- Tooling: no script-like files detected.
- Generated duplication: none detected.

### `ROCmFPX-llama-wiki`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: source ledger in CSV/JSON and citation-coverage data; no conventional package license/notice file. **License status is unresolved for the included Python and JavaScript.**
- Tooling: one Python server and 2 JavaScript assets.
- Generated content: 29 HTML files; 27 have a same-directory Markdown page with the same stem. Treat Markdown and structured ledgers as canonical candidates and HTML as generated presentation output.

### `strix-halo-dual-usb4-llm-wiki`

- Integrity: clean at final capture; see the concurrent-work observation above.
- Provenance/legal metadata: `LICENSE.md`, `CITATION.cff`, `CITATIONS.md`, and `docs/sources.md` are present. Vendored MathJax has its own license file.
- Tooling: 12 Python, 4 shell, 2 PowerShell, and 4 JavaScript files as packaged; many are duplicated inside `site/`.
- Generated duplication: `site/` contains 144 files totaling 9,357,202 bytes; 105 are byte-identical to files outside `site/`. It duplicates documentation, tools, data, and vendored content and should be excluded from canonical promotion.
- Broken relative links: `site/README.md` links to `OPEN_WIKI.html` and `site/index.html` as if the README were at package root. From `site/README.md`, those resolve to nonexistent `site/OPEN_WIKI.html` and `site/site/index.html`. The actual targets are at package root and `site/index.html`, respectively.

### `strix-halo-gfx1151-llm-wiki-2026.07.17`

- Integrity: clean manifest and extraction parity.
- Provenance/legal metadata: `LICENSE`, `NOTICE`, `CITATION.cff`, `docs/sources.md`, and source registries in CSV/JSON/YAML.
- Tooling: 4 Python, 11 shell, and one JavaScript file. Scripts include build, install, download, USB4 setup, diagnostics, and smoke-test operations and require isolated review before use.
- Generated content: `site/` contains 33 files totaling 457,619 bytes. No byte-identical outside copies were found because the site is rendered HTML, but it remains generated presentation content.

## Cross-package generated-content policy

Do not merge whole research-package roots into the canonical HaloFPX Wiki. Use the following default selection:

| Prefer | Exclude unless specifically justified |
|---|---|
| source Markdown, source ledgers, schemas, raw evidence, exact source pins, licenses/notices | `site/`, generated HTML, `.pytest_cache/`, `__pycache__/`, `.pyc`, generated reports, regenerated manifests, vendored assets already available from their canonical source |

This reduces duplicate search hits, prevents stale presentation copies from outranking source pages, and keeps code review focused on the actual candidate artifacts.

## Suspicious path and secret-indicator review

Text and metadata scans found:

- 0 Windows user-profile absolute paths (`C:\\Users\\...`), macOS user paths (`/Users/...`), or Linux home-directory paths (`/home/...`);
- 0 private-key headers, AWS access-key IDs, GitHub token forms, OpenAI-style secret-key forms, or long bearer-token forms;
- 3 literal API-key fallback placeholders, all `change-me`, in the llama-ai/CachyLlama examples;
- 1 `.key` file, the documented public deterministic HaloFPX test fixture described above;
- 0 native executable/shared-library payloads (`.exe`, `.dll`, `.so`).

This is not a guarantee that arbitrary prose or binary fixtures contain no sensitive data. It is a bounded metadata/text-pattern screen appropriate to an untrusted documentation intake.

## Machine-reproducible commands

Run these commands from `C:\\Users\\britt\\Documents\\Custom_Inference_Project` in PowerShell. They are read-only and do not invoke any imported package script.

### Verify preserved ZIP hashes against the receipt

```powershell
$receipt = Get-Content -LiteralPath 'sources\imports\2026-07-17-further-research-wikis\import-receipt.md'
$expected = @{}
foreach ($line in $receipt) {
    if ($line -match '^\| `([^`]+\.zip)` \| `([0-9a-f]{64})` \|') {
        $expected[$Matches[1]] = $Matches[2]
    }
}
Get-ChildItem -LiteralPath 'sources\imports\2026-07-17-further-research-wikis\source-archive' -Filter '*.zip' -File |
    Sort-Object Name |
    ForEach-Object {
        $actual = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        [pscustomobject]@{ Archive = $_.Name; Match = ($expected[$_.Name] -eq $actual); SHA256 = $actual }
    }
```

Expected: 12 rows and every `Match` value is `True`.

### Verify every ZIP entry against the extracted review view

```powershell
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archiveRoot = Resolve-Path 'sources\imports\2026-07-17-further-research-wikis\source-archive'
$extractRoot = Resolve-Path 'sources\imports\2026-07-17-further-research-wikis\extracted'
$sha = [System.Security.Cryptography.SHA256]::Create()
Get-ChildItem -LiteralPath $archiveRoot -Filter '*.zip' -File | Sort-Object Name | ForEach-Object {
    $package = [IO.Path]::GetFileNameWithoutExtension($_.Name)
    $destination = Join-Path $extractRoot $package
    $zip = [IO.Compression.ZipFile]::OpenRead($_.FullName)
    try {
        $entries = @($zip.Entries | Where-Object Name)
        $zipNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $missing = 0; $changed = 0
        foreach ($entry in $entries) {
            [void]$zipNames.Add($entry.FullName)
            $path = Join-Path $destination $entry.FullName.Replace([char]47, [IO.Path]::DirectorySeparatorChar)
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $missing++; continue }
            $stream = $entry.Open()
            try { $zipHash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
            finally { $stream.Dispose() }
            if ($zipHash -ne (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash) { $changed++ }
        }
        $extras = @(Get-ChildItem -LiteralPath $destination -Recurse -File -Force |
            ForEach-Object { $_.FullName.Substring($destination.Length + 1).Replace([char]92, [char]47) } |
            Where-Object { -not $zipNames.Contains($_) })
        [pscustomobject]@{ Package=$package; ZipFiles=$entries.Count; Missing=$missing; Changed=$changed; Extras=$extras.Count }
    }
    finally { $zip.Dispose() }
}
$sha.Dispose()
```

Expected: `Missing=0`, `Changed=0`, and `Extras=0` for all packages.

### Verify package checksum manifests

```powershell
$base = Resolve-Path 'sources\imports\2026-07-17-further-research-wikis\extracted'
Get-ChildItem -LiteralPath $base -Directory | Sort-Object Name | ForEach-Object {
    $package = $_
    $root = Get-ChildItem -LiteralPath $_.FullName -Directory | Select-Object -First 1
    $manifest = Get-ChildItem -LiteralPath $root.FullName -File |
        Where-Object Name -Match '^(?i)(MANIFEST\.sha256|SHA256SUMS|checksums\.sha256)$' |
        Select-Object -First 1
    $checked = 0; $missing = 0; $mismatch = 0; $unparsed = 0
    foreach ($line in Get-Content -LiteralPath $manifest.FullName) {
        if ($line -match '^([0-9a-fA-F]{64})\s+\*?(.+?)\s*$') {
            $relative = $Matches[2].Trim()
            if ($relative.StartsWith('./')) { $relative = $relative.Substring(2) }
            $path = Join-Path $root.FullName $relative.Replace('/', [IO.Path]::DirectorySeparatorChar)
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $missing++; continue }
            if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $Matches[1]) { $mismatch++ }
            $checked++
        }
        elseif ($line.Trim() -and -not $line.Trim().StartsWith('#')) { $unparsed++ }
    }
    [pscustomobject]@{ Package=$package.Name; Checked=$checked; Missing=$missing; Mismatch=$mismatch; Unparsed=$unparsed }
}
```

Expected: the checked counts in the integrity table and zero missing, mismatched, or unparsed entries.

### Inventory executable/script-like content without execution

```powershell
$base = Resolve-Path 'sources\imports\2026-07-17-further-research-wikis\extracted'
Get-ChildItem -LiteralPath $base -Directory | Sort-Object Name | ForEach-Object {
    $root = Get-ChildItem -LiteralPath $_.FullName -Directory | Select-Object -First 1
    $files = @(Get-ChildItem -LiteralPath $root.FullName -Recurse -File -Force |
        Where-Object Extension -Match '^(?i)\.(py|pyw|sh|bash|zsh|fish|ps1|psm1|bat|cmd|js|ts|exe|com|dll|so|c)$')
    [pscustomobject]@{
        Package = $_.Name
        Count = $files.Count
        Types = (($files | Group-Object { $_.Extension.ToLowerInvariant() } | Sort-Object Name |
            ForEach-Object { "$($_.Name)=$($_.Count)" }) -join ', ')
    }
}
```

### Repeat the bounded secret/path indicator scan

```powershell
$root = 'sources\imports\2026-07-17-further-research-wikis\extracted'
rg -n -H -I --glob '!*.png' --glob '!*.svg' --glob '!*.bin' --glob '!*.pyc' `
  '(?i)([A-Z]:\\Users\\|/home/[A-Za-z0-9._-]+/|/Users/[A-Za-z0-9._-]+/)' $root
rg -n -H -I --glob '!*.png' --glob '!*.svg' --glob '!*.bin' --glob '!*.pyc' `
  '(?i)(-----BEGIN (RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----|AKIA[0-9A-Z]{16}|gh[pousr]_[A-Za-z0-9]{30,}|sk-[A-Za-z0-9]{20,}|Bearer\s+[A-Za-z0-9._~+/-]{16,})' $root
Get-ChildItem -LiteralPath $root -Recurse -File -Force |
  Where-Object Name -Match '(?i)(secret|credential|\.key$|\.pem$|id_rsa|id_ed25519)'
```

## Closeout

- Raw sources preserved: yes
- Canonical Wiki edited: no
- Imported scripts executed: no
- Final extracted parity rechecked: yes
- Review outcome: accept as candidate evidence with the conditions above
- Required next gate: semantic/licensing/provenance review before selective promotion
