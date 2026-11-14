# Eksperimen USB terkontrol

Pada 14 November 2026, dua siklus berurutan PC → Onten Ethernet → PC berhasil tanpa restart HP uji. SSH bekerja melalui LAN dan kembali melalui gadget jaringan USB. Restart normal dengan kabel PC terpasang juga mengaktifkan kembali layanan, layar native, tombol Power, dan SSH USB setelah pemasangan pada HP pengembangan.

Workaround memakai `kernel/devicetree/mars-native-managed-usb-test.dts` dan `userspace/mars-managed-usb.sh` beserta contoh service. Tree mempertahankan ADSP/PMIC, tetapi sengaja tidak menghubungkan graph konektor UCSI ke DWC3 agar perubahan peran otomatis tidak saling bersaing. Mode OTG memakai peripheral sebagai default dan marker kondisional `mars,managed-usb-test`.

Layanan menunggu partner/peran data UCSI stabil sekitar tiga detik. Sebelum menjadi host, gadget USB dilepas ikatannya. Saat kembali dari host ke device, layanan **melepas/memasang ulang hanya controller platform DWC3**, lalu mengikat gadget yang sudah ada dan memulihkan alamat jaringan USB. Perubahan mode DRD biasa sebelumnya selesai di sisi software tetapi gagal memulihkan deteksi PC; pemuatan ulang controller berhasil pada dua siklus yang diuji. Penyebab mendasar pada hardware/driver belum diisolasi.

Dependensi dan batasan:

- Gadget configfs bernama `mars` harus sudah dikonfigurasi; skrip ini tidak membuatnya.
- Skrip mengharapkan alamat DWC3, debugfs, port UCSI, driver, dan susunan userspace yang diuji. Jangan langsung memasangnya pada HP lain.
- Subnet USB uji adalah `192.168.77.0/24` (HP `.2`). PC membutuhkan alamat yang sesuai; pada Windows pengembangan, alamat `.1` perlu dipulihkan setelah enumerasi ulang.
- Perpindahan memutus transport jaringan; sesi SSH yang sedang aktif tidak dipertahankan.
- Pengujian berhenti jika peran berubah lebih dari tiga kali dalam 30 detik. Restart layanan otomatis dinonaktifkan untuk menghindari loop reset.
- Perekaman kernel terus-menerus hanya aktif jika `/etc/mars-managed-usb-debug` ada; log perpindahan biasa tetap tersedia.
- Pembacaan melalui USB PC menunjukkan pengisian dari SDP dengan batas 500 mA. Label charge type `Fast` dari firmware **bukan** bukti pengisian 67 W.
- Cold boot, suspend, keandalan hotplug panjang, semua kombinasi charger/hub, serta PD sink + Ethernet bersamaan belum dikualifikasi. Ambang pengisian kemudian dibuat secara terpisah; lihat `CHARGING.id.md`.

Pertahankan tree host tetap dan peripheral tetap sebagai konfigurasi referensi/pemulihan. Repositori ini tidak membagikan binary boot, provisioning root filesystem, atau otomasi flashing.
