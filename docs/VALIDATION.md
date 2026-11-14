# Publication checks / Pemeriksaan publikasi

Date / Tanggal: 2026-11-14.

Checks performed on the exported source, without flashing or accessing the phone / Pemeriksaan pada sumber ekspor, tanpa flash atau akses HP:

- Public-file pattern scan and local Markdown link checks passed. / Pemindaian pola berkas publik dan tautan Markdown lokal lolos.
- Both patch sets passed `git apply --cached --check --ignore-space-change` against temporary indexes at the pinned upstream commits. Development worktrees were not modified. / Kedua patch lolos pemeriksaan terhadap index sementara pada commit upstream yang dicatat; working tree pengembangan tidak diubah.
- All exported Python files passed syntax parsing. / Sintaks seluruh berkas Python ekspor lolos pemeriksaan.
- Power-button daemon and reference init compiled with the ARM64 cross compiler using `-static -Os -Wall -Wextra -Werror`. / Daemon tombol dan init referensi berhasil dikompilasi dengan opsi tersebut untuk ARM64.
- Both expanded device trees compiled with `dtc`. / Kedua device tree berhasil dikompilasi dengan `dtc`.
- Running the public panel generator on the private matching panel input produced a header byte-for-byte equal to the tested local panel header. Input and generated firmware-derived data were not exported. / Generator publik menghasilkan header identik dengan header panel lokal yang diuji; masukan pribadi dan data hasil ekstraksi tidak diekspor.

These checks do **not** establish a complete reproducible boot-image build, charging control, automatic USB switching, support for Windows/other phones, or production reliability. Hardware observations are listed separately in the README. Pattern scanning supplements manual review; it cannot prove the absence of every kind of sensitive information.

Pemeriksaan ini **tidak** membuktikan build image lengkap yang dapat direproduksi, kontrol pengisian, perpindahan USB otomatis, dukungan Windows/HP lain, atau keandalan produksi. Hasil pada perangkat dicatat terpisah di README. Pemindaian pola membantu peninjauan manual, bukan jaminan mutlak tidak adanya informasi sensitif.
