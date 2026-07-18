# Literal claim labels

Every material assertion uses one or more literal labels.

| Label | Meaning |
|---|---|
| `[MEASURED]` | Supplied as an observed local identity or version; not independently attested here. |
| `[OFFICIAL]` | Stated by an official OEM, vendor, project, or upstream source. |
| `[EXPLICITLY_APPLIES]` | The source names the exact product family or exact product string. |
| `[FAMILY_APPLIES]` | The source names a broader family mapped to the target by an official source. |
| `[INFERRED]` | Reasoned conclusion from dated/versioned official facts; underlying facts are separately recorded. |
| `[NOT_LISTED_FOR_TARGET]` | Target is absent from a captured affected-product list. This is not an “unaffected” finding. |
| `[SUPPORTED]` | The authority documents the interface or function without an unresolved target capability gate. |
| `[SUPPORTED_IF_PRESENT]` | Kernel/interface support is conditional on hardware capability, firmware ownership, configuration, or driver enablement. |
| `[UNSUPPORTED_IN_THIS_KERNEL_SOURCE]` | A specific source revision contains a negative gate for the tested tuple; other revisions may differ. |
| `[UNKNOWN]` | Evidence is insufficient to classify support or applicability. |
| `[OPEN]` | Public evidence was not located or exact mapping requires local/vendor-only data. |
| `[DISCLOSURE_LIMITED]` | Public source scope or vendor disclosure process limits conclusions. |
| `[FIX_AVAILABLE]` | An official mitigation floor/package is identified; this does not prove the target has received it. |
| `[NO_PUBLIC_PACKAGE]` | No public target-specific package was found at the captured authority. |
| `[SIGNATURE_UNPROVEN]` | Authenticity enforcement for the exact payload/device path was not proven. |
| `[ROLLBACK_UNPROVEN]` | Downgrade/recovery behavior was not proven for the exact device. |
| `[HOLD]` | Do not deploy/update/close the item until specified evidence and human approval exist. |

## Interpretation rule

`[OPEN]`, `[UNKNOWN]`, and `[NOT_LISTED_FOR_TARGET]` are not evidence of no errata, no vulnerability, no correctable errors, or no uncorrectable-fault risk.
