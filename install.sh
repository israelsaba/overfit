#!/bin/sh
set -eu

script_dir=$(dirname -- "$0")
root=$(cd "$script_dir" && pwd)
make -C "$root" install
printf '%s\n' "Installed overfit to ${HOME}/.local/bin/overfit"
