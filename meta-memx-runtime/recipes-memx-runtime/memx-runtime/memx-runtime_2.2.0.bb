SUMMARY = "MemryX Runtime"
DESCRIPTION = "MemryX Core Runtime (libmemx) and C++ MxAccl Runtime API"
LICENSE = "MPL-2.0"
LIC_FILES_CHKSUM = "file://LICENSE-MPL-2.0;md5=e879eaad64aab8e1f8e63e298f5cea28"
INSANE_SKIP:${PN} += "already-stripped dev-elf installed-vs-shipped"
INSANE_SKIP:${PN}-dev += "already-stripped dev-elf"
INSANE_SKIP:${PN}-dbg += "already-stripped dev-elf"

DEPENDS += "util-linux-libuuid"

SRC_URI = "gitsm://github.com/memryx/MxAccl;protocol=https;branch=release"
SRCREV = "3e6afe50815fa1ac23bf9ed2ff06b6d7d16e2398"

S = "${WORKDIR}/git"

# Inherit the CMake class to use CMake for building.
inherit cmake
inherit systemd

# use bitbake's provided CFLAGS instead of our optimized (and potentially wrong) AVX2, etc., flags
EXTRA_OECMAKE += "-DCMAKE_BUILD_TYPE=Packaging -DCMAKE_X86_FLAGS_BASE=0 -DCMAKE_X86_FLAGS_AVX2=0 -DCMAKE_AARCH64_FLAGS=0 -DCMAKE_RISCV_FLAGS=0 "

do_configure:prepend() {
    echo "Patching CMakeLists.txt to comment out -march flags on lines 72 and 76..."
    sed -i '72s/ -march=/" # -march=/' ${S}/CMakeLists.txt
    sed -i '76s/ -march=/" # -march=/' ${S}/CMakeLists.txt
    
    echo "Copying libmemx header file to source..."
    install -d ${STAGING_INCDIR}/memx/
    install -m 0644 ${S}/misc/libmemx/memx.h ${STAGING_INCDIR}/memx/

    echo "Copying libmemx for this architecture to source..."
    install -d ${STAGING_LIBDIR}/
    install -m 0644 ${S}/misc/libmemx/${TARGET_ARCH}/libmemx.so ${STAGING_LIBDIR}/
}

do_install:append() {

    echo "Copying libmemx header file to final install..."
    install -d ${D}${includedir}/memx/
    install -m 0644 ${S}/misc/libmemx/memx.h ${D}${includedir}/memx/

    echo "Copying libmemx for this architecture to final install..."
    install -d ${D}${libdir}/
    install -m 0755 ${S}/misc/libmemx/${TARGET_ARCH}/libmemx.so ${D}${libdir}/

    # Optional: if anything expects libmemx.so.2, provide a symlink
    ln -sf libmemx.so ${D}${libdir}/libmemx.so.2

    # Install mxa_manager binary
    install -d ${D}${bindir}
    install -m 0755 ${B}/mxa_manager/mxa_manager ${D}${bindir}/

    # Install config
    install -d ${D}${sysconfdir}/memryx
    install -m 0644 ${S}/mxa_manager/mxa_manager.conf ${D}${sysconfdir}/memryx/

    # Install systemd unit
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/debian_manager/mxa-manager.service ${D}${systemd_system_unitdir}/
    
    # Patch systemd service file to change User from mxa-manager to nobody
    sed -i 's/User=mxa-manager/User=nobody/' ${D}${systemd_system_unitdir}/mxa-manager.service
}

SYSTEMD_SERVICE:${PN} = "mxa-manager.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"


FILES:${PN} = "${bindir}/acclBench"
FILES:${PN} += "${bindir}/mxa_manager"
FILES:${PN} += "${sysconfdir}/memryx/mxa_manager.conf"
FILES:${PN} += "${systemd_system_unitdir}/mxa-manager.service"
FILES:${PN} += "${libdir}/libmemx.so"
FILES:${PN} += "${libdir}/libmx_accl.so.2"
FILES:${PN}-dev += "${libdir}/libmx_accl.so"
FILES:${PN}-dev = "${includedir}/memx"
FILES:${PN}-dev += "${includedir}/memx/memx.h"
FILES:${PN}-dev += "${includedir}/memx/accl"
FILES:${PN}-dev += "${includedir}/memx/accl/client.h"
FILES:${PN}-dev += "${includedir}/memx/accl/DeviceManager.h"
FILES:${PN}-dev += "${includedir}/memx/accl/dfp.h"
FILES:${PN}-dev += "${includedir}/memx/accl/DFPRunner.h"
FILES:${PN}-dev += "${includedir}/memx/accl/messages.h"
FILES:${PN}-dev += "${includedir}/memx/accl/MxAcclBase.h"
FILES:${PN}-dev += "${includedir}/memx/accl/MxAccl.h"
FILES:${PN}-dev += "${includedir}/memx/accl/MxAcclMT.h"
FILES:${PN}-dev += "${includedir}/memx/accl/prepost.h"
FILES:${PN}-dev += "${includedir}/memx/accl/MxModel.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/blocky_queue.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/cpu_opts.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/errors.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/featureMap.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/gbf.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/id_tracker.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/locked_var.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/macros.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/mxpack.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/mxTypes.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/path.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/sha512.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/threadsafe_map.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/uint128.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/auto_clocker.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/comm_sockets.h"
FILES:${PN}-dev += "${includedir}/memx/accl/utils/rental_store.h"