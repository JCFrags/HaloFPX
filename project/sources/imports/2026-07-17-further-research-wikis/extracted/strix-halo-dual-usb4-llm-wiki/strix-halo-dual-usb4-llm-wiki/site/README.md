# Dual-Strix-Halo USB4 Distributed LLM Wiki

A research and decision repository for distributing large-language-model inference across **two AMD Ryzen AI Max+ 395 (“Strix Halo”) systems connected by two USB4 links**.

The repository does not predict tokens/s. It provides:

- sourced platform and interconnect constraints;
- symbolic communication, synchronization, memory, and break-even models;
- ownership and placement definitions for tensor parallelism, pipeline parallelism, contiguous layer splitting, MoE expert placement, remote speculation, replicated decode, and hybrids;
- model-specific calculated examples;
- execution diagrams and machine-readable placement schemas;
- a benchmark protocol that turns unknown link/runtime behavior into measured inputs;
- explicit go/no-go gates.

## Open the wiki

Open [`OPEN_WIKI.html`](OPEN_WIKI.html), or open [`site/index.html`](site/index.html) directly in a browser. The prebuilt site has a persistent navigation rail, page search, equation rendering, source labels, and print styling.

For a local HTTP server:

```bash
python tools/serve.py
```

Then browse to `http://127.0.0.1:8000/`.

## Rebuild and validate

```bash
python tools/build_site.py
python tools/validate_wiki.py
python -m pytest -q
```

The source pages are under [`docs/`](docs/). A compatible [`mkdocs.yml`](mkdocs.yml) is also supplied for teams that prefer MkDocs Material.

## Scope and evidentiary policy

Every result is marked as one of:

- **SOURCED FACT** — directly supported by a cited primary source;
- **CALCULATED** — arithmetic from sourced architecture fields or declared variables;
- **MEASURED INPUT REQUIRED** — deliberately left unfilled until the target systems are benchmarked;
- **SCENARIO ASSUMPTION** — an illustrative input, never presented as observed performance;
- **DECISION RULE** — a feasibility or break-even inequality.

Nominal USB4 line rate is used only to calculate impossible-to-beat payload-time floors. Effective payload bandwidth, latency, collective phase count, compute time, overlap, and routing locality remain measured inputs.

## Repository map

See [`SUMMARY.md`](SUMMARY.md) for the full page tree and [`MANIFEST.md`](MANIFEST.md) for artifact classes.

## License

Documentation is licensed under CC BY 4.0. Original utility code is licensed under MIT. See [`LICENSE.md`](LICENSE.md).
