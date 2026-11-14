# Build and research notes

These notes expose the development inputs. A complete clean-room rebuild of the final device installation has **not** been validated. There is no automatic root filesystem installer or ready-made boot image.

## Prerequisites and upstream versions

Use a Linux build host (the original development used Ubuntu 24.04 under WSL), Python 3, Git, an AArch64 cross compiler, GNU make, `dtc`, and the dependencies required by each upstream project. UEFI additionally used a Python environment, EDK II/Mu build tooling and the upstream device-binary/ACPI submodules.

Exact source and submodule revisions are in [`sources.json`](../sources.json). Obtain the upstream source trees separately and check out those exact commits. Kernel/UEFI sources and toolchains can require several gigabytes; the helper does not download them.

Do not publish your extracted firmware, full live device tree, host keys, authorized keys, or partition backups. Obtain required device firmware from your own device/stock package under its applicable terms.

## Prepare source checkouts

From this repository on Linux, using **separate clean checkouts**:

```sh
python3 tools/prepare-sources.py --uefi /path/to/Mu-Silicium --kernel /path/to/linux
python3 tools/prepare-sources.py --uefi /path/to/Mu-Silicium --kernel /path/to/linux --apply
```

The first command only checks pinned commits and patch applicability. The second explicitly modifies those source trees and copies the UEFI overlay. Neither command builds or flashes anything. Submodules must also match the recorded revisions; inspect them rather than updating to arbitrary latest versions.

## Kernel and device trees

Use `kernel/config/mars-bringup.config` as the recorded configuration, not a guarantee that every full-build artifact has been tested. A conventional starting point is:

```sh
mkdir -p /path/to/kernel-build
cp kernel/config/mars-bringup.config /path/to/kernel-build/.config
make -C /path/to/linux O=/path/to/kernel-build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
make -C /path/to/linux O=/path/to/kernel-build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j4 Image modules
mkdir -p out
dtc -I dts -O dtb -o out/mars-usb.dtb kernel/devicetree/mars-native-display-test.dts
dtc -I dts -O dtb -o out/mars-lan.dtb kernel/devicetree/mars-native-display-ethernet-test.dts
```

These trees are already preprocessed and do not need local header includes. `mars-usb` is fixed peripheral with no ADSP/PMIC node; `mars-lan` is fixed host with ADSP/PMIC. They are deliberately separate. Their reserved-memory layout must be reviewed for another firmware/device.

The recorded configuration includes components that were built/loaded as additional modules during bring-up. Review module dependencies, `depmod`, and userspace startup ordering; producing an `Image` alone is insufficient. Required pieces include UFS PHY/controller, pinctrl, SMMU, PMIC GLINK, ADSP/QRTR/PDR, display/DSI and REFGEN support. Do not extract a module archive over the root `/lib` symlink; use a narrowly scoped path under `/usr/lib/modules/<release>/`.

## Panel driver

The tested panel is `qcom,mdss_dsi_k2_38_08_0a_mp_dsc_cmd`, using DSC 1.1, 10 bits/component, 8 compressed bits/pixel and two 720×50 slices. Confirm your actual panel first.

Extract the **selected panel-only DTSI** from your own stock device tree. The generator expects a decoded DTSI with its 60/90/120 Hz timing nodes:

```sh
python3 tools/generate-panel-commands.py /path/to/selected-panel.dtsi --output /path/to/generated-panel
cp /path/to/generated-panel/mars-panel-commands.h kernel/panel/
make -C /path/to/linux O=/path/to/kernel-build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M="$PWD/kernel/panel" modules
```

The generated command header/PPS data are intentionally not distributed. The generator validates packet boundaries and DSC dimensions; this does not validate hardware compatibility. At 60 Hz the tested data had 73 on and 11 off commands. See the kernel patch for DSC input-depth and packetization fixes; both were necessary on the test phone.

## UEFI and handoff

Follow the pinned upstream UEFI build instructions after applying the overlay. The Mars target is selected with `python build_uefi.py -d mars` in the prepared UEFI tree once its dependencies are ready. The initial port still references `venus` binary/ACPI inputs; audit these dependencies and their licensing separately.

The custom boot probe expects a FAT payload containing:

```text
EFI/MARSDIAG/Image
EFI/MARSDIAG/mars-efi-minimal.dtb
EFI/MARSDIAG/mars-diagnostic.cpio.gz
```

`Resources/DTBs/mars.dtb` is an additional local UEFI packaging input in the original build and is not bundled. It must be prepared/audited separately; do not substitute a different phone's live DTB. The final FAT/Android boot-image packaging pipeline and all its firmware inputs are not yet made reproducible in this publication. Do not interpret the source-preparation check as a successful firmware rebuild.

## Userspace and initramfs

`userspace/mars-screen-key.c` can be compiled with:

```sh
aarch64-linux-gnu-gcc -static -Os -Wall -Wextra -Werror userspace/mars-screen-key.c -o out/mars-screen-key
```

The daemon uses exclusive input access and framebuffer blanking. It needs the native `msmdrmfb` display, QCOM PON input and coordinated logind handling. It does not implement full system suspend.

`reference/initramfs/init.c` is the original development init reference. It expects additional binaries/modules, a marked root filesystem and the test device's partition numbering. Do not deploy it unchanged to another phone. No authorized keys, Dropbear host keys, filesystem images or original root provisioning/formatting scripts are included. Supply your own SSH identity and verify partition identities before any storage operation.
