# P06d MiniMax-M2 Q6 peer-placement canary independent review

Status: **ACCEPT after correction; no remaining source finding.**

The initial review rejected promotion because the first placement helper did
not independently exclude tensor overrides and the topology check relied on
device-name prefixes. It also identified public-name ambiguity for duplicated
tensors. That pre-review exact run is retained but not promoted.

The corrected source filters buffer types by exact requested-device ownership,
rejects all tensor buffer overrides while the seam is enabled, classifies ROCm
and RPC by backend registry identity, and excludes only the three
implementation shadows from public name lookup. The admission log follows
successful exact-Q6 creation and exclusion. Search confirmed that no graph
path consumes the shadows and that authoritative tensors remain available to
adapter lookup.

Strict parsing, exact tuple/type/layer admission, ownership lifetime,
default-off behavior, rollback, and licensing are sound. The corrected remote
GCC build passed, the exact model loaded, and the deterministic request matched
the P04 control and candidate content hash. Claims are correctly limited to
one-layer Q6 peer-data placement with unchanged authoritative inference;
shadow computation and Q6 partition equivalence remain closed.
