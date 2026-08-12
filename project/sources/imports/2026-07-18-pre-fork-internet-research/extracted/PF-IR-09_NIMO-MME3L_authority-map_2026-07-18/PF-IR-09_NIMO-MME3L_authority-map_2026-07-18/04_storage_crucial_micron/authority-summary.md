# Crucial/Micron storage authority — P310 1 TB / VACR001

## Product mapping

The official P310 2230 flyer identifies 1 TB part `CT1000P310SSD2`. The measured drive is described as “Crucial P310 1 TB” with firmware `VACR001`, but the exact installed part number, form factor, namespace and serial were not supplied. Applicability is therefore `[FAMILY_APPLIES]`, not exact-part proof.

## Firmware availability

The official indexed P310 support result states no firmware updates are available at capture time. It does not enumerate `VACR001`, prior releases, security fixes, package signing, firmware-slot use, activation behavior, downgrade policy, or recovery.

**Classification:** `[NO_PUBLIC_PACKAGE] [OPEN] [SIGNATURE_UNPROVEN] [ROLLBACK_UNPROVEN]`.

## Telemetry

NVMe Identify Controller, Firmware Slot Information, SMART/health, and Error Information logs are the primary local authorities. Storage Executive is Windows-oriented; its indexed FAQ states SMART values are stored in the SSD controller. Public material does not prove which internal NAND/controller ECC events VACR001 exposes.

## Action boundary

Do not cross-flash a similar P310 image. Before any firmware action, retain full `nvme id-ctrl`, `fw-log`, `smart-log`, `error-log`, PCI path, model/serial, namespace, firmware activation support, backups and a written vendor applicability statement.
