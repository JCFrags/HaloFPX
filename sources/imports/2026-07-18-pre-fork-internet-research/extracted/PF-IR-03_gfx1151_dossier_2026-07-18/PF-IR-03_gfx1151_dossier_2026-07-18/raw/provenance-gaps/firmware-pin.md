# [PROVENANCE_GAP] gfx1151 firmware pin

The current Ryzen matrix specifies the supported inbox kernel but does not pin a gfx1151-specific `linux-firmware` package version or individual AMD firmware blob digests. The local build manifest must capture the distribution package version/hash/signature, `WHENCE`, the filenames requested in `dmesg`, and SHA-256 hashes for those exact files.
