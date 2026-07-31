#!/bin/sh
# Run on a real Mac. Dumps the export list of the frameworks/dylibs macholo cares
# about, per architecture, into a tarball to scp back to the VM. Uses dyld_info
# (macOS 12+, works against the dyld shared cache — the dylibs no longer exist as
# files on disk, so nm/otool on the paths would fail).
#
#   sh mac-dump-exports.sh
#   scp macos-exports-*.tgz <vm>:
set -u

out="macos-exports-$(sw_vers -productVersion)-$(uname -m)"
mkdir -p "$out"
{ sw_vers; uname -mv; date -u +%Y-%m-%dT%H:%M:%SZ; } > "$out/MANIFEST.txt"

FRAMEWORKS="CoreFoundation Foundation AppKit CoreGraphics ApplicationServices
CoreServices Carbon Security AudioToolbox CoreAudio IOKit ScriptingBridge
Metal QuartzCore OpenGL CoreText CoreVideo"
DYLIBS="/usr/lib/libSystem.B.dylib /usr/lib/libobjc.A.dylib /usr/lib/libc++.1.dylib"

dump() { # $1=path $2=basename
    for arch in arm64 x86_64; do
        if dyld_info -arch "$arch" -exports "$1" > "$out/$2.$arch.txt" 2>/dev/null \
           && [ -s "$out/$2.$arch.txt" ]; then
            echo "  $2 [$arch]: $(wc -l < "$out/$2.$arch.txt") lines"
        else
            rm -f "$out/$2.$arch.txt"
        fi
    done
}

for f in $FRAMEWORKS; do
    dump "/System/Library/Frameworks/$f.framework/$f" "$f"
done
for d in $DYLIBS; do
    dump "$d" "$(basename "$d" .dylib)"
done

# objc class/protocol surface of the big three (what the metadata converter and a
# future mini-CF would bridge against); best-effort, flag exists on newer dyld_info.
for f in CoreFoundation Foundation AppKit; do
    dyld_info -arch arm64 -objc "/System/Library/Frameworks/$f.framework/$f" \
        > "$out/$f.objc.txt" 2>/dev/null && [ -s "$out/$f.objc.txt" ] \
        || rm -f "$out/$f.objc.txt"
done

tar czf "$out.tgz" "$out"
echo "wrote $out.tgz — scp it to the VM (any path, e.g. ~/) and tell the session"
