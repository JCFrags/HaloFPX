# Claim-label grammar

The project labels are applied literally:

- `[RELEASED]`: behavior or identity present in a released tag, mainline prepatch, public implementation or documented current interface.
- `[PACKAGED]`: an exact distro package or packaging artifact is publicly recorded.
- `[BACKPORTED]`: a released-base candidate plus a separately sourced correction/patch lane.
- `[PROPOSED]`: mailing-list or review material not merged into the selected released source.
- `[NORMATIVE]`: standards/errata text or revision metadata from the standards body.
- `[MACHINE-TESTED]`: reserved for exact machine logs; intentionally unused here.
- `[UNSUPPORTED]`: the selected released source lacks the requested interface/capability.
- `[SPECULATIVE]`: a hypothesis not established by source, standard or test. The dossier avoids positive speculative deployment claims.
- `[EVIDENCE-GAP]`: evidence is inaccessible, proprietary, not captured, or insufficient to establish the claim.

The CSV claim ledger and source-to-claim manifest are the canonical machine-readable application of these labels.
