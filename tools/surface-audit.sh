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
#      > blocked > lane4 > UNRESOLVED) that mirrors lib::resolve() in
#      src/lib.cpp. "lane4" here models the *real* resolver lane 4 --
#      dlsym(nullptr, name) == RTLD_DEFAULT -- as one load-order-priority pass
#      over the DT_NEEDED closure of build/macholo (`ldd`), not a fixed
#      libc/libm/GL/asound/SDL2 guess; the resolution string keeps the actual
#      providing library and whether it's under deps/sysroot, e.g.
#      "glibc:libc.so.6" / "glibc:libdispatch.so" / "sysroot:libobjc.so.4.6".
#   4. Track 3 judgment: cross-references docs/surface-audit/libsystem-verdicts.tsv
#      and fails the run if any glibc-lane symbol has no verdict row (drift
#      check), then regenerates src/forward_allowlist.inc from the FORWARD rows.
#
# Output: docs/surface-audit/libsystem.tsv (symbol, weak, demanded_by,
# apple_sublib, resolution), docs/surface-audit/SUMMARY.md (bucket counts),
# and src/forward_allowlist.inc (generated, see lib::resolve() in src/lib.cpp).
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

# ------------------------------------------------------- 3e/3f. lane4 scope ---
# Model resolver lane 4 (`::dlsym(nullptr, name)` == RTLD_DEFAULT) as the real
# DT_NEEDED closure of the built loader binary, not a hand-picked libc/libm/
# GL/asound/SDL2 guess -- that guess is what produced the phantom UNRESOLVED
# rows in VERDICTS.md §3 (libdispatch.so, libgcc_s.so.1 were never in it).
#
# One priority-ordered pass, in `ldd`'s own (load-order-approximating) list
# order, first-definition-wins -- this must be a SINGLE pass rather than
# "check the sysroot dir, then check everything else" because deps/sysroot
# libs (libobjc, libgnustep-base, ...) are early *direct* dependencies of
# build/macholo while some real host libs they still pull in (e.g. libobjc
# -> libdispatch.so -> libBlocksRuntime.so, which re-exports libobjc's own
# _Block_object_assign/_NSConcreteStackBlock/etc under the same names) only
# get satisfied transitively, later. Splitting the scan into two independent
# first-wins passes would let the later host copy win over the earlier
# trusted one for any name both happen to export -- wrong, and it would
# falsely flag already-trusted GNUstep block-runtime symbols as new,
# unaudited glibc-lane fall-throughs. libstub.so is excluded (the shim/stub
# lanes above already own its exports and run before lane 4 is ever reached).
: > "$tmp/lane4_all.tsv"
ldd "$root/build/macholo" | awk '{ for (i = 1; i <= NF; i++) if ($i ~ /^\//) { print $i; break } }' \
  | grep -v '/ld-linux' \
  | grep -vE '/libstub\.so$' \
  | awk '!seen[$0]++' \
  | while IFS= read -r so; do
      [ -e "$so" ] || continue
      case "$so" in
        "$root/deps/sysroot/"*) tag="sysroot" ;;
        *)                      tag="glibc" ;;
      esac
      base=$(basename "$so")
      nm -D --defined-only "$so" 2>/dev/null | awk -v lib="$base" -v tag="$tag" \
        'NF >= 3 { sym = $3; sub(/@.*/, "", sym); if (sym != "") print sym "\t" tag ":" lib }'
    done >> "$tmp/lane4_all.tsv"
awk -F'\t' '!seen[$1]++' "$tmp/lane4_all.tsv" > "$tmp/lane4.tsv"

# ------------------------------------------------------------ 4. resolve ---
# resolve_import() (src/lib.cpp) strips exactly one leading '_' off the raw
# Darwin import name before lib::resolve() ever looks at it — every lookup
# below (shim/stub/override/blocked/lane4) matches on that stripped name.
# Apple attribution, by contrast, matches the *raw* demand symbol directly
# against the dyld export dumps (Mach-O export names already carry the same
# leading underscore).
awk -F'\t' \
  -v shimf="$tmp/shim.txt" -v stubf="$tmp/stub.txt" -v overridef="$tmp/override.txt" \
  -v blockedf="$tmp/blocked.txt" -v lane4f="$tmp/lane4.tsv" -v applef="$tmp/apple.tsv" '
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
    while ((getline line < lane4f) > 0) { split(line, a, "\t"); lane4[a[1]] = a[2] }
    close(lane4f)
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
    if (res == "" && (stripped in lane4)) res = lane4[stripped]
    if (res == "") res = "UNRESOLVED"

    sub_ = (raw in apple) ? apple[raw] : "none"
    print sub_ "\t" raw "\t" weak "\t" db "\t" res
  }
' "$tmp/demand.tsv" | LC_ALL=C sort -t "$(printf '\t')" -k1,1 -k2,2 \
  | awk -F'\t' -v OFS='\t' '{print $2, $3, $4, $1, $5}' > "$tsv"

# ---------------------------------------------------------- 4b. drift check ---
# Every symbol this audit resolves through the glibc lane (the audited-risk
# domain per VERDICTS.md §7.2) must already have a judgment row in
# libsystem-verdicts.tsv. A newly demanded glibc fall-through with no verdict
# row means a new game version added surface nobody has judged yet -- fail
# loudly here instead of letting it silently forward at runtime.
verdicts="$out_dir/libsystem-verdicts.tsv"
if [ -f "$verdicts" ]; then
  awk -F'\t' '$5 ~ /^glibc:/ { print $1 }' "$tsv" | LC_ALL=C sort -u > "$tmp/glibc_syms.txt"
  awk -F'\t' 'NR > 1 { print $1 }' "$verdicts" | LC_ALL=C sort -u > "$tmp/verdict_syms.txt"
  missing=$(LC_ALL=C comm -23 "$tmp/glibc_syms.txt" "$tmp/verdict_syms.txt")
  if [ -n "$missing" ]; then
    printf 'surface-audit: DRIFT -- symbols resolved via the glibc lane with no row in %s:\n' "$verdicts" >&2
    printf '%s\n' "$missing" >&2
    exit 1
  fi
else
  echo "surface-audit: WARNING -- $verdicts not found, skipping drift check" >&2
fi

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
    order = "shim stub override blocked glibc sysroot UNRESOLVED"
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

# ---------------------------------------------------- 5. forward allowlist ---
# Generated from the Track 3 verdicts table: lane 4 (dlsym(nullptr, name)) may
# only answer for symbols judged ABI-identical on Darwin-x86_64 vs glibc-x86_64.
inc="$root/src/forward_allowlist.inc"
{
  printf '// GENERATED by tools/surface-audit.sh from docs/surface-audit/libsystem-verdicts.tsv\n'
  printf '// Do not edit by hand: change the verdict in the TSV and re-run the audit.\n'
  awk -F'\t' 'NR > 1 && $3 == "FORWARD" {
        sym = $1
        sub(/^_/, "", sym)          # resolve_import() strips exactly one leading underscore
        print sym
      }' "$verdicts" | LC_ALL=C sort -u | sed -E 's/.*/FORWARD("&")/'
} > "$inc"

echo "wrote $tsv" >&2
echo "wrote $summary" >&2
echo "wrote $inc" >&2
