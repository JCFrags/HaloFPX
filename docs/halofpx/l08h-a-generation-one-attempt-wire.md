# L08h-a generation-one attempt wire

L08h-a supplies the authenticated state record needed by the upcoming Linux
generation-one full-v1 publication authority. It remains excluded and cannot
read or write a filesystem, publish material, create an anchor, restore state,
or link into the server.

The implementation parent is HaloFPX
`7b61a08748ef72adebb8501feeffd256fbc94e42`, tree
`decfb5016e729a3aeb126dac8630572134077695`. The locked ROCmFPX base remains
`61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

MSVC Release passed the focused canonical/determinism/tamper/wrong-key/
wrong-field/wrong-status/bounds test 1/1. The static isolation contract passed.
Independent review accepted the seam with no P1/P2 finding. It noted that
pending-before-terminal sequencing is intentionally deferred to the Linux
authority state machine.

The next milestone durably publishes this pending record before L08f material
mutation, creates and verifies the exact generation-one anchor, writes a
terminal record before clearing pending, and reconciles restart to exact
success, conclusive abort, or quarantine. Broader crash permutations,
generation advancement, retention, distributed recovery, server linkage, and
product admission remain deferred.

