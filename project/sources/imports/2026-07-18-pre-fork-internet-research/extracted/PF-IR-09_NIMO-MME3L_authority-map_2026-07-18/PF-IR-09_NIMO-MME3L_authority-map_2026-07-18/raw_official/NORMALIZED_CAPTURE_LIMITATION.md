# Normalized web-capture limitation

This pack preserves the original Crucial/Micron PDF and the LVFS public `PULP_MANIFEST` as raw binary/text artifacts. Official web authorities were accessed on 2026-07-18 through a controlled browsing service that exposes normalized page text and source metadata, not the server's original response bytes or complete HTML/CSS/JavaScript bundle.

Accordingly, files under `*/sources/` are **normalized evidence captures**, not claimed raw HTTP responses. Each capture records the canonical URL, access date, authority, extracted rows, claim labels, and reuse limitations. This limitation prevents false claims of byte-for-byte preservation while retaining the decision-relevant official content.
