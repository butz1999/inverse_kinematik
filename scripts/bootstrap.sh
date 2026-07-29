#!/usr/bin/env bash

set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
submodule_path="$repository_root/third_party/bluepad32"

if [[ ! -d "$submodule_path/.git" && ! -f "$submodule_path/.git" ]]; then
  git -C "$repository_root" submodule update --init third_party/bluepad32
fi

shopt -s nullglob
for patch_file in "$repository_root"/patches/bluepad32/*.patch; do
  if git -C "$submodule_path" apply --reverse --check "$patch_file"; then
    continue
  fi
  git -C "$submodule_path" apply --check "$patch_file"
  git -C "$submodule_path" apply "$patch_file"
done
