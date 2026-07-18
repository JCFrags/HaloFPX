# Pre-fork Internet research import receipt

Date: 2026-07-18

Reason: import the ten completed PF-IR research-agent packages, preserve their
provenance, remove the source ZIPs from Downloads, and route reviewed conclusions
without treating package self-claims as project authority.

## Actions

| Action | From | To | Status | Notes |
|---|---|---|---|---|
| Move | `C:\Users\britt\Downloads\<ten named ZIPs>` | `source-archive/` | complete | Original filenames and bytes preserved |
| Extract | `source-archive/*.zip` | `extracted/<archive-name>/` | complete | Isolated package roots; no bundled script executed |
| Review | package decision/README entry points | `reviews/intake/2026-07-18__pf-ir-returns__review__v01.md` | complete | Candidate evidence only |
| Promote | reviewed cross-package synthesis | Section 85 and `knowledge/` | complete | No source baseline or implementation decision automatically approved |

## Archive verification

| Research ID | Archive | Bytes | Files | SHA-256 |
|---|---|---:|---:|---|
| PF-IR-01 | `PF-IR-01_llm-wiki.zip` | 111,685 | 77 | `981811B1967186E070686497AEE1DEE4E3EA339D9AAD256C078CA0AEBA8F3977` |
| PF-IR-02 | `PF-IR-02-dossier.zip` | 101,281 | 61 | `C60C381F68537DE0B83CC26B676761EDD15EA2B4706D8C55AE0940BF8CEBDDBB` |
| PF-IR-03 | `PF-IR-03_gfx1151_dossier_2026-07-18.zip` | 132,540 | 93 | `ED43AEC150C18B7DCF34F6A058D450378D500B97643BFFF5AADA2477BA4A8094` |
| PF-IR-05 | `PF-IR-05_llm_wiki_2026-07-18.zip` | 136,877 | 102 | `501A5351C2F588B42D48E41CF6B5E58CF9A63C1878306196EA0C95B66F1E8522` |
| PF-IR-06 | `PF-IR-06-HaloKV-crash-durability-wiki-2026-07-18.zip` | 205,952 | 112 | `35E252DB9EB2DE046EEC34A4A77E5F083718BB2249814ACEBAD5CFD89C4F3F27` |
| PF-IR-07 | `HaloKV-PF-IR-07-LLM-Wiki.zip` | 9,232,972 | 159 | `4566CDAB9B309E2B57300082482AF36D0F3FCDCCA02753F939BFCDD8F4D765C5` |
| PF-IR-08 | `PF-IR-08_RCCL_two_host_wiki.zip` | 139,135 | 88 | `65B2FD9FF1C6BD3C2AE335C71B5B622A46DA8EC70153FCA867A004FD9BD70A32` |
| PF-IR-09 | `PF-IR-09_NIMO-MME3L_authority-map_2026-07-18.zip` | 2,297,022 | 80 | `882FC26567F662802D47CF819EA10A249FEB9287AB87C19D4882D325550BEA08` |
| PF-IR-10 | `PF-IR-10-llm-wiki.zip` | 181,742 | 162 | `BCA0959E7E59A7F2CACA0A5EFE9834F3C738DFBADCF821EEFF48DBA8941DFD5B` |
| PF-IR-11 | `PF-IR-11_XDNA2_Linux_Wiki_2026-07-18.zip` | 187,089 | 97 | `4CA9156383F29E702BB88F59ADE993D2DF52DD8F88C788B20AC1C8C64F9385CE` |

Verification:

- all ten source paths existed and resolved inside `C:\Users\britt\Downloads`;
- no ZIP contained an absolute or parent-traversal entry;
- extracted file counts equal the non-directory entry counts for every archive;
- the ten named Downloads paths are absent after the move;
- original archives remain canonical under `source-archive/`;
- PF-IR-04 is not part of this intake and remains pending.

