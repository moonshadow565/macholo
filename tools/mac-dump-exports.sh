#!/bin/sh
# Run on a real Mac. Dumps the export list of the frameworks/dylibs macholo cares
# about, per architecture, into a tarball to scp back to the VM. Uses dyld_info
# (macOS 12+, works against the dyld shared cache — most of these no longer exist
# as files on disk, so nm/otool on the paths would fail).
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

DYLIBS="libSystem.B libobjc.A libc++.1 libc++abi libresolv.9 libiconv.2 libicucore.A"

# The libSystem umbrella re-exports these, so the flat libSystem.B list already
# contains their symbols — but the per-sublibrary split (which symbol lives where)
# is exactly what maps onto macholo's libsystem/ module files. Cache-resident:
# enumerate by name, tolerate misses across OS versions.
SYSLIBS="libsystem_asl libsystem_blocks libsystem_c libsystem_collections
libsystem_configuration libsystem_containermanager libsystem_coreservices
libsystem_darwin libsystem_dnssd libsystem_featureflags libsystem_info
libsystem_kernel libsystem_m libsystem_malloc libsystem_networkextension
libsystem_notify libsystem_platform libsystem_pthread libsystem_sandbox
libsystem_secinit libsystem_symptoms libsystem_trace libcache libcommonCrypto
libcompiler_rt libcopyfile libcorecrypto libdispatch libdyld libkeymgr libmacho
libquarantine libremovefile libunwind libxpc"

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

dump_objc() { # $1=path $2=basename — classes/protocols/categories metadata
    for arch in arm64 x86_64; do
        if dyld_info -arch "$arch" -objc "$1" > "$out/$2.objc.txt" 2>/dev/null \
           && [ -s "$out/$2.objc.txt" ]; then
            echo "  $2 [objc/$arch]: $(wc -l < "$out/$2.objc.txt") lines"
            return 0
        fi
        rm -f "$out/$2.objc.txt"
    done
}

for f in $FRAMEWORKS; do
    p="/System/Library/Frameworks/$f.framework/$f"
    dump "$p" "$f"
    dump_objc "$p" "$f"
done
for d in $DYLIBS; do
    dump "/usr/lib/$d.dylib" "$d"
done
for s in $SYSLIBS; do
    dump "/usr/lib/system/$s.dylib" "$s"
done
dump_objc /usr/lib/libobjc.A.dylib libobjc.A

tar czf "$out.tgz" "$out"
echo "wrote $out.tgz — scp it to the VM (any path, e.g. ~/) and tell the session"
