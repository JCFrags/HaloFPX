# Rollback and removal

## Core SDK native packages

For the gfx1151 meta package:

```bash
sudo apt autoremove amdrocm7.14-gfx1151
# or on RPM systems
sudo dnf remove amdrocm7.14-gfx1151
```

Then remove only the repository/key files created for the candidate and refresh package-manager metadata. Before removal, export the installed package list, package-file list, alternatives and resolved `/opt/rocm` symlinks.

## Raw tarball

Remove the selected extraction directory and delete the corresponding profile/environment block. Do not use a broad wildcard that can delete a legacy control installation. AMD's example assumes a `therock-tarball` directory:

```bash
sudo rm -rf therock-tarball
sudo rm -f /etc/profile.d/set-rocm-env.sh   # only when this file belongs to this install
```

## Python/pip lane

Purge the dedicated virtual environment, not the system interpreter:

```bash
python -m pip cache purge
rm -rf .venv
```

## Runfile

```bash
bash rocm-installer-7.14.0-6.run uninstall-rocm gfx=gfx1151
```

Driver removal is a separate operation and normally should not be used for the documented Ryzen inbox-driver lane. When a runfile-installed driver was deliberately used for a non-Ryzen test, preserve its installer and manifest before `uninstall-amdgpu`.

## Rollback acceptance

After rollback, verify that no 7.14 package, library, alternative, profile fragment, CMake package or cache remains in the 7.2 control namespace. `scripts/rollback-7.14.sh` is dry-run by default and requires an explicit `--execute` flag.
