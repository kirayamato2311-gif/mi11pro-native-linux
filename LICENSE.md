# Licensing and provenance / Lisensi dan asal sumber

This is a source patch set, not a relicensing of its upstream projects. / Ini kumpulan patch sumber, bukan pelisensian ulang proyek asal.

| Paths / Berkas | Terms / Ketentuan |
|---|---|
| `patches/linux/`, `kernel/panel/` | Linux derivative code: GPL-2.0-only; preserve original copyright/SPDX notices. See `LICENSES/GPL-2.0.txt`. |
| `kernel/devicetree/`, `kernel/config/` | Development configuration/device-tree snapshot built from the pinned Linux tree and board bring-up; distributed under GPL-2.0-only. Hardware constants do not include firmware executable blobs. Preserve any upstream notices. |
| `patches/uefi/`, `uefi/overlay/` | Derived from Project Silicium/Mu UEFI code; preserve embedded file notices, `LICENSES/UEFI-BSD-2-Clause.txt`, and `LICENSES/BSD-2-Clause-Patent.txt` for files bearing that SPDX identifier. Dependencies retain their own licenses. |
| `tools/`, `userspace/`, `reference/initramfs/`, documentation and repository metadata | New project material under MIT (`LICENSES/MIT.txt`), except any file with its own retained upstream notice. |

Exact source revisions are recorded in `sources.json`. / Revisi sumber dicatat dalam `sources.json`.

- UEFI base: [Project-Silicium/Mu-Silicium](https://github.com/Project-Silicium/Mu-Silicium), with Microsoft Project Mu/EDK II and other credited contributors.
- Kernel base: [sm8350-mainline/linux](https://gitlab.com/sm8350-mainline/linux), with Linux and Qualcomm/Linaro contributors as identified in source files.
- Xiaomi battery protocol reference: [MiCode/Xiaomi_Kernel_OpenSource, venus-r-oss, qti_battery_charger.c](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/blob/venus-r-oss/drivers/power/supply/qti_battery_charger.c). The Mars correction accounts for the inserted constant-current property. Later input-suspend support uses an ID independently confirmed from the local stock module; see docs/CHARGING.en.md. No proprietary module is distributed.
- Panel command/PPS input is extracted locally from the user's matching stock panel data and is not bundled.

No proprietary device firmware, upstream binary submodule contents, Windows images, credentials, partition backups or SSH/Tailscale state are part of this repository. Obtain dependencies separately and inspect their applicable terms before redistribution. / Firmware proprietary, isi submodule binary upstream, image Windows, kredensial, backup partisi, dan status SSH/Tailscale tidak disertakan. Siapkan dependensi secara terpisah dan periksa ketentuannya sebelum distribusi ulang.

Credit does not imply endorsement by upstream projects or hardware vendors. / Pencantuman sumber tidak menyiratkan dukungan resmi proyek asal atau vendor perangkat.
