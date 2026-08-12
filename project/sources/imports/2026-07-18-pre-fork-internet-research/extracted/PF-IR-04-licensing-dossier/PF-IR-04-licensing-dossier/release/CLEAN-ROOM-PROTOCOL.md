# Clean-room protocol template

## Purpose

Define a controlled process for independently implementing approved functional requirements without copying restricted source or documentation expression.

## Approved sources and prohibited sources

List exact URLs/commits, public standards, executable endpoints, allowed observations, and prohibited repositories/files for each role.

## Roles

Use `manifests/clean-room-role-matrix.template.csv`. Name individuals and alternates; record prior exposure.

## Information flow

- Observer produces dated requirements and test vectors.
- Reviewer removes source expression and approves the package.
- Implementer receives only the approved package.
- Validator reports behavioral discrepancies without donor snippets.
- Release engineer handles only approved source and evidence.

## Controls

Repository ACLs, separate accounts/channels, access logging, source-snippet scanning, code-similarity review, signed attestations, and exception escalation.

## Completion evidence

Approved requirements, access logs, independent design, commit history, test results, similarity review, and final human signoff.
