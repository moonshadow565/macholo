#!/bin/sh
# Track 3 (docs/REFACTOR.md "Track 3"): inventory-only audit of the libsystem
# demand surface. No classification judgment, no resolver changes here — this
# just regenerates the ground-truth table from scratch every run:
#
#   1. Demand: every symbol the game imports from libSystem/libresolv, across
#      both the 16.3 and 16.15 surfaces (docs/surface-16.*/lib{System,resolv}.txt).
#   2. Apple attribution: which real Apple libsystem_* (or friend) dylib
#      exports that symbol, from the dyld_info dumps in
#      ~/projects/osx-libs/macos-exports-26.6-arm64/.
#   3. Current resolution: how macholo's resolver actually satisfies that
#      symbol today, checked in a fixed priority order (shim > stub > override
#      > blocked > glibc > sysroot > host > UNRESOLVED) that mirrors
#      lib::resolve() in src/lib.cpp.
#
# Output: docs/surface-audit/libsystem.tsv (symbol, weak, demanded_by,
# apple_sublib, resolution) and docs/surface-audit/SUMMARY.md (bucket counts).
#
# Usage: tools/surface-audit.sh   (no args; run from anywhere, paths are
#   resolved relative to this script / the fixed external input locations)
set -eu

here=$(cd -- "$(dirname -- "$0")" && pwd)
root=$(cd -- "$here/.." && pwd)
apple_dir="/home/user/projects/osx-libs/macos-exports-26.6-arm64"

out_dir="$root/docs/surface-audit"
tsv="$out_dir/libsystem.tsv"
summary="$out_dir/SUMMARY.md"

mkdir -p "$out_dir"

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# ---------------------------------------------------------------- 0. build ---
# The shim lane ($darwin$<name>) and the stub lane (<name>) both come from
# libstub.so — build it fresh in this worktree before inspecting its exports.
CC=clang CXX=clang++ OBJC=clang ASM=clang cmake -B "$root/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DGNUSTEP_DIR="$root/deps/sysroot/usr/GNUstep" >&2
ninja -C "$root/build" >&2

