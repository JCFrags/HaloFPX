# Capture limitations

1. The target distro was not specified. The bundle therefore distinguishes:
   - the **AMD-qualified reference**: Ubuntu 24.04 LTS, kernel >= 6.10, Ryzen AI Software 1.7.1;
   - the **actual target**: unknown until the read-only probe is run.
2. Firmware binaries are not included. Their filenames, symlinks, upstream commit, WHENCE blob, and license pointer are preserved.
3. AMD account-gated DEB/TGZ payloads are not included. Exact package filenames are preserved; package hashes remain `[MISSING]` until locally downloaded.
4. AMD EULA and third-party-license PDFs are not included or interpreted.
5. Public XRT/compiler repository HEADs are evidence of open components, not proof of the source revision used to build AMD's gated 1.7.1 binaries.
6. Large kernel files are represented by exact pinned full small files, an exact full architecture document, exact source excerpts, blob identifiers, and an immutable fetch script.
7. No local Ryzen AI MAX+ 395 machine was probed and no performance result is asserted.
