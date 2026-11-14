# Batas baterai eksperimental

HP pengembangan memakai aturan userspace untuk **menghentikan masukan pada 85% dan memulai lagi pada 80%**, diperiksa setiap 15 detik selama Linux berjalan. Ini input suspend, **bukan bypass baterai**: HP memakai baterai saat masukan dihentikan. Proteksi firmware tetap diperlukan. Ambang ini bukan batas hardware yang berlaku pada semua kondisi boot/mati.

## Protokol khusus firmware

Patch kernel menambahkan atribut auxiliary-device khusus Mars di:

```text
/sys/class/power_supply/qcom-battmgr-bat/device/mars_input_suspend
```

Pada firmware stok **OS1.0.11.0.UKAMIXM** yang diuji, permintaan Xiaomi XM memakai GET `0x50`, SET `0x51`, properti **44**, dan nilai boolean. ID dikonfirmasi melalui susunan permintaan `input_suspend_show`/`input_suspend_store` dalam `qti_battery_charger_main.ko` asli dari filesystem vendor yang diekstrak secara lokal. Modul proprietary tidak disertakan di sini.

Jangan menyalin nomor enum XM dari sumber Xiaomi lain: susunan Venus lama berbeda, dan enum Star pun bergantung pada opsi build. Konfigurasi stok memakai `CONFIG_BQ2597X=y` tanpa dual fuel gauge, tetapi menghitung enum sumber saja tetap tidak cocok dengan binary yang lebih baru. Atribut saat ini dibatasi pada `xiaomi,mars`, tetapi driver **belum mendeteksi versi firmware secara otomatis**. Verifikasi protokol secara mandiri sebelum dipakai pada instalasi lain.

Percobaan awal yang menganggap properti pengaturan termal 10 sebagai setter arus FCC dalam mikroampere keliru untuk firmware ini dan memicu crash proses charger saat pemulihan nilai. Perubahan sudah dibatalkan. Koreksi properti arus konstan juga belum membuktikan pembatasan pengisian efektif. Antarmuka tulis FCC tersebut **tidak** disertakan dalam patch akhir yang dipublikasikan. Jangan mengirim nilai arus besar ke indeks tingkat termal.

## Bukti pada perangkat

Dengan USB PC terpasang dan baterai sekitar 75%:

| Kondisi | Status firmware | Arus baterai |
|---|---|---:|
| Masukan diizinkan (`0`) | Charging | sekitar −252 mA |
| Masukan dihentikan (`1`) | Discharging | sekitar +292 mA |
| Masukan diizinkan kembali (`0`) | Charging | sekitar −255 mA |

Pada HP ini, arus negatif dilaporkan ketika mengisi. SSH tetap dapat dipakai selama pengujian. Aturan kemudian diuji dengan ambang berhenti sementara pada kapasitas saat itu, sehingga masukan berhenti; setelah dikembalikan ke 85/80, masukan lanjut karena kapasitas masih di bawah 80%. **Baterai belum benar-benar melewati satu siklus lengkap 85% → 80% pada pengujian ini.**

## Berkas aturan

- `userspace/mars-charge-limit.sh` → `/usr/local/sbin/mars-charge-limit`
- `userspace/mars-charge-limit.service` dan `.timer` → `/etc/systemd/system/`
- `/etc/default/mars-charge-limit`:

```ini
START_PERCENT=80
STOP_PERCENT=85
```

Timer diaktifkan dengan `systemctl enable --now mars-charge-limit.timer` setelah atribut kernel dan fungsi berhenti/lanjut manual terbukti bekerja. Status disimpan di `/var/lib/mars-charge-limit/`. Kegagalan tulis/baca balik membuat marker `fault` dan menghentikan penulisan otomatis berikutnya untuk diperiksa. Skrip tidak mengatur arus pengisian atau menegosiasikan pengisian Xiaomi 67 W.

Untuk menghentikan aturan dan mengizinkan masukan kembali pada instalasi kompatibel yang sudah terverifikasi:

```sh
sudo systemctl disable --now mars-charge-limit.timer
echo 0 | sudo tee /sys/class/power_supply/qcom-battmgr-bat/device/mars_input_suspend
```

Mematikan timer saja tidak membatalkan input suspend pada firmware. PD sambil memakai Ethernet, penggunaan charger dinding, perilaku baterai jangka panjang, serta kompatibilitas charger masih perlu diuji. USB PC yang diuji melaporkan sumber SDP dengan batas 500 mA; label `Fast` dari firmware bukan bukti daya masuk 67 W.
