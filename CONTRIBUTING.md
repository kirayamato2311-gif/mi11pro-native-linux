# Contributing / Kontribusi

**English:** Issues and pull requests are welcome in English or Bahasa Indonesia. Start with the status table and limitations. Keep changes small, identify the upstream base, preserve licenses, and state whether a change is source-reviewed, compile-checked, or tested on hardware. A successful compile is not proof of hardware support. Include recovery behavior and sanitized logs with hardware results. Never attach personal firmware backups, IMEI/QCN data, credentials or proprietary images.

**Bahasa Indonesia:** Issue dan pull request boleh menggunakan bahasa Indonesia atau Inggris. Baca tabel status dan keterbatasan terlebih dahulu. Buat perubahan terfokus, sebutkan sumber upstream, pertahankan lisensi, dan bedakan hasil pemeriksaan kode, kompilasi, serta pengujian perangkat. Berhasil dikompilasi belum membuktikan dukungan hardware. Sertakan perilaku pemulihan dan log yang sudah dibersihkan. Jangan lampirkan backup firmware pribadi, IMEI/QCN, kredensial, atau image proprietary.

Priority research / Prioritas riset:

- Automatic USB role switching and PD sink + USB host operation / Perpindahan peran USB dan pengisian PD sambil menjadi USB host.
- Verified charge-control protocol and battery thresholds / Protokol kontrol pengisian dan ambang baterai yang teruji.
- CPU frequency, cooling policy, sustained load and power-loss recovery / Frekuensi CPU, pendinginan, beban panjang, dan pemulihan kehilangan daya.
- Narrow, upstream-quality SMMU and PMIC fixes / Perbaikan SMMU dan PMIC dengan cakupan tepat untuk upstream.
- A repeatable build and a documented, recoverable installation process / Build yang dapat diulang dan instalasi dengan jalur pemulihan jelas.
- Independent Windows ARM/other OS research; no existing support is implied / Riset Windows ARM/OS lain secara mandiri; belum ada klaim dukungan.
