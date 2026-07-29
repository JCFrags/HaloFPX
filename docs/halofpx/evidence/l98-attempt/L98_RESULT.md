# L98 terminal result

Status: **NOT PROMOTED; no retry**.

## Exact identity

- accepted base: `647f3d4bfd4574e6b5086c42407116cbb5ce843b`
- source root: `073a32d4ed86ceb38c6c59c21ae786d7c2ce5d0d38ef183350ef817b5469afbc`
- build ID: `d1460995dab7a6fdf71e0301a90a1516c5b3ab4cf985c5a7f9731ce2588dff9b`
- worker: `18b127bc0970383ce897d461f3ac87d6cdb311ebf5c41a62939b5d286cfc5662`
- canary: `d9aa062faafc813408adf485984be15e107d84127138b1492dde0b5731b1626c`
- model: `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`
- source archive: 204707328 bytes,
  `8d2ca5170a6f942bf65538c2a4c12ef01d1fd2f124fa6831fb599f3769767fb7`
- build archive: 223744000 bytes,
  `a2716b8d859416ce98e52699c41904d84bb31c5e900282f6add62dc75848ccd5`

The relocatable staged-runtime package gate passed before production shutdown.

## Runtime result

Residency A captured successfully with authenticated token `21549` and suffix
` alpha`. Residency B was genuinely fresh, authenticated, executed, and
terminalized. The corrected result parser accepted exact equality between the
durable JSON and emitted restore result, including the empty
`prompt_chunk_sizes` value. The corrected systemd custody path retained exact
restore InvocationID `6e9bcb29304b4d028754e3bdfa3935b6`, PID `2140538`,
cursor, active/exited success tuple, and journal before unload.

The restored token was `9283` and the retained suffix was UTF-8 `计划`. It does
not equal the residency-A reference. Although the control, local-state, and
component-manifest hashes shown in the two result records agree, exact
token/output equality fails, so L98 provides a negative cache-correctness
result.

Five authenticated 4200-byte server authorities were retained (four capture,
one restore). The response streams were copied and durably hashed, but their
combined verification refused with:

`record event sequence is missing, duplicate, or out of order`

The retained streams contain four complete capture response productions plus
one restore production. This response-custody verifier boundary is separate
from, and cannot explain away, the already observed deterministic output
mismatch. No zero-legacy-GET/SET acceptance claim is made because the composed
result was not produced after the refusal.

