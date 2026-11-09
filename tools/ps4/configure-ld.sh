#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 RafaelNGP
# Linker for third-party configure scripts (FFmpeg) on PS4: links test
# programs as a PIE against the OpenOrbis toolchain stub libraries.
: "${OO_PS4_TOOLCHAIN:?}"
set -- "$@"
out=a.out
objs=
while [ $# -gt 0 ]; do
    case $1 in
        -o) out=$2; shift ;;
        -o*) out=${1#-o} ;;
        -l*|-L*|-Wl,*|-pthread|-m*|-f*|-O*|-g*|-std=*|-isysroot|-isystem|-target|--target=*|-D*|-I*|-W*)
            case $1 in -isysroot|-isystem|-target) shift ;; esac ;;
        *.o|*.a) objs="$objs $1" ;;
    esac
    shift
done
# Prefer the versioned linker installed by the workflow, then fall back to ld.lld.
ld=${LD_LLD:-$(command -v ld.lld-18 || command -v ld.lld)}
[ -x "$ld" ] || { echo "configure-ld: ld.lld not found" >&2; exit 1; }
# shellcheck disable=SC2086
exec "$ld" -m elf_x86_64 -pie --script "$OO_PS4_TOOLCHAIN/link.x" -o "$out" $objs -L"$OO_PS4_TOOLCHAIN/lib" -lc -lkernel "$OO_PS4_TOOLCHAIN/lib/crt1.o"
