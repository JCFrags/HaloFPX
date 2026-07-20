# L14Q-H02 HIP four-element KV loader rejection independent review

Status: **ACCEPT after one P2 correction; no remaining P1/P2 blocker.**

The review confirmed the bounded H02 correctness result, matched primary-model
arithmetic, calibrated rejection, dual-rail operation, evidence bundles,
rollback, default-off behavior, provenance boundary, and immutable-reference
integrity. The runtime widening is absent from the implementation repository.
The adverse generation point estimate against feature OFF justifies rejection
without rescue trials; the evidence establishes neither a speedup nor a
reproducible regression.

The initial review found one P2 in the retained generator repair. Its broad
`*.cu` cleanup deleted the two manually maintained ROCmFPX TurboQuant instance
units before regeneration. The corrected generator deletes only translation
units whose first line is its exact ownership header. A fresh disposable run
started with 132 tracked units, retained 132, reproduced all 130 generator-owned
files with zero normalized-content mismatch, and retained both manual units
byte-identically at SHA-256
`1adfd183ea15fd6699dd2f8a8641b177c1bbd4f07fccb8a4e5102b3aab25a463`
and `c46d578ca3570cd82e5f5559620ec884d49c3bd181f078c38583d64370e6c22a`.

No additional runtime or fault matrix is warranted. The promoted milestone is
limited to the generator source-of-truth repair and the rejected-experiment
record. It adds no donor code, GPL llama-ai code, CachyLLama code, dependency,
remote, WebUI, persistent-write authority, deployment, or model mutation.
