# MemryX Yocto Recipes for Renesas RZ/G3E

Yocto integration layers for MemryX SDK on Renesas RZ/G3E platform.

## meta-mx3-driver

Builds MX3 M.2 PCIe kernel module.

1. Add `meta-mx3-driver` path to `conf/bblayers.conf`
2. Include `memx-cascade-plus-pcie` target in build

**NOTE**: ARM/RISCV systems may require device tree modifications for BAR/MSIX configuration. Contact MemryX for platform-specific support.

## meta-memx-runtime

Builds [MxAccl C++ Runtime Library](https://developer.memryx.com/api/accelerator/cpp.html) and [libmemx](https://developer.memryx.com/api/driver/driver.html) binary.

1. Add `meta-memx-runtime` path to `conf/bblayers.conf`
2. Include `memx-runtime` and `memx-bench` targets in build

## Build Instructions

Reference: [Renesas RZ/G3E Linux Start Guide v1.0.0](https://www.renesas.com/en/document/gde/rzg3e-linux-start-guide-rev100)

Download BSP v1.0.0 (`RTK0EF0045Z0040AZJ-v1.0.0.zip`): https://www.renesas.com/en/software-tool/rzg3e-board-support-package#download

Target: `core-image-weston` with graphics and video codec support.

### 1. Clone Repository

```bash 
git clone https://github.com/memryx/memx-yocto-renesas.git
```

### 2. Configure Build Environment

Install host dependencies:

```bash
sudo apt-get update 
sudo apt install build-essential chrpath cpio debianutils diffstat file gawk \
gcc git iputils-ping libacl1 liblz4-tool locales python3 python3-git \
python3-jinja2 python3-pexpect python3-pip python3-subunit socat texinfo unzip \
wget xz-utils zstd
```

Configure git (required for patch application):

```bash
git config --global user.email "you@example.com"
git config --global user.name "Your Name"
```

Initialize BSP (requires 200GB free space):

```bash 
PACKAGE_VERSION=1.0.0
mkdir ~/rzg3e_bsp_v${PACKAGE_VERSION}
cd ~/rzg3e_bsp_v${PACKAGE_VERSION}
cp ../Downloads/RTK0EF0045Z0040AZJ-v${PACKAGE_VERSION}.zip .
unzip ./RTK0EF0045Z0040AZJ-v${PACKAGE_VERSION}.zip
tar zxvf ./RTK0EF0045Z0040AZJ-v${PACKAGE_VERSION}/rzg3e_bsp_v${PACKAGE_VERSION}.\
tar.gz
```

### 3. Add Graphics and Video Codec Support (Recommended)

Download both packages from https://www.renesas.com/en/software-tool/rzg3e-board-support-package#overview (MyRenesas account required).

Extract graphics layer:

```bash
cp ~/Downloads/RTK0EF0045Z14001ZJ-v4.2.0.2_rzg_EN.zip .
unzip ./RTK0EF0045Z14001ZJ-v4.2.0.2_rzg_EN.zip
tar zxvf ./RTK0EF0045Z14001ZJ-v4.2.0.2_rzg_EN/meta-rz-features_graphics_v4.2.0.2.tar.gz
```

Extract codec layer:

```bash 
cp ~/Downloads/RTK0EF0207Z00001ZJ-v4.4.0.0_rzg3e_EN.zip .
unzip ./RTK0EF0207Z00001ZJ-v4.4.0.0_rzg3e_EN.zip
tar zxvf ./RTK0EF0207Z00001ZJ-v4.4.0.0_rzg3e_EN/meta-rz-features_codec_v4.4.0.0.tar.gz
```

Initialize build environment:

```bash
TEMPLATECONF=$PWD/meta-renesas/meta-rz-distro/conf/templates/rz-conf/ source \
poky/oe-init-build-env build
```

### 4. Add MemryX and Feature Layers

```bash
bitbake-layers add-layer /path/to/memx-yocto-renesas/meta-memx-runtime
bitbake-layers add-layer /path/to/memx-yocto-renesas/meta-mx3-driver
bitbake-layers add-layer ../meta-rz-features/meta-rz-graphics
bitbake-layers add-layer ../meta-rz-features/meta-rz-codecs
```

### 4. Add MemryX and Feature Layers

From build directory (`~/rzg3e_bsp_v1.0.0/build/`), add layers to `bblayers.conf`:

```bash
bitbake-layers add-layer /path/to/memx-yocto-renesas/meta-memx-runtime
bitbake-layers add-layer /path/to/memx-yocto-renesas/meta-mx3-driver
bitbake-layers add-layer ../meta-rz-features/meta-rz-graphics
bitbake-layers add-layer ../meta-rz-features/meta-rz-codecs
```

Add recipe targets to `local.conf`:

```bash 
echo 'IMAGE_INSTALL:append = " memx-cascade-plus-pcie"' >> conf/local.conf
echo 'IMAGE_INSTALL:append = " memx-runtime"' >> conf/local.conf
echo 'IMAGE_INSTALL:append = " memx-bench"' >> conf/local.conf
```

Add GStreamer and OpenCV for video inference rendering:

```bash
echo 'IMAGE_INSTALL:append = " opencv gstreamer1.0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-omx gstreamer1.0-plugin-vspmfilter"' >> conf/local.conf
echo 'IMAGE_INSTALL:append = " gstreamer1.0-omx gstreamer1.0-plugin-vspmfilter"' >> conf/local.conf
```

Add utilities for `mx_set_powermode`:

```bash
echo 'IMAGE_INSTALL:append = " dialog"' >> conf/local.conf
echo 'IMAGE_INSTALL:append = " sudo"' >> conf/local.conf
```

Apply PCIe Legacy INTA interrupt patch:

```bash
cp ~/memx-yocto-renesas/pcie_legacy_fix.patch ~/rzg3e_bsp_v1.0.0/meta-renesas/meta-rz-bsp/recipes-kernel/linux/files/
sed -i '/file:\/\/0005-gpu-drm-bridge-Support-S2R-ITE-it6263.patch \\/a\	file://pcie_legacy_fix.patch \\' ~/rzg3e_bsp_v1.0.0/meta-renesas/meta-rz-bsp/recipes-kernel/linux/linux-renesas_6.1.inc
```

### 5. Build Image

Execute build (~1 hour depending on host specs):

```bash
MACHINE=smarc-rzg3e bitbake core-image-weston
```

### 6. Flash microSD Card

Install bmap-tools:

```bash 
sudo apt install bmap-tools
```

Identify microSD device and unmount partitions:

```bash
sudo fdisk -l
umount /dev/sda2  # Example: adjust device path as needed
```

Write image to microSD:

```bash
cd tmp/deploy/images/smarc-rzg3e/
sudo bmaptool copy --bmap <wic_image>.wic.bmap <wic _image>.wic.gz /dev/sda
```

### 7. Flash VMX-004 M.2 Module

Download firmware `cascade_4chips_flash.bin` from https://github.com/memryx/mx3_driver_pub/tree/sdk2p2/firmware

Flash module:

```bash
sudo mxfw_pcie_update_flash -f ~/Downloads/cascade_4chips_flash.bin
```

Verify SDK 2.2 firmware: `FW_CommitID=0x196bb59f`

Boot system with module and SD card installed. Login: `root`

## Cross-Compilation SDK Setup

### 1. Build SDK

From sourced build environment (`~/rzg3e_bsp_v1.0.0/build`):

```bash
MACHINE=smarc-rzg3e bitbake core-image-weston -c populate_sdk
```

### 2. Install SDK

Execute SDK installer:

```bash
cd tmp/deploy/sdk
sudo sh rz-vlp-glibc-x86_64-core-image-weston-cortexa55-smarc-rzg3e-toolchain-5.0.8.sh
```

Confirm default installation path or specify alternative. Accept installation prompt.

### 3. Source SDK Environment

Source environment for each new shell session:

```bash 
source /opt/rz-vlp/5.0.8/environment-setup-cortexa55-poky-linux
```

### 4. Clone Sample Application

Clone [sample repository](https://github.com/memryx/memx_test_app) with SDK environment sourced:

```bash
cd ~
git clone https://github.com/memryx/memx_test_app.git
```

### 5. Cross-Compile Application

Build executable:

```bash
cd memx_test_app
mkdir build && cd build
cmake ..
make
```

Output: `main` executable in `build/` directory.

### 6. Deploy Executable

Mount microSD ext3 partition (adjust device path):

```bash
sudo mount /dev/sda2 /media/
cd /media/usr/bin
sudo cp ~/sample_app/build/main .
sudo chmod +x main
```

Reinstall microSD on SOM and boot.

### 7. Execute Application

Download and extract DFP:

```bash
wget developer.memryx.com/model_explorer/2p2/YOLO26_nano_640_640_3_onnx.zip
unzip YOLO_v8_small_640_640_3_onnx.zip
```

Run inference application (USB camera at `/dev/video0`):

```bash
/usr/bin/main --video_paths cam:0 -d YOLO26_nano_640_640_3_onnx.dfp --show
```

Omit `--show` to disable rendering. Exit: `CTRL+C`