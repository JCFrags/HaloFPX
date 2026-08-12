# GPL operational behavior and clean-room role separation

## Distinguish expression from requirements

The GPL scripts are primary evidence of executable operational behavior and source expression. A clean-room effort may use approved observations, interoperability facts, command semantics, test results, and independently written requirements without silently copying GPL code, comments, or documentation expression. Whether a particular process is adequate is a human legal/maintainer decision.

## Minimum role controls

1. **Source observer / requirements author:** documents behavior and facts, with exact donor citations, while excluding code/text expression from the requirements package.
2. **Independent implementer:** works only from approved requirements, standards, and test vectors; has no donor-source access under the protocol.
3. **Validator:** performs black-box or approved comparative tests without passing donor snippets to implementers.
4. **Build/release:** maps approved source to binaries and assembles SBOM/notices.
5. **Maintainer/legal reviewer:** approves roles, access, exceptions, final admissibility, and release model.

## Evidence to retain

- Repository ACLs and access logs.
- Named role assignments and conflict declarations.
- Dated source-observation notes and expression-exclusion review.
- Requirements/test-vector provenance.
- Independent design and commit history.
- Communication channel separation.
- Black-box validation logs.
- Final reviewer signoff.

The role matrix is in `manifests/clean-room-role-matrix.template.csv`.
