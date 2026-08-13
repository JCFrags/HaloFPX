# GitHub Wiki convenience-mirror runbook

Status: **private, generated, non-authoritative publication design**. The
GitHub Wiki has not been enabled or published by this change.

## Authority and exact initial scope

The sole canonical source for content in this generated mirror is
[`project/wiki/HaloFPX_Wiki`](../../project/wiki/HaloFPX_Wiki). Project Lead
status and decision authority remains
[`project/project-management/lead/`](../../project/project-management/lead/).
A GitHub Wiki copy is navigation and recovery convenience only. Do not edit it
in the web UI or use it as evidence that differs from the canonical tree.

The initial registry was frozen from repository commit
`9bfccf25d43af0c446df591035e9cdac0b74d6c0`, canonical Wiki tree
`09c24fd4b5411c0e132f728cacd889eee1103858`. That tree contains 642 regular Git
blobs and 2,262,094 Git-blob bytes:

- 534 Markdown pages;
- 108 byte-preserved support assets: 100 YAML, six JSON, and two Python files;
- 12 categories and 86 numbered sections; and
- no image, media, symlink, executable-mode, or binary asset.

Generation adds `_Sidebar.md`, `_Footer.md`, and `mirror-manifest.json`, so the
closed initial output contains exactly 645 files. `mirror-manifest.json` binds
the other 644 outputs and deliberately does not hash itself.

Do not add other repository material to this mirror. Cross-tree evidence links
point to immutable source-commit URLs; they do not duplicate `project/sources`,
root documentation, credentials, models, build products, or release assets.

## Deterministic mapping and transformation

[`github-wiki-page-map.json`](../../project/project-management/documentation/github-wiki-page-map.json)
is a frozen source-path registry. It prevents duplicate `README.md` and standard
section-page titles from colliding in GitHub's flat page namespace:

| Canonical path kind | Mirror destination |
|---|---|
| root `README.md` | `Home.md` |
| other root Markdown | `R-<root-slug>.md` |
| category `README.md` | `C<category>-<category-slug>.md` |
| section `README.md` | `S<section>-<section-slug>.md` |
| section leaf page | `S<section>-<section-slug>--<leaf-slug>.md` |
| non-Markdown support file | `assets/<full-canonical-relative-path>` |

Existing destinations remain stable when a title changes. A source addition,
deletion, move, or case-only collision stops generation until a reviewed map
change accounts for it.

[`github_wiki_mirror.py`](../../project/project-management/documentation/github_wiki_mirror.py)
parses Markdown with the CommonMark `markdown-it-py` stack, compares semantic
link multisets before and after its source-preserving rewrite, and stops on an
unmapped, missing, root-relative, protocol-relative, unsafe-scheme, ambiguous,
or broken-anchor link. It does not modify fenced/indented code, inline code, or
ordinary prose. It rewrites:

- canonical Markdown targets to their frozen Wiki page routes;
- canonical assets and in-repository cross-tree targets to immutable
  `blob/<source-commit>` or `tree/<source-commit>` URLs; and
- three applied legacy S23 fragment occurrences to their explicitly audited
  full anchors. The manifest names each repair; an unlisted mismatch fails
  closed.

The admitted mutable link grammar is deliberately narrower than all valid
CommonMark. Current one-line reference definitions and ordinary inline links
are supported; multiline/container definitions and raw HTML `href`/`src`
attributes fail closed rather than risk a silent partial rewrite. Semantically
unchanged destinations retain their original escaped/entity source bytes.

Strict leading YAML front matter is omitted from the rendered page body so
GitHub does not display project-governance metadata as prose. Its original bytes
remain bound by the source Git-blob/SHA-256 identity and by per-page
`source_front_matter_size` and `source_front_matter_sha256` manifest fields.
Unterminated or non-blank-separated front matter fails closed. Canonical YAML
schema/content validity remains the responsibility of the source Wiki
validator, which runs before mirror generation; this tool checks delimiter
shape and cryptographic identity, not YAML semantics. LF and CRLF delimiters are
admitted; a UTF-8 BOM is rejected rather than rendered ambiguously.

Every transformed source page and the generated footer say that the mirror is
generated, private-project material, non-authoritative, and not a blanket-MIT
work. They link to the exact canonical source commit/tree and
[`LICENSES_AND_PROVENANCE.md`](../../LICENSES_AND_PROVENANCE.md).

## Local bootstrap, generation, and validation

Requirements are exact universal wheel artifacts, not moving package names.
The lock records upstream provenance, MIT notices, versions, and SHA-256 values.
Internet access is needed only if those two wheels are not already in a local
package cache. Generation and validation read local Git objects and installed
packages only. Before parsing, the tool checks the installed distributions'
versions, every immutable SHA-256/size entry in their wheel `RECORD` metadata,
the audited complete record-set digests, and the imported module locations. The
manifest records both the wheel hashes and those installed record-set digests.

From the repository root:

```powershell
python -m pip install --disable-pip-version-check --require-hashes -r project/project-management/documentation/github-wiki-mirror-requirements.txt
python project/project-management/documentation/github_wiki_mirror.py --repo . verify --source-ref HEAD
python -m unittest project/project-management/documentation/test_github_wiki_mirror.py -v
```

Generate only to a new path outside the canonical Wiki and every Git
administration directory. Symlink, junction, or other reparse-point ancestors
are rejected, and the output parent must already exist. Generation builds and
validates a private randomly named sibling, then atomically publishes it without
replacing any destination. A stopped run leaves its private sibling for
inspection and deliberate cleanup rather than deleting paths that another
process may have replaced:

```powershell
$sourceCommit = git rev-parse HEAD
$stage = Join-Path $env:TEMP ("halofpx-wiki-" + [guid]::NewGuid().ToString("N"))
python project/project-management/documentation/github_wiki_mirror.py --repo . generate --source-ref $sourceCommit --output $stage
python project/project-management/documentation/github_wiki_mirror.py --repo . validate --source-ref $sourceCommit --output $stage
python project/project-management/documentation/github_wiki_mirror.py --repo . audit-manifest --output $stage
```

Record the JSON summary, including source commit, Wiki tree, source/output
counts, and manifest SHA-256. `verify` regenerates twice in memory and requires
byte equality. `validate` reconstructs the expected mirror offline and rejects
missing, extra, or changed bytes in an existing staging directory.

Pull-request CI performs these checks with repository contents read-only,
checkout credentials disabled, and the hash-locked parser stack. CI has no Wiki
write credential, does not call GitHub's Wiki/settings API, does not upload the
generated tree, and does not publish.

## One-time private bootstrap gate

An administrator must confirm all of the following outside CI:

1. `JCFrags/HaloFPX` remains private and the account plan supports a Wiki for a
   private repository.
2. The exact proposed tree has passed the repository's license/provenance and
   sensitivity review. The Wiki must remain private until the public-release
   gate explicitly includes its exact tree.
3. The administrator enables the Wiki and creates only the required initial
   `Home` page so `https://github.com/JCFrags/HaloFPX.wiki.git` exists.
4. A fresh clone contains exactly that bootstrap `Home.md`; any other file is
   unexplained state and stops bootstrap.
5. A separately scoped credential has been proven to clone and normally push
   this private Wiki repository. Do not assume that a workflow token or a
   fine-grained token supports it until a bounded dry run proves the capability.

This repository does not enable the setting, create the first page, store a
credential, or push the Wiki. Never place a credential in a remote URL, command
argument, generated tree, manifest, receipt, log, or artifact.

## Manual fast-forward publication

Use this sequence only after the bootstrap gate or for a subsequent clean
publication. It is a controlled Git operation against the separate Wiki
repository, never a two-way sync.

1. Require a clean main-repository checkout at the exact intended source commit
   and run the local validation sequence above.
2. Obtain the current Wiki remote tip with authenticated `git ls-remote` and
   record its full 40-character commit. Discover the default branch; do not
   assume `master` or `main`.
3. Clone the Wiki to a new, explicitly named temporary directory. Require its
   checked-out tip to equal the recorded remote tip. Stop on mismatch.
4. Before changing the clone, create `git bundle create <backup>.bundle --all`,
   compute its SHA-256, test it with `git bundle verify`, and copy the bundle and
   checksum to protected private storage. Keep it outside the Git repository.
5. For later publication, use `git archive` at the exact cloned Wiki tip to
   extract its tracked tree into a new plain temporary directory. Before the
   archive, inspect `git ls-tree -r -z <expected-tip>` and require that every
   entry is a `100644 blob` and that its path set equals the manifest-declared
   file set plus `mirror-manifest.json`; reject symlinks, executable modes,
   Gitlinks/submodules, trees outside that closed set, and any other mode/type.
   Then run
   `audit-manifest --output <archive-directory>`. Do not point the tool at a
   working clone because `.git` metadata is deliberately treated as an extra.
   Require the manifest SHA-256 to equal the prior tip's recorded commit trailer
   and immutable receipt. The audit validates the historical tree using its own
   manifest and therefore does not depend on the current generator or page map,
   but a self-consistent rewritten tree plus manifest is not external authority.
   The manifest must have the exact canonical v1 shape and encoding. An
   unsupported or malformed schema, missing/extra/manual path, symlink,
   junction/reparse point, case collision, byte/hash/size mismatch, aggregate
   mismatch, or receipt/trailer mismatch is drift and stops publication. For
   first bootstrap, require exactly `Home.md` instead.
6. After the exact clone path and tip are revalidated, remove the clone's
   tracked paths through Git so history preserves them, then copy only the
   already validated 645-file staging tree. Do not copy `.git`, operate on an
   unresolved path, use a filesystem-wide recursive force-delete, or run the
   generator over an existing directory.
7. Run `validate` against the separate plain staging directory, then compare
   that exact staged file/path/hash inventory to the clone's Git index. Do not
   run `validate` against the clone because `.git` is an intentional extra.
   Inspect `git status`, and require that every added/changed/deleted path is
   explained by the generated set.
   Configure an explicit human publisher identity and create one commit with:

   ```text
   HaloFPX-Source-Commit: <40-character source commit>
   HaloFPX-Canonical-Wiki-Tree: <40-character canonical Wiki tree>
   HaloFPX-Manifest-SHA256: <64-character manifest SHA-256>
   HaloFPX-Prior-Wiki-Tip: <40-character prior Wiki commit>
   ```

8. Re-read the remote tip and require it still equals the prior tip. Push the
   one new commit normally to the discovered branch. Never use `--force`,
   `--mirror`, history replacement, or a retry that skips this compare-and-swap
   check.
9. Fresh-clone the Wiki after push. Require the advertised tip, local commit,
   645-file set, every generated byte, manifest hash, link closure, and source
   commit/tree to match the staged publication.
10. Record an immutable verification receipt under `docs/publication/` in a
    separate main-repository pull request. Include old/new Wiki tips, Wiki tree,
    source commit/tree, map/generator/parser identities, manifest/aggregate and
    backup-bundle hashes, command versions, private-visibility check, validation
    results, publisher, date, and any limitation.

A normal non-fast-forward rejection means another actor changed the Wiki. Fetch,
preserve, inspect, and restart the sequence from a new expected tip. Do not
override it.

## Drift and recovery

Direct Wiki web edits are not merged back automatically. If an edit is wanted,
apply and review it in the canonical in-repository Wiki, regenerate, and publish
a new forward commit. Otherwise restore the generated bytes with a new forward
commit. Do not erase the drift commit.

To restore a prior known-good mirror, first prove that the candidate Wiki commit
is an ancestor of the current tip and that its exact tree/manifest validates.
Copy that tree onto the current clean clone, create a new child commit with the
same trailers plus a recovery reason and source authority, recheck the remote
tip, and normally push. Never reset or force-push.

If the Wiki repository becomes absent, the canonical Wiki and frozen tooling
remain sufficient recovery authority: re-enable the private Wiki, create its
single bootstrap page, and repeat the one-time gate. Wiki Git history is useful
backup evidence, not required project authority. A bundle can restore review
history into a disposable clone, but it must not replace current canonical
content without the same validation and forward-publication process.

## Nonclaims and stopping conditions

The mirror proves deterministic transformation and publication identity only.
It does not prove the accuracy of every canonical claim, grant distribution
rights, make a performance claim, qualify target hardware, or make the GitHub
Wiki an operational dependency.

The exact GitHub/Gollum rendering of generated page routes, fragments, and
nested `assets/...` paths remains **[OPEN]** until the first private publication
performs a bounded live page smoke check and records it in the immutable
publication receipt. Closed CommonMark and hash checks do not substitute for
that renderer-specific observation.

Content-only canonical Wiki changes intentionally require reviewing and
updating the audited Wiki-tree, source-byte, and link-census expectations in
`test_github_wiki_mirror.py`. A path addition, move, or deletion requires
generating a candidate map at a new path, reviewing its complete diff, and then
intentionally replacing the registry; never silently overwrite the frozen map.
Never reuse a retired mirror destination for a different source. Preserve it as
a reviewed redirect/tombstone or reserve it permanently in the next map schema,
so an old external Wiki URL cannot silently acquire unrelated content.

Stop before publication for any failed source validator, dirty canonical tree,
map drift, dependency mismatch, broken/ambiguous link, output mismatch, missing
receipt field, public visibility, unknown license/sensitivity status, absent
backup, unexplained Wiki path, remote-tip change, or unavailable private-Wiki
entitlement/credential capability.
