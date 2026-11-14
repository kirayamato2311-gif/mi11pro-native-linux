# Xiaomi Mi 11 Pro — Native UEFI & Linux

[Bahasa Indonesia](README.id.md) · **English**

An experimental native UEFI/Linux bring-up for **Xiaomi Mi 11 Pro (`mars`, Snapdragon 888 / SM8350)**, based on [Project Silicium](https://github.com/Project-Silicium/Mu-Silicium) and the [SM8350 mainline Linux tree](https://gitlab.com/sm8350-mainline/linux).

Linux boots on the phone itself, without Android, Termux, a container, or a virtual machine. The long-term aim is a small ARM64 server. This repository publishes the development sources so others can inspect the work and contribute OS and hardware support.

> **Research snapshot, not an installer.** Tested on one 8 GB Mi 11 Pro. There are no downloadable boot images or complete installation scripts in this initial publication. Do not flash another model using these sources. An unlocked bootloader, device-specific backups, and a working recovery route are prerequisites for hardware testing. Even a temporary boot can write storage if its payload mounts a filesystem.

## Observed on the test device

| Component | Status as of 2026-11-14 |
|---|---|
| UEFI → native Linux | Booted; experimental board port and handoff |
| Linux / userspace | Linux 6.7-rc3 based development kernel; Ubuntu 24.04 ARM64 |
| CPU | All 8 logical CPUs online; performance and sustained thermal behavior unqualified |
| RAM | About 7,083 MiB available from 8 GB physical RAM; firmware reservations retained |
| Internal UFS | Ubuntu root filesystem used successfully; device-specific provisioning not included |
| Native display | 1440×3200 DSI/DSC console working on the tested panel variant |
| Power button | One short press blanks/unblanks the display; this is not system suspend |
| USB connection to PC | Network gadget / SSH works with the separate peripheral configuration |
| Onten USB Ethernet | Works with fixed-host configuration; direct LAN TCP test about **94.4 Mbps** |
| USB host ↔ peripheral switching | Experimental userspace workaround: **two PC → Ethernet → PC cycles passed without phone reboot**, using controller reprobe on return to PC; long-term reliability unqualified |
| Temperature | TSENS readings available; Xiaomi battery property-number correction also tested |
| Charging while using Ethernet | **Not yet verified**; PD splitter testing pending |
| Charge limit at 80–90% | **Not implemented** |
| CPU frequency / cooling policy | **Not finished**; do not assume production server readiness |
| Wi-Fi / cellular modem | Not brought up in this port; the test phone also has pre-existing radio faults |
| Camera / audio / GPU acceleration / suspend | Not validated |
| Windows ARM / other OSes | **Not tested and not supported yet**; UEFI boot alone does not supply Windows drivers or ACPI support |

The test phone has physical display line defects. Native display operation does not mean those defects were repaired. The 16 GiB swap used locally is a configuration choice, not additional physical RAM or a requirement for this port.

## What is published

- `patches/uefi/`: tracked UEFI development changes against the pinned base.
- `uefi/overlay/`: added Mars board files and Linux/storage boot probes.
- `patches/linux/`: current kernel changes, **including diagnostic workarounds**.
- `kernel/config/` and `kernel/devicetree/`: kernel configuration and expanded device trees for the two tested USB modes.
- `kernel/panel/`: experimental panel driver; generate its command header from your own matching stock panel data.
- `userspace/`: Power-button daemon and experimental managed USB role service.
- `reference/initramfs/`: development init source for review, not a ready-to-run ramdisk.
- `tools/`: source preparation, panel command generation, FDT inspection and export checks.
- `sources.json`: exact upstream commits and UEFI submodule revisions.

## Start here

1. Read [the English build/research notes](docs/BUILD.en.md) or [catatan bahasa Indonesia](docs/BUILD.id.md).
2. Read the [known limitations](docs/LIMITATIONS.en.md) / [keterbatasan](docs/LIMITATIONS.id.md), especially SMMU and power handling.
3. Use [the testing guide](docs/TESTING.en.md) / [panduan pengujian](docs/TESTING.id.md) before reporting hardware results.
4. See [contributing](CONTRIBUTING.md) and [licenses/provenance](LICENSE.md).

The [managed USB experiment](docs/MANAGED_USB.en.md) / [catatan USB terkontrol](docs/MANAGED_USB.id.md) documents the later role-switching workaround. Normal boot with USB PC connected was also verified after installing that image on the development phone. This does not provide a public installation image or establish simultaneous PD charging + Ethernet.

No proprietary EFI/ADSP/modem firmware, personal partition backups, IMEI/QCN files, SSH credentials or Tailscale state are distributed. This is an independent community experiment, not an official Xiaomi, Qualcomm, Microsoft, or Project Silicium release. Upstream projects are credited without implying endorsement.
