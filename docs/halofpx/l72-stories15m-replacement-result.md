# L72 Stories15M feature-on replacement

Status: **NOT PROMOTED**

L72 consumed exactly one feature-on Stories15M replacement using source HEAD
`3f22338f0582640496c4f7033ea1c67132f6ba8d` and the retained L69 runtime
tuple. The accepted L68 feature-off control was not rerun: token `29916`,
output `x` (`78` hex), and empty authority.

The L71 canonical census correction closed the earlier cardinality defect.
The client observed exactly the sealed `13 register / 36 exclude` census.
Authenticated preparation reached the server, which prepared and executed
sequence `1`, graph UID `27`, digest
`03630ca6d2e58012dea774437b6733a373e3a18344ae074e1f80c24e68659f86`.
The client recorded prepare, graph decision, two complete transport stages
(`20` transport events), commit, and end.

The client then refused the successful terminal production and
`llama_decode` returned `-3`; no model token/output or accepted terminal
authority was produced. The exact semantic boundary is ordered-census
grammar incompatibility:

- canonical census sorting orders by backend, provenance, stable tensor/copy
  identity, then disposition;
- runtime L44 iteration consumes that exact order and therefore can interleave
  register and exclude events;
- grammar v1 requires one contiguous register plan followed by one contiguous
  exclusion plan.

Matching cardinalities therefore coexist with a real post-execute order
refusal. No retry or source correction was attempted in L72.

Build identity:

- source root:
  `89557f18139a0fd1c80fac1cd548773f020e016d12b7c584612fb049b76f377f`
- build ID:
  `d69162cc80a071d267e0e885857b0736d496d111a2aec64938fbea5b6e6eca16`
- worker SHA-256:
  `4349e322022ca8125c952f6425f2c83a5c66ceb4e7984d9984ca1e1c367de3cd`
- canary SHA-256:
  `ab0a8be58143a586875714a6f93204b8ab6587eaa73db43815f3a2b6023b4222`

Cleanup was bounded: disposable source/build roots, keys, work/evidence paths,
listeners, and transient units were removed. Production preflight and final
snapshots are byte-identical SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`,
with the retained PIDs, HTTP `200`, and zero restarts.

The focused independent terminal review returned PASS for an
evidence-complete NOT PROMOTED result, with no P1/P2 in the evidence or
classification.

