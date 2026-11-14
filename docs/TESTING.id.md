# Pengujian perangkat dan laporan

Mulai dari peninjauan kode. Publikasi awal ini belum memiliki prosedur instalasi yang didukung atau image boot terverifikasi untuk diunduh. Jangan memakai HP utama atau model selain `mars`.

Sebelum boot payload hasil build sendiri, simpan backup stok/recovery pribadi yang terverifikasi dan pastikan pemulihan fastboot berfungsi. Membuka bootloader serta provisioning penyimpanan dapat menghapus data. Jangan unggah backup tersebut secara publik.

Laporkan setiap komponen secara terpisah:

1. **Identitas:** codename, RAM, versi firmware stok, dan varian panel jika diketahui. Hapus nomor seri/IMEI/MAC/identitas akun.
2. **Build:** commit repositori ini, versi upstream, konfigurasi dan perbedaan patch, hash image, mode USB, serta versi userspace.
3. **Boot:** boot sementara/permanen, tahap terakhir yang tercapai, keluaran konsol, dan perilaku pemulihan. Jangan menganggap metadata A/B maupun nomor partisi sama dengan perangkat lain.
4. **Layar/Power:** tulisan konsol bergerak, nama framebuffer native, satu tekan singkat padam/menyala, dan konektivitas server saat layar padam. Jangan memakai tahan Power berulang sebagai kontrol layar biasa.
5. **Jaringan:** identitas chipset/driver adaptor, link hasil negosiasi, throughput TCP kabel lokal, error/drop, kecepatan internet, serta Tailscale langsung/relay sebagai pengukuran terpisah.
6. **Daya/termal:** model charger/hub, tren persentase baterai, status pengisian, suhu baterai dan SoC, durasi beban, serta clock CPU jika tersedia. Hentikan jika suhu tidak wajar atau perangkat restart. Lampu menyala saja belum membuktikan baterai sedang diisi.

Untuk Windows/OS lain, mulai dari issue/diskusi yang menjelaskan jalur boot dan komponen yang belum ada. Laporkan pekerjaan ACPI, penyimpanan, layar, dan driver USB secara terpisah. Loader Windows yang tampil belum berarti Windows didukung. Jangan unggah image Windows, paket driver proprietary, atau firmware pihak lain tanpa izin distribusi yang sesuai.

Gunakan template issue dua bahasa. Bersihkan log sebelum diunggah; device tree dan log boot dapat mengandung identitas. Simpan hash image yang diubah serta catatan pemulihan secara lokal juga.
