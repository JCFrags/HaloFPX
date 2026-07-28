# L68 Stories15M vertical slice

Status: **NOT PROMOTED**

L68 used the disposable two-host Stories15M fixture to compare one bounded
deterministic request with the ADR-0049 foundation off and on. Production was
read-only throughout.

## Result

- Feature off completed normally with five decoded prompt tokens, generated
  token `29916` (`output_hex=78`, the text `x`), and an empty authority field.
- Feature on negotiated the retained L42/L44 composition and registered the
  real scheduler census (11 roots and 36 exclusions), but `llama_decode`
  returned `-3` before authenticated graph execution or a model result.
- The client recorder then refused its abort production because the expected
  census was still `0/0`; the server independently refused its abort
  production. The later teardown messages are consequences, not evidence of
  a malformed successful response.

The source-proven P2 is that expected register/exclude counts are installed
only after mutable prepare succeeds. An abort after registration but before
prepare therefore cannot close the exact grammar, loses the terminal
authority record, and leaves the primary runtime refusal ambiguous. This is a
semantic foundation defect, so no further model retry or in-milestone
foundation correction was attempted.

## Identity and evidence

- Base commit: `38c7d4ad7802116daac83e3927a1e1ea42fec8c9`
- Stories15M artifact SHA-256:
  `66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`
- Build provenance source root:
  `8e80cd3e39c5c7c11ee4e195277eb59ebe147b99590b8adc885e682482ee5abf`
- Build ID:
  `dd008b1d7f1f3177ea43c99329506a9330c3ef8138eec9662ea1159704a95a94`
- Manifest SHA-256:
  `24b58cdffc43c426cfb282e67276ca247d8cd3346ce40650420a0498218b87dd`
- Feature-off log SHA-256:
  `01ffe40671c585e7e4a0b597c8a811a7136e65bc05e7b987e242a62247e94341`
- Feature-on client journal SHA-256:
  `5683dd25b80cae65547cc4425befc6ac26989d2f678b5b9b23dac31f11f3f599`
- Final production snapshot SHA-256:
  `511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`

Timing in the retained logs is diagnostic metadata only. L68 makes no
performance or cache claim.

## Production reconciliation

The preflight and final snapshots agree:

- nimo-1 coordinator: PID `2356329`, listener `8081`, HTTP `200`,
  `NRestarts=0`.
- nimo-2 worker: PID `1535639`, listener `50052`, `NRestarts=0`.

## Independent review

The focused independent reviewer returned **NOT PROMOTED**, found no P1, and
confirmed the correctness P2 above. The reviewer also verified feature-off
inertness/determinism, real feature-on L42/L44 setup, evidence identity, and
production non-interference.
