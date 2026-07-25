# L43 RPC mutable/update authority adversarial review

Verdict: **NOT PASS**

The independent reviewer inspected the real RPC update seams, client and
server census logic, scheduler binding, view authority, registry lifetime, and
disposable evidence. The reviewer made no source changes.

Accepted partial findings in the rejected candidate:

- scheduler identity was obtained from scheduler-owned admission authority;
- reconstructed server leaves and submitted census entries were checked
  bijectively;
- mutation requests, server readback receipts, and census entries shared the
  same authenticated view-chain digest;
- real exclusion and `SET_TENSOR_HASH` miss/hit behavior were observed.

Material blockers:

- role/exclusion registrations remained process-global and pointer-keyed while
  the API had no session identity, so concurrent RPC attempts could not be
  proven isolated;
- live qualification did not inject malformed/tampered updates, sequence
  duplication/reordering, invalid range/view authority, and incomplete census
  through the actual server handlers.

The reviewer therefore rejected PASS as a reusable authority layer. The
candidate was removed and L43 closes NOT PROMOTED.