# ------------------------------------------------------ 1. demand surface ---
# One row per unique base symbol (weak marker stripped). weak=y only if every
# occurrence of that symbol across all four demand files was marked weak;
# demanded_by records which of the two game versions' demand files (libSystem
# + libresolv) mention it.
awk '
  {
    line = $0
    weak = 0
    if (line ~ / \(weak\)$/) { weak = 1; sub(/ \(weak\)$/, "", line) }
    sym = line
    if (sym == "") next
    if (!(sym in seen)) { seen[sym] = 1; allweak[sym] = 1 }
    if (!weak) allweak[sym] = 0
    if (FILENAME ~ /surface-16\.3\//) in163[sym] = 1
    if (FILENAME ~ /surface-16\.15\//) in1615[sym] = 1
  }
  END {
    for (sym in seen) {
      w = allweak[sym] ? "y" : "n"
      if (sym in in163 && sym in in1615) db = "both"
      else if (sym in in163) db = "16.3"
      else db = "16.15"
      print sym "\t" w "\t" db
    }
  }
' "$root/docs/surface-16.3/libSystem.txt" "$root/docs/surface-16.3/libresolv.txt" \
  "$root/docs/surface-16.15/libSystem.txt" "$root/docs/surface-16.15/libresolv.txt" \
  > "$tmp/demand.tsv"

# ------------------------------------------------------ 2. apple attribution ---
# dyld_info -exports dump format: header lines ("/usr/lib/.../foo.dylib
# [arch]:", "-exports:", "offset      symbol") are ignored because they don't
# match either data pattern below. Data lines are either
# "0x<hex>  <symbol> [optional annotation]" or
# "[re-export] <symbol> (<real-symbol> from <lib>)" — in both cases the
# symbol we care about is the token right after the offset/tag.
# First file a symbol appears in wins (fixed priority order below); this
# matters for a handful of symbols cross-referenced by [re-export].
apple_files=""
for f in "$apple_dir"/libsystem_*.x86_64.txt; do
  apple_files="$apple_files $f"
done
for n in cache commonCrypto compiler_rt copyfile corecrypto dispatch dyld keymgr \
         macho quarantine removefile unwind xpc; do
  apple_files="$apple_files $apple_dir/lib$n.x86_64.txt"
done
apple_files="$apple_files $apple_dir/libresolv.9.x86_64.txt"

# shellcheck disable=SC2086 (intentional word-splitting: list of file paths)
awk '
  FNR == 1 {
    n = split(FILENAME, parts, "/")
    sublib = parts[n]
    sub(/\.x86_64\.txt$/, "", sublib)
  }
  $1 ~ /^0x[0-9A-Fa-f]+$/ && NF >= 2 {
    sym = $2
    if (!(sym in seen)) { seen[sym] = 1; print sym "\t" sublib }
    next
  }
  $1 == "[re-export]" && NF >= 2 {
    sym = $2
    if (!(sym in seen)) { seen[sym] = 1; print sym "\t" sublib }
  }
' $apple_files > "$tmp/apple.tsv"

# --------------------------------------------------- 3a/3b. shim + stub ---
nm -D --defined-only "$root/build/stub/libstub.so" | awk '
  $3 ~ /^\$darwin\$/ { s = $3; sub(/^\$darwin\$/, "", s); print s; next }
  NF >= 3 { print $3 > "'"$tmp"'/stub.txt" }
' > "$tmp/shim.txt"
# (the awk above writes shim hits to stdout and stub hits to stub.txt in one pass)

# ------------------------------------------------------- 3c/3d. lib.cpp ---
grep -oE 'OVERRIDE\("[^"]+"' "$root/src/lib.cpp" | sed -E 's/^OVERRIDE\("//; s/"$//' > "$tmp/override.txt"
awk '/BLOCKED\[\] = \{/,/\};/' "$root/src/lib.cpp" \
  | grep -oE '"[^"]*"' | sed -E 's/^"//; s/"$//' > "$tmp/blocked.txt"

# ------------------------------------------------------------- 3e. glibc ---
{ nm -D --defined-only /lib64/libc.so.6; nm -D --defined-only /lib64/libm.so.6; } | awk '
  NF >= 3 { sym = $3; sub(/@.*/, "", sym); if (sym != "") print sym }
' | sort -u > "$tmp/glibc.txt"

# ----------------------------------------------------------- 3f. sysroot ---
: > "$tmp/sysroot_all.tsv"
find -L "$root/deps/sysroot" -name '*.so' | sort | while IFS= read -r so; do
  base=$(basename "$so")
  nm -D --defined-only "$so" 2>/dev/null | awk -v lib="$base" 'NF >= 3 { print $3 "\t" lib }'
done >> "$tmp/sysroot_all.tsv"
awk -F'\t' '!seen[$1]++' "$tmp/sysroot_all.tsv" > "$tmp/sysroot.tsv"

# --------------------------------------------------------------- 3g. host ---
: > "$tmp/host_all.tsv"
for hf in /usr/lib64/libGL.so.1 /usr/lib64/libasound.so.2 /usr/lib64/libSDL2-2.0.so.0; do
  [ -e "$hf" ] || continue
  base=$(basename "$hf")
  nm -D --defined-only "$hf" 2>/dev/null | awk -v lib="$base" 'NF >= 3 { print $3 "\t" lib }'
done >> "$tmp/host_all.tsv"
awk -F'\t' '!seen[$1]++' "$tmp/host_all.tsv" > "$tmp/host.tsv"

# ------------------------------------------------------------ 4. resolve ---
# resolve_import() (src/lib.cpp) strips exactly one leading '_' off the raw
# Darwin import name before lib::resolve() ever looks at it — every lookup
# below (shim/stub/override/blocked/glibc/sysroot/host) matches on that
# stripped name. Apple attribution, by contrast, matches the *raw* demand
# symbol directly against the dyld export dumps (Mach-O export names already
# carry the same leading underscore).
awk -F'\t' \
  -v shimf="$tmp/shim.txt" -v stubf="$tmp/stub.txt" -v overridef="$tmp/override.txt" \
  -v blockedf="$tmp/blocked.txt" -v glibcf="$tmp/glibc.txt" \
  -v sysrootf="$tmp/sysroot.tsv" -v hostf="$tmp/host.tsv" -v applef="$tmp/apple.tsv" '
  BEGIN {
    while ((getline line < applef) > 0) { split(line, a, "\t"); apple[a[1]] = a[2] }
    close(applef)
    while ((getline line < shimf) > 0) shim[line] = 1
    close(shimf)
    while ((getline line < stubf) > 0) stub[line] = 1
    close(stubf)
    while ((getline line < overridef) > 0) ov[line] = 1
    close(overridef)
    nblocked = 0
    while ((getline line < blockedf) > 0) { nblocked++; blocked[nblocked] = line }
    close(blockedf)
    while ((getline line < glibcf) > 0) glibc[line] = 1
    close(glibcf)
    while ((getline line < sysrootf) > 0) { split(line, a, "\t"); sysroot[a[1]] = a[2] }
    close(sysrootf)
    while ((getline line < hostf) > 0) { split(line, a, "\t"); host[a[1]] = a[2] }
    close(hostf)
  }
  {
    raw = $1; weak = $2; db = $3
    stripped = raw
    if (substr(raw, 1, 1) == "_") stripped = substr(raw, 2)

    res = ""
    if (stripped in shim) res = "shim"
    else if (stripped in stub) res = "stub"
    else if (stripped in ov) res = "override"
    else {
      for (i = 1; i <= nblocked && res == ""; i++)
        if (index(stripped, blocked[i]) == 1) res = "blocked"
    }
    if (res == "" && (stripped in glibc)) res = "glibc"
    if (res == "" && (stripped in sysroot)) res = "sysroot:" sysroot[stripped]
    if (res == "" && (stripped in host)) res = "host:" host[stripped]
    if (res == "") res = "UNRESOLVED"

    sub_ = (raw in apple) ? apple[raw] : "none"
    print sub_ "\t" raw "\t" weak "\t" db "\t" res
  }
' "$tmp/demand.tsv" | LC_ALL=C sort -t "$(printf '\t')" -k1,1 -k2,2 \
  | awk -F'\t' -v OFS='\t' '{print $2, $3, $4, $1, $5}' > "$tsv"

# --------------------------------------------------------------- summary ---
{
  total=$(wc -l < "$tsv")
  printf '# libsystem surface audit — summary\n\n'
  printf 'Regenerated by `tools/surface-audit.sh`. %s demanded symbols total (union of ' "$total"
  printf '16.3 + 16.15 libSystem/libresolv demand lists).\n\n'

  printf '## Resolution buckets\n\n'
  printf '| resolution | count |\n|---|---|\n'
  awk -F'\t' '{
    r = $5
    sub(/:.*/, "", r)
    c[r]++
  } END { for (r in c) print r "\t" c[r] }' "$tsv" | LC_ALL=C sort -t "$(printf '\t')" -k2,2rn \
    | awk -F'\t' '{ printf "| %s | %s |\n", $1, $2 }'

  printf '\n## Apple sublib x resolution\n\n'
  awk -F'\t' '{
    r = $5
    sub(/:.*/, "", r)
    key = $4 "\t" r
    c[key]++
    subs[$4] = 1
    reses[r] = 1
  }
  END {
    # collect resolution bucket names in a stable order, but only the ones
    # that actually occur anywhere (keeps the table compact) — used[] is the
    # single source of truth for BOTH the header and every data row so the
    # column count always matches.
    order = "shim stub override blocked glibc sysroot host UNRESOLVED"
    n = split(order, ord, " ")
    nu = 0
    for (i = 1; i <= n; i++) if (ord[i] in reses) used[++nu] = ord[i]
    printf "| apple_sublib"
    for (i = 1; i <= nu; i++) printf " | %s", used[i]
    printf " |\n|---"
    for (i = 1; i <= nu; i++) printf "|---"
    printf "|\n"
    for (s in subs) subslist[++ns] = s
    # simple sort of subslist
    for (i = 1; i <= ns; i++)
      for (j = i + 1; j <= ns; j++)
        if (subslist[j] < subslist[i]) { t = subslist[i]; subslist[i] = subslist[j]; subslist[j] = t }
    for (i = 1; i <= ns; i++) {
      s = subslist[i]
      printf "| %s", s
      for (k = 1; k <= nu; k++) {
        key = s "\t" used[k]
        printf " | %s", (key in c ? c[key] : 0)
      }
      printf " |\n"
    }
  }' "$tsv"

  printf '\n## UNRESOLVED symbols\n\n'
  printf 'Non-weak UNRESOLVED symbols in a fall-through-to-glibc resolver should be rare for a\n'
  printf 'binary that reaches GAMELOOP; treat a long non-weak list here as a bug (attribution\n'
  printf 'order / underscore-stripping) rather than a real gap.\n\n'
  printf '| symbol | weak |\n|---|---|\n'
  awk -F'\t' '$5 == "UNRESOLVED" { printf "| %s | %s |\n", $1, $2 }' "$tsv"
} > "$summary"

echo "wrote $tsv" >&2
echo "wrote $summary" >&2
