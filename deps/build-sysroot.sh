#!/usr/bin/env bash
#
# Build the GNUstep dependency sysroot for macholo and extract it from the
# container into deps/sysroot/.
#
#   ./build-sysroot.sh              build image (using cache) + extract
#   ./build-sysroot.sh --no-cache   force a full rebuild
#
# Produces:
#   deps/sysroot/usr/GNUstep/...    headers + libraries + gnustep-make
#   deps/sysroot/usr/local/...      anything that escaped the GNUstep layout
#   deps/sysroot/etc/...            GNUstep.conf + ld.so.conf.d snippet
#   deps/host-packages.txt          Fedora RPMs the host needs at runtime
#
set -euo pipefail

HERE="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
IMAGE="${IMAGE:-macholo-gnustep:f44}"
SYSROOT="$HERE/sysroot"
PKGLIST="$HERE/host-packages.txt"
PODMAN="${PODMAN:-podman}"

# ---------------------------------------------------------------- build ----
echo "==> podman build $IMAGE"
"$PODMAN" build "$@" -t "$IMAGE" -f "$HERE/Containerfile" "$HERE"

# -------------------------------------------------------------- extract ----
# Stream a tar out of a throwaway container rather than using `podman cp`, so
# the extracted tree is owned by the invoking user and the set of paths is
# explicit.  --no-same-owner keeps it that way when running as root.
echo "==> extracting sysroot into $SYSROOT"
rm -rf "$SYSROOT"
mkdir -p "$SYSROOT"

"$PODMAN" run --rm "$IMAGE" bash -c '
    set -eu
    paths="usr/GNUstep etc/GNUstep"
    [ -f /etc/ld.so.conf.d/gnustep-make.conf ] && paths="$paths etc/ld.so.conf.d/gnustep-make.conf" || :
    # Nothing should land in /usr/local (libobjc2 installs into the GNUstep
    # LOCAL domain because gnustep-config exists at configure time), but pick
    # it up if a future component ends up there.  Match on actual payload, not
    # just a non-empty directory - the Fedora base image already ships an empty
    # /usr/local/lib64/bpf that we do not want in the sysroot.
    for d in usr/local/lib usr/local/lib64; do
        [ -n "$(find "/$d" -maxdepth 1 -name "lib*.so*" 2>/dev/null)" ] && paths="$paths $d" || :
    done
    [ -n "$(find /usr/local/include -type f 2>/dev/null)" ] && paths="$paths usr/local/include" || :
    tar -C / -cf - $paths
' | tar -C "$SYSROOT" -xf - --no-same-owner

# --------------------------------------------------- host runtime deps -----
# Walk ldd of every shared object we built and map the resolved paths back to
# their owning RPM, so the host knows what to `dnf install` for the sysroot to
# load.  Packages provided by the sysroot itself are skipped (they are not
# RPM-owned anyway).
echo "==> deriving $PKGLIST"
"$PODMAN" run --rm "$IMAGE" bash -c '
    set -eu
    libs=$(find /usr/GNUstep /usr/local -type f \( -name "*.so" -o -name "*.so.*" \) 2>/dev/null)
    {
        for so in $libs; do
            ldd "$so" 2>/dev/null | awk "/=> \//{print \$3}"
        done
    } | sort -u | while read -r p; do
        case "$p" in
            /usr/GNUstep/*|/usr/local/*) continue ;;
        esac
        rpm -qf --queryformat "%{NAME}\n" "$p" 2>/dev/null || true
    done | sort -u
' > "$HERE/.pkgs.raw"

{
    echo "# Fedora packages required on the host for deps/sysroot to load."
    echo "# Derived from ldd of every .so under sysroot/usr/GNUstep (+ /usr/local),"
    echo "# mapped through rpm -qf inside the build container."
    echo "#"
    echo "#   sudo dnf install \$(grep -v '^#' host-packages.txt)"
    echo "#"
    echo "# Notes:"
    echo "#  * libdispatch (swift-corelibs-libdispatch) is both a build and a"
    echo "#    runtime dependency of libgnustep-base, which links it directly."
    echo "#  * libcxx/libcxxabi are here because libobjc was built with libc++,"
    echo "#    matching the -stdlib=libc++ the macholo executable uses."
    echo "#  * libstdc++ comes in via libicu, not via anything GNUstep built."
    echo "#"
    echo "# Compile-time only, so NOT listed below:"
    echo "#   clang lld cmake libcxx-devel libcxxabi-devel"
    echo
    cat "$HERE/.pkgs.raw"
} > "$PKGLIST"
rm -f "$HERE/.pkgs.raw"

# --------------------------------------------------------------- verify ----
echo "==> verifying extracted sysroot"
LIBDIR="$SYSROOT/usr/GNUstep/Local/Library/Libraries"
fail=0
check_sym() { # <lib> <symbol>...
    local lib="$1"; shift
    if [ ! -e "$LIBDIR/$lib" ]; then
        echo "  MISSING $lib"; fail=1; return
    fi
    # Dump once into a variable: piping straight into `grep -q` would SIGPIPE
    # nm on the first match and, under `set -o pipefail`, look like a failure.
    local syms
    syms="$(nm -D --defined-only "$LIBDIR/$lib")"
    for sym in "$@"; do
        if grep -qw -- "$sym" <<<"$syms"; then
            echo "  ok  $lib defines $sym"
        else
            echo "  ERR $lib does not define $sym"; fail=1
        fi
    done
}
check_sym libobjc.so             objc_msgSend objc_allocateClassPair
check_sym libgnustep-base.so     GSInitializeProcess
check_sym libgnustep-corebase.so CFArrayCreate CFStringCreateWithBytes

echo "==> done"
echo "    sysroot: $SYSROOT"
echo "    packages: $PKGLIST"
[ -f "$SYSROOT/usr/GNUstep/BUILD-INFO.txt" ] && cat "$SYSROOT/usr/GNUstep/BUILD-INFO.txt" || :
exit $fail
