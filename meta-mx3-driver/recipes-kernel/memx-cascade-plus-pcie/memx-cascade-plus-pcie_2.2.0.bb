SUMMARY = "MemryX MX3 PCIe"
DESCRIPTION = "Kernel module for the MemryX MX3 (PCIe)"
HOMEPAGE = "http://developer.memryx.com"

LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Proprietary;md5=0557f9d92cf58f2ccdd50f62f8ac0b28"

INSANE_SKIP:${PN} += "buildpaths"
INSANE_SKIP:${PN}-dbg += "buildpaths"

SRC_URI = "git://git@github.com/memryx/mx3_driver_pub.git;protocol=ssh;branch=release \
           file://mx_set_powermode \
          "
SRCREV = "b1ed73725a555d0aaf477042ab0f66f0099efd25"

S = "${WORKDIR}/git/kdriver/linux/pcie"

inherit module

EXTRA_OEMAKE += "KERNEL_SRC=${STAGING_KERNEL_DIR} EXTRA_CFLAGS='-I${WORKDIR}/git/kdriver/include'"

KERNEL_MODULE_AUTOLOAD += "memx_cascade_plus_pcie"

do_configure:prepend() {
    # Add KERNEL_SRC variable after CONFIG_MODULE_SIG line
    sed -i '/^CONFIG_MODULE_SIG=n/a KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build' ${S}/Makefile
    
    # Replace hardcoded kernel path with KERNEL_SRC variable
    sed -i 's|/lib/modules/\$(shell uname -r)/build|\$(KERNEL_SRC)|g' ${S}/Makefile
    
    # Remove EXTRA_CFLAGS from driver target (keep it only in debug/android)
    sed -i '/^driver:/,/^$/ s/EXTRA_CFLAGS="\$(INCLUDES)" //' ${S}/Makefile
    
    # Add modules_install target after android target
    sed -i '/^android:/,/^$/ {
        /modules$/ a\
\nmodules_install:\n\tmake -C $(KERNEL_SRC) M=$(PWD) modules_install
    }' ${S}/Makefile
}

do_install:append() {
    install -d ${D}${nonarch_base_libdir}/firmware
    install -m 0644 ${WORKDIR}/git/firmware/cascade.bin ${D}${nonarch_base_libdir}/firmware/
    install -m 0644 ${WORKDIR}/git/firmware/cascade_4chips_flash.bin ${D}${nonarch_base_libdir}/firmware/
    install -m 0644 ${WORKDIR}/git/firmware/cascade_mini_bar_flash.bin ${D}${nonarch_base_libdir}/firmware/

    # Create and install power.conf
    install -d ${D}${sysconfdir}/memryx
    cat > ${D}${sysconfdir}/memryx/power.conf << 'EOF'
# WARNING: manually editing this file is an extremely bad idea. Please don't.
FREQ4C=500
VOLT4C=690
FREQ2C=700
VOLT2C=725
EOF
    chmod 0644 ${D}${sysconfdir}/memryx/power.conf

    # Install mx_set_powermode script
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/mx_set_powermode ${D}${bindir}/mx_set_powermode

    # Install PCIe flash update tool
    install -m 0755 ${WORKDIR}/git/tools/flash_update_tool/bin/aarch64/pcieupdateflash ${D}${bindir}/mxfw_pcie_update_flash
}

FILES:${PN} += " \
    ${nonarch_base_libdir}/firmware/ \
    ${sysconfdir}/memryx/power.conf \
    ${bindir}/mx_set_powermode \
    ${bindir}/mxfw_pcie_update_flash \
"