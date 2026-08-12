# Current authoritative-status notes

**Checked:** 2026-07-18

These notes distinguish final/current authoritative sources from visible drafts
or pre-releases. They are source-status observations, not compliance claims.

## NIST GCM

NIST SP 800-38D (November 2007) remained the final GCM/GMAC recommendation. The
official NIST block-cipher publications page listed **SP 800-38D Rev. 1 — Second
Pre-Draft Call for Comments**, dated 2026-06-01. That pre-draft status is not
used as a final normative baseline in this bundle.

Official records:

* https://csrc.nist.gov/pubs/sp/800/38/d/final
* https://csrc.nist.gov/Projects/block-cipher-techniques/publications

## NIST HMAC

FIPS 198-1 remained listed as final. NIST's planning note proposed withdrawing
it when a final SP 800-224 is issued. SP 800-224 was still listed as an initial
public draft; therefore the final FIPS 198-1 text and RFC 2104 are retained, and
no inference is made that the future SP is final.

Official records:

* https://csrc.nist.gov/pubs/fips/198-1/final
* https://csrc.nist.gov/pubs/sp/800/224/ipd
* https://csrc.nist.gov/Projects/message-authentication-codes/publications

## cryptsetup

The official release archive listed cryptsetup **2.8.6** dated 2026-04-02 as the
selected stable release and also listed **2.8.7-rc2** dated 2026-07-10. The
pre-release was not selected. The recorded upstream SHA-256 for the 2.8.6
archive was not independently verified in this run; selected tagged manual
sources are locally hashed.

Official record:

* https://www.kernel.org/pub/linux/utils/cryptsetup/v2.8/

## systemd

The systemd release selected is **v261.1**, released 2026-06-26, tag commit
`eff9446d505d62c075bed37d606860b38cfe51fb`. `man/crypttab.xml` was inspected at
that tag; its Git blob SHA-1 is
`38ba4ceafbe80aead3025bc0866a3573a0a36ad5`. The raw XML transport limitation is
recorded separately.

Official/tagged records:

* https://github.com/systemd/systemd/releases/tag/v261.1
* https://github.com/systemd/systemd/blob/v261.1/man/crypttab.xml

## Linux kernel documentation

The accessed current documentation identified itself as **Linux 7.2.0-rc3**.
Raw documentation snapshots are pinned to source tag `v7.2-rc3`; this is
explicitly a release-candidate revision.
