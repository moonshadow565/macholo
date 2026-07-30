# Import surface — League 16.15 x86_64 slice

Machine-generated from the game binary; one file per source dylib, one symbol per line
(`(weak)` marks weak imports). `all-imports.tsv` is the full `dylib<TAB>symbol` table
(libc++ excluded from per-dylib files — self-solved by linking host libc++).

These files are the manual prototype of the planned `macholo --audit` output: the
implemented-stub surface should be diffed against lists like these per target binary.

Regenerate:

```sh
BIN="path/to/LeagueofLegends" OUT=docs/surface-16.15
{ llvm-objdump --macho --arch x86_64 --bind "$BIN"
  llvm-objdump --macho --arch x86_64 --lazy-bind "$BIN"; } |
  awk '$NF=="(weak_import)" {print $(NF-2)"\t"$(NF-1)" (weak)"; next}
       NF>4 && $NF ~ /^_/   {print $(NF-1)"\t"$NF}' | sort -u > $OUT/all-imports.tsv
cut -f1 $OUT/all-imports.tsv | sort | uniq -c | sort -rn > $OUT/counts-by-dylib.txt
```

Caveat: static imports only. ObjC method usage (selectors, protocol methods — e.g. the
whole Metal device/encoder API) flows through msgSend and does not appear here; that
surface needs runtime dispatch logging.
