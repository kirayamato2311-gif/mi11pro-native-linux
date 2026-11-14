# Xiaomi Mi 11 Pro — UEFI & Linux Native

**Bahasa Indonesia** · [English](README.md)

Port UEFI/Linux native eksperimental untuk **Xiaomi Mi 11 Pro (`mars`, Snapdragon 888 / SM8350)**, berdasarkan [Project Silicium](https://github.com/Project-Silicium/Mu-Silicium) dan [kernel Linux mainline SM8350](https://gitlab.com/sm8350-mainline/linux).

Linux berjalan langsung pada HP, tanpa Android, Termux, container, atau mesin virtual. Tujuan jangka panjangnya adalah server ARM64 kecil. Kode pengembangan dipublikasikan agar komunitas dapat memeriksa pekerjaan ini dan membantu dukungan perangkat keras maupun OS lain.

> **Snapshot riset, bukan installer.** Baru diuji pada satu Mi 11 Pro RAM 8 GB. Publikasi awal ini belum menyediakan image boot siap pakai atau skrip instalasi lengkap. Jangan menggunakan port ini untuk mem-flash model lain. Pengujian membutuhkan bootloader terbuka, backup perangkat sendiri, dan jalur pemulihan yang berfungsi. Boot sementara pun dapat menulis penyimpanan jika payload memasang filesystem.

## Hasil pada perangkat uji

| Komponen | Status per 14 November 2026 |
|---|---|
| UEFI → Linux native | Berhasil boot; port board dan handoff masih eksperimental |
| Linux / userspace | Kernel pengembangan berbasis Linux 6.7-rc3; Ubuntu 24.04 ARM64 |
| CPU | Seluruh 8 CPU logis online; performa dan suhu pada beban panjang belum dikualifikasi |
| RAM | Sekitar 7.083 MiB tersedia dari RAM fisik 8 GB; reservasi firmware dipertahankan |
| UFS internal | Root filesystem Ubuntu sudah digunakan; provisioning khusus perangkat belum disertakan |
| Layar native | Konsol DSI/DSC 1440×3200 bekerja pada varian panel yang diuji |
| Tombol Power | Satu tekan singkat memadamkan/menyalakan layar; bukan suspend seluruh sistem |
| USB ke PC | Gadget jaringan / SSH bekerja pada konfigurasi peripheral tersendiri |
| Onten USB Ethernet | Bekerja dalam konfigurasi host tetap; tes TCP LAN langsung sekitar **94,4 Mbps** |
| Perpindahan USB host ↔ peripheral | **Belum otomatis**; mengganti kabel saja belum cukup |
| Suhu | Sensor TSENS tersedia; koreksi nomor properti baterai Xiaomi juga sudah diuji |
| Pengisian sambil memakai Ethernet | **Belum terverifikasi**; menunggu pengujian splitter PD |
| Batas pengisian 80–90% | **Belum dibuat** |
| Frekuensi CPU / kebijakan pendinginan | **Belum selesai**; belum dinyatakan layak sebagai server produksi |
| Wi-Fi / modem seluler | Belum diaktifkan pada port ini; perangkat uji juga memiliki gangguan radio sebelumnya |
| Kamera / audio / akselerasi GPU / suspend | Belum divalidasi |
| Windows ARM / OS lain | **Belum diuji dan belum didukung**; UEFI saja belum menyediakan driver Windows atau dukungan ACPI |

Layar perangkat uji memiliki kerusakan garis fisik. Keberhasilan driver layar bukan berarti kerusakan tersebut diperbaiki. Swap 16 GiB pada pengujian lokal adalah pilihan konfigurasi, bukan tambahan RAM fisik dan bukan persyaratan port.

## Isi repositori

- `patches/uefi/`: perubahan UEFI terhadap commit dasar yang dicatat.
- `uefi/overlay/`: berkas board Mars dan probe boot Linux/penyimpanan.
- `patches/linux/`: perubahan kernel saat ini, **termasuk workaround diagnostik**.
- `kernel/config/` dan `kernel/devicetree/`: konfigurasi kernel dan device tree yang sudah diekspansi untuk dua mode USB yang diuji.
- `kernel/panel/`: driver panel eksperimental; header perintah dibuat dari data panel firmware stok milik pengguna sendiri yang cocok.
- `userspace/`: kode daemon tombol Power.
- `reference/initramfs/`: kode init pengembangan untuk dipelajari, bukan ramdisk siap pakai.
- `tools/`: persiapan sumber, generator perintah panel, pemeriksaan FDT, dan pemeriksaan ekspor.
- `sources.json`: commit upstream dan revisi submodule UEFI yang tepat.

## Mulai dari sini

1. Baca [catatan build dan riset](docs/BUILD.id.md) atau [English notes](docs/BUILD.en.md).
2. Periksa [keterbatasan](docs/LIMITATIONS.id.md), khususnya SMMU dan pengelolaan daya.
3. Ikuti [panduan pengujian](docs/TESTING.id.md) sebelum melaporkan hasil.
4. Lihat [panduan kontribusi](CONTRIBUTING.md) serta [lisensi dan asal sumber](LICENSE.md).

Repositori ini tidak membagikan firmware EFI/ADSP/modem proprietary, backup partisi pribadi, IMEI/QCN, kredensial SSH, atau status autentikasi Tailscale. Ini eksperimen komunitas independen, bukan rilis resmi Xiaomi, Qualcomm, Microsoft, maupun Project Silicium. Pencantuman proyek asal tidak menyiratkan dukungan resmi mereka.
