# Catatan build dan riset

Dokumen ini menjelaskan masukan pengembangan. Build bersih lengkap yang menghasilkan ulang instalasi akhir **belum divalidasi**. Belum ada installer root filesystem otomatis atau image boot siap pakai.

## Lingkungan dan versi sumber

Gunakan host Linux (pengembangan awal memakai Ubuntu 24.04 di WSL), Python 3, Git, cross compiler AArch64, GNU make, `dtc`, dan dependensi masing-masing proyek asal. UEFI juga membutuhkan lingkungan Python, alat build EDK II/Mu, serta submodule binary perangkat/ACPI upstream.

Commit sumber dan revisi submodule ada di [`sources.json`](../sources.json). Siapkan checkout upstream pada commit yang tepat. Ukuran sumber kernel/UEFI dan toolchain dapat mencapai beberapa gigabyte; helper tidak mengunduhnya.

Jangan unggah firmware hasil ekstraksi, device tree lengkap perangkat, host key, authorized key, atau backup partisi pribadi. Siapkan firmware yang dibutuhkan dari perangkat/paket stok sendiri sesuai ketentuannya.

## Menyiapkan checkout

Jalankan pada Linux dari repositori ini dengan **checkout bersih yang terpisah**:

```sh
python3 tools/prepare-sources.py --uefi /path/to/Mu-Silicium --kernel /path/to/linux
python3 tools/prepare-sources.py --uefi /path/to/Mu-Silicium --kernel /path/to/linux --apply
```

Perintah pertama hanya memeriksa commit dan penerapan patch. Perintah kedua mengubah sumber tersebut dan menyalin overlay UEFI. Keduanya tidak melakukan build atau flash. Cocokkan juga revisi submodule yang dicatat, bukan memperbaruinya sembarangan ke versi terbaru.

## Kernel dan device tree

Gunakan `kernel/config/mars-bringup.config` sebagai catatan konfigurasi, bukan jaminan bahwa setiap hasil full build telah diuji. Titik awal konvensionalnya:

```sh
mkdir -p /path/to/kernel-build
cp kernel/config/mars-bringup.config /path/to/kernel-build/.config
make -C /path/to/linux O=/path/to/kernel-build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
make -C /path/to/linux O=/path/to/kernel-build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j4 Image modules
mkdir -p out
dtc -I dts -O dtb -o out/mars-usb.dtb kernel/devicetree/mars-native-display-test.dts
dtc -I dts -O dtb -o out/mars-lan.dtb kernel/devicetree/mars-native-display-ethernet-test.dts
```

Device tree sudah diekspansi dan tidak memerlukan include header lokal. `mars-usb` memakai peripheral tetap tanpa node ADSP/PMIC; `mars-lan` memakai host tetap beserta ADSP/PMIC. Keduanya sengaja terpisah. Periksa kembali reservasi memori untuk perangkat/firmware berbeda.

Sebagian komponen pada konfigurasi dibangun/dimuat sebagai modul tambahan selama riset. Periksa dependensi modul, `depmod`, dan urutan startup userspace; membuat `Image` saja belum cukup. Komponen penting mencakup PHY/controller UFS, pinctrl, SMMU, PMIC GLINK, ADSP/QRTR/PDR, layar/DSI, dan REFGEN. Jangan mengekstrak arsip modul di atas symlink `/lib`; gunakan direktori terbatas di `/usr/lib/modules/<release>/`.

## Driver panel

Panel yang diuji adalah `qcom,mdss_dsi_k2_38_08_0a_mp_dsc_cmd`: DSC 1.1, 10 bit per komponen, 8 bit terkompresi per piksel, dua slice 720×50. Pastikan panel perangkat sendiri terlebih dahulu.

Ekstrak **DTSI khusus panel yang dipilih** dari device tree stok sendiri. Generator membutuhkan DTSI yang sudah didekode dengan node timing 60/90/120 Hz:

```sh
python3 tools/generate-panel-commands.py /path/to/selected-panel.dtsi --output /path/to/generated-panel
cp /path/to/generated-panel/mars-panel-commands.h kernel/panel/
make -C /path/to/linux O=/path/to/kernel-build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M="$PWD/kernel/panel" modules
```

Header perintah/data PPS hasil ekstraksi sengaja tidak dibagikan. Generator memeriksa batas paket dan dimensi DSC, bukan membuktikan kompatibilitas hardware. Data 60 Hz yang diuji memiliki 73 perintah menyala dan 11 perintah padam. Patch kernel memuat koreksi kedalaman input DSC dan pembagian paket; keduanya dibutuhkan pada perangkat uji.

## UEFI dan handoff

Ikuti panduan build UEFI upstream yang versinya dicatat setelah menerapkan overlay. Target Mars dipilih dengan `python build_uefi.py -d mars` di sumber UEFI yang telah disiapkan beserta dependensinya. Port awal masih memakai referensi binary/ACPI `venus`; audit ketergantungan dan lisensinya secara terpisah.

Probe boot mengharapkan payload FAT dengan susunan:

```text
EFI/MARSDIAG/Image
EFI/MARSDIAG/mars-efi-minimal.dtb
EFI/MARSDIAG/mars-diagnostic.cpio.gz
```

`Resources/DTBs/mars.dtb` juga menjadi masukan packaging UEFI lokal pada build awal dan tidak disertakan. Siapkan/audit secara terpisah; jangan menggantinya dengan live DTB milik model lain. Pipeline akhir packaging FAT/Android boot image beserta seluruh masukan firmware belum dibuat dapat direproduksi pada publikasi ini. Lolos pemeriksaan sumber tidak berarti firmware selesai dibangun ulang.

## Userspace dan initramfs

Daemon tombol dapat dikompilasi dengan:

```sh
aarch64-linux-gnu-gcc -static -Os -Wall -Wextra -Werror userspace/mars-screen-key.c -o out/mars-screen-key
```

Daemon memakai akses input eksklusif dan framebuffer blanking. Dibutuhkan layar native `msmdrmfb`, input QCOM PON, serta koordinasi pengaturan logind. Ini bukan suspend seluruh sistem.

`reference/initramfs/init.c` adalah referensi init pengembangan awal. Kode mengharapkan binary/modul tambahan, root filesystem bertanda, dan nomor partisi perangkat uji. Jangan memasangnya tanpa penyesuaian pada HP lain. Authorized key, host key Dropbear, image filesystem, dan skrip provisioning/format awal tidak disertakan. Gunakan identitas SSH sendiri dan periksa identitas partisi sebelum operasi penyimpanan.
