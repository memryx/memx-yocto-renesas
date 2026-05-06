SUMMARY = "MemryX benchmark tool"
DESCRIPTION = "Benchmark utility for MemryX accelerator"
LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Proprietary;md5=0557f9d92cf58f2ccdd50f62f8ac0b28"

SRC_URI = " \
    file://CMakeLists.txt \
    file://memx_bench.cpp \
    file://dfp.cpp \
    file://mxpack.cpp \
    file://include \
"

S = "${WORKDIR}"

inherit cmake

DEPENDS += "memx-runtime"

FILES:${PN} += "${bindir}/memx_bench"
RDEPENDS:${PN} += "memx-runtime"