# HaloFPX Documentation and Organization Task

## Purpose

Make the HaloFPX internal documentation easy to read, easy to navigate, and
safe for workers to use.

Use a project Simplified Technical English standard.
Do not claim formal ASD-STE100 certification.

This task is a separate user-visible Codex task.
It must not perform implementation or production work.

## Required reading

Read these files before any edit:

1. `C:\Users\britt\Documents\Custom_Inference_Project\AGENTS.md`
2. `README.md`
3. `PROJECT_GOAL.md`
4. `wiki\HaloFPX_Wiki\README.md`
5. Every Wiki category manifest
6. `project-management\lead\OBJECTIVES.md`
7. `project-management\lead\MONITORING.md`
8. `project-management\lead\CURRENT_STATUS.md`
9. `project-management\lead\DECISIONS.md`
10. `references\agent-harness.md`
11. Relevant HaloFPX decisions and evidence indexes in
    `C:\Users\britt\Documents\HaloFPX\docs\halofpx\`

Read the complete Wiki inventory before changing its structure.

## Source authority

Use this precedence:

1. Exact current source and commit
2. Machine evidence and immutable receipts
3. Accepted project decisions
4. Official documentation and standards
5. Research papers
6. Clearly labeled inference or recommendation

Do not convert an inference into a fact.
Do not convert an open item into a completed item.
Do not change exact hashes, sizes, versions, measurements, service names, or
commit IDs.

## Protected material

Do not rewrite these items:

- raw evidence;
- machine receipts;
- benchmark logs;
- source archives;
- third-party source;
- license text;
- quoted source material;
- immutable decision history; or
- rejected-candidate evidence.

Create Simplified Technical English summaries and indexes for protected
material.
Keep the original protected files byte-identical.

## Project Simplified Technical English rules

Apply these rules to all editable internal guidance and Wiki prose.

### Sentence rules

- Use active voice.
- State the actor when the actor is important.
- Use one instruction or fact per sentence.
- Prefer sentences of 20 words or fewer.
- Split a sentence when it contains more than one main action.
- Use positive instructions when possible.
- Put the condition before the action.
- Use lists for three or more related items.
- Do not use rhetorical questions.
- Do not use jokes, idioms, metaphors, or informal filler.

### Word rules

- Use one approved term for one concept.
- Do not use different words for the same component.
- Define every abbreviation at first use.
- Do not invent an abbreviation for a term used fewer than three times.
- Use concrete nouns instead of vague pronouns.
- Avoid `it`, `this`, `that`, and `they` when the reference can be unclear.
- Avoid vague terms such as `recent`, `normal`, `fast`, `large`, and `safe`
  without a defined reference.
- Replace `latest` with an exact commit, version, or date.
- Replace `better` with a named metric and comparison.
- Replace `works` with the exact accepted behavior.
- Use `must` for requirements.
- Use `may` for permission.
- Use `can` for capability.
- Use `should` only for a recommendation.

### Technical claim rules

- Keep claim labels exact:
  `[MEASURED]`, `[VERIFIED]`, `[INFERENCE]`, `[ASSUMPTION]`,
  `[RECOMMENDATION]`, and `[OPEN]`.
- Put the environment next to each measured result.
- Put the source or evidence link next to each important claim.
- Separate prompt performance from generation performance.
- Separate synthetic results from full-model results.
- State feature-on and feature-off conditions.
- State rank ownership for distributed behavior.
- State failure, fallback, and recovery behavior.
- State that cache corruption causes a miss or recomputation.
- Never claim a speed improvement without a matched accepted comparison.

### Number and identifier rules

- Keep exact units.
- Use consistent byte and time formats.
- Do not round an exact artifact size.
- Keep full hashes in authority tables.
- Use short hashes only in narrative text when the full hash is linked.
- Use absolute dates for events.
- Name the host, model, commit, and binary when they affect a result.

## Information architecture

Create a clear entry path for users and workers.

The main navigation must contain:

1. **Start Here**
2. **Current Project State**
3. **Goals and Non-Negotiable Rules**
4. **System Architecture**
5. **Production Operations**
6. **Cache Design**
7. **Distributed Execution**
8. **Performance Results**
9. **Milestones and Decisions**
10. **Evidence Index**
11. **Worker Guide**
12. **Glossary**
13. **Archive**

Create or update these navigation artifacts:

- one root start page;
- one worker start page;
- one current-state page;
- one architecture overview;
- one evidence map;
- one decision map;
- one glossary;
- one archive index; and
- category manifests with consistent fields.

Each category manifest must state:

- purpose;
- authoritative files;
- current owner;
- status;
- last verified date;
- source commits;
- related decisions;
- related evidence;
- open work; and
- next safe action.

## Worker start requirements

Create `WORKER_START_HERE.md`.

The page must tell every worker to:

1. Read `AGENTS.md`.
2. Read the project goal.
3. Read current status and decisions.
4. Read the relevant category manifest.
5. Read linked accepted decisions.
6. Verify current source and evidence.
7. Check the exact production authority before an authorized transition.
8. Preserve unrelated worktree files.
9. Keep feature-off behavior unchanged.
10. Report exact commits, binaries, evidence, and cleanup.

Update `AGENTS.md` only when needed to require this worker start sequence.
Do not weaken any existing evidence or safety rule.

## Organization rules

- Preserve stable canonical paths when possible.
- Add redirects or index links before moving a widely referenced file.
- Do not delete duplicate or stale material without explicit authority.
- Move stale material to an archive only after link analysis.
- Record every move in an organization receipt.
- Keep raw evidence near its milestone.
- Keep summaries separate from raw evidence.
- Use consistent file names with lowercase words and hyphens unless an
  existing project convention requires another form.
- Do not create many shallow folders.
- Prefer one clear index over duplicate overview pages.
- Ensure that a worker can find required authority in three clicks or fewer
  from the root start page.

## Inventory phase

Before edits, create a machine-readable inventory with:

- absolute or repository-relative path;
- document type;
- category;
- claim labels;
- last modified commit;
- inbound links;
- outbound links;
- orphan status;
- duplicate topic;
- protected status;
- current authority status; and
- proposed action.

Classify each file as:

- authoritative;
- supporting;
- historical;
- raw evidence;
- generated;
- duplicate;
- stale;
- open research; or
- unknown.

Do not reorganize an `unknown` file until its role is resolved.

## Rewrite process

Use this order:

1. Inventory all documents.
2. Identify protected files.
3. Build the proposed navigation map.
4. Verify the map against current source and decisions.
5. Rewrite the root and worker entry pages.
6. Rewrite category manifests.
7. Rewrite active guidance pages.
8. Add STE summaries for protected evidence.
9. Repair links.
10. Archive only approved stale duplicates.
11. Run validation.
12. Obtain independent review.

Do not rewrite the full Wiki in one unreviewed bulk operation.
Use coherent category commits.

## Validation

Validation must include:

- all internal links resolve;
- no authoritative page is orphaned;
- no raw evidence changed;
- all exact hashes and measurements remain exact;
- claim labels remain valid;
- no `[OPEN]` item became accepted without a decision;
- no accepted result became stronger;
- every category has a manifest;
- every worker path begins at `WORKER_START_HERE.md`;
- glossary terms use one preferred name;
- duplicate navigation pages are removed or redirected safely;
- Git diff has no unrelated file;
- JSON and machine-readable indexes parse; and
- Wiki validation tools pass.

Use a focused readability check:

- sentence length;
- passive voice candidates;
- ambiguous pronouns;
- undefined abbreviations;
- inconsistent terms;
- vague comparison words; and
- missing evidence links.

Treat automated language checks as warnings.
Apply technical judgment before each rewrite.

## Independent review

Use an independent reviewer after each major category.

The reviewer must check:

- technical meaning;
- source fidelity;
- claim strength;
- provenance;
- navigation;
- STE style;
- protected evidence;
- worker usability; and
- unrelated changes.

Correct clear mechanical findings in the same task.
Stop and ask the Project Lead when a correction changes technical meaning.

## Repository ownership

The documentation task owns:

- root navigation pages;
- Wiki prose and manifests;
- worker guidance;
- glossary;
- evidence and decision indexes;
- archive indexes;
- documentation validation scripts; and
- organization receipts.

The task does not own:

- implementation source;
- raw evidence contents;
- lead status and decision records unless the Project Lead assigns them;
- production configuration;
- model files;
- third-party source; or
- licenses.

Stage only documentation files owned by this task.
Preserve unrelated research, evidence, and user files.

## Reporting

Report material boundaries to Project Lead task
`019fa661-0b7d-7a63-8fb4-07658f368f55`.

Each report must include:

- exact repository HEAD;
- changed categories;
- inventory counts;
- protected files checked;
- link results;
- validation results;
- independent review;
- moves and archive receipts;
- remaining categories; and
- next safe action.
