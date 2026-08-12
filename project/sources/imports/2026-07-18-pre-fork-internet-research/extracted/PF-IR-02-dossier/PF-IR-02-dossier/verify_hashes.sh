#!/bin/sh
set -eu
cd "$(dirname "$0")"
sha256sum -c manifests/files.sha256
