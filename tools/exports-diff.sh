#!/bin/sh
# Guardrail for the stub split (docs/REFACTOR.md): the set of dynamic symbols the
# aggregate stub lib defines must not change across a build-graph refactor.
#
#   tools/exports-diff.sh capture <libstub.so>   # write baseline next to this script
#   tools/exports-diff.sh check   <libstub.so>   # diff against baseline; exit 1 on drift
set -eu
here=$(dirname "$0")
baseline="$here/exports-baseline.txt"
mode=${1:?usage: exports-diff.sh capture|check <libstub.so>}
lib=${2:?usage: exports-diff.sh capture|check <libstub.so>}

dump() { nm -D --defined-only "$1" | awk '{print $3}' | LC_ALL=C sort -u; }

case "$mode" in
  capture) dump "$lib" > "$baseline"; echo "captured $(wc -l < "$baseline") exports" ;;
  check)   dump "$lib" | diff -u "$baseline" - && echo "exports unchanged" ;;
  *) echo "unknown mode: $mode" >&2; exit 2 ;;
esac
