# Keterbatasan yang diketahui

Ini snapshot pengembangan yang telah diperiksa, bukan distribusi yang sudah diperkuat keamanannya.

- Bukti pengujian perangkat baru tersedia untuk `mars` RAM 8 GB dan konfigurasi panel K2 yang diuji. Model lain dan panel pengganti perlu verifikasi mandiri.
- Port UEFI masih mewarisi sebagian referensi board/binary/ACPI `venus`. SoC SM8350 yang sama tidak membuktikan kesamaan board. Dukungan Windows belum diklaim.
- Patch SMMU mempertahankan pemetaan stream ID 0 yang sudah aktif pada SM8350 serta memuat log/jeda diagnostik. Cakupannya lebih luas daripada perbaikan khusus Mars yang matang, mengubah perilaku reset, dan perlu peninjauan sebelum digunakan di luar lingkungan riset.
- Diagnostik pinctrl juga menambah jeda. Auto-boot ADSP dinonaktifkan oleh patch kernel dan membutuhkan startup terkontrol dari userspace.
- Dua device tree awal memakai **mode USB tetap yang berbeda**. Tree dan layanan managed-USB tambahan lolos dua siklus singkat PC/Ethernet memakai pemuatan ulang controller. Ini workaround eksperimental, bukan driver perpindahan peran yang telah dikualifikasi atau bukti pengisian sambil memakai Ethernet; lihat `MANAGED_USB.id.md`.
- Koreksi nomor properti baterai hanya berlaku pada mesin `xiaomi,mars`. Patch yang dipublikasikan belum membuat ambang pengisian atau penulisan kontrol arus. Pembacaan tegangan, kapasitas, dan suhu belum membuktikan kontrol pengisian bekerja.
- Pengaturan frekuensi CPU, kebijakan termal, dan operasi berkelanjutan belum selesai. Kipas tidak menggantikan kebijakan proteksi termal.
- Memori reservasi firmware, eksekusi aman, dan subsistem aktif tidak boleh direklamasi hanya karena suatu alamat tampak tidak terpakai. RAM fisik 8 GB tidak seluruhnya tersedia sebagai RAM biasa.
- Kecepatan sinyal bus USB 480 Mbps bukan hasil pengukuran throughput Ethernet. Jalur adaptor yang diuji menghasilkan sekitar 94,4 Mbps; gigabit belum terbukti.
- Persiapan root filesystem, pemasangan modul, dan perakitan initramfs belum diotomatisasi. Init referensi mengharapkan `/dev/sda34` berlabel `PARTNAME=userdata` serta root ext4 bertanda yang sudah ada. Ini khusus perangkat uji dan **bukan peta partisi universal**.
- Perangkat uji memiliki kerusakan layar dan radio sebelumnya. Tidak semua gangguan berasal dari port, dan perangkat lain belum tentu berperilaku sama.

Belum ada rilis binary sampai izin distribusi firmware, perilaku pemulihan, dan build bersih yang dapat diulang selesai ditinjau.
