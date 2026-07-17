# License and attribution notice

This inventory contains original analysis and links to source repositories. It does not redistribute the assessed source code.

| Material | Pin | Observed license | Portability implication |
|---|---|---|---|
| `fewtarius/llama-ai` orchestration/scripts | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | GPL-3.0; individual scripts commonly identify GPL-3.0-or-later | Do not copy into an MIT-only ROCmFPX distribution without accepting the corresponding GPL obligations or obtaining separate permission. Clean-room reimplementation or a separate GPL deployment package is the preferred path. |
| `fewtarius/CachyLlama` component | `6be745998f568e379ea197fcf827baec73ff9940` | MIT | Source units can generally be adapted into ROCmFPX with copyright/license notices preserved. |
| `charlie12345/ROCmFPX` target | `a5605a72768c6562241b248e268e33dc92787394` | MIT | Target baseline for the port plan. |
| Parent documentation | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | The parent README identifies a separate documentation content license | Paraphrase rather than copying substantial documentation text. |

This is an engineering license inventory, not legal advice. Review repository history, per-file notices, bundled dependencies, model licenses, and distribution form before release.
