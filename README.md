# v-cam-sender

Virtual camera sender untuk Windows 10/11, kompatibel dengan Chrome & Firefox.
Menggunakan [Softcam](https://github.com/tshino/softcam) sebagai DirectShow filter.

## Komponen

- `src/sender.cpp` — aplikasi pengirim frame (membaca `fotoorang2.jpeg`, resize ke 640x480, lalu mengirim ke Softcam)
- `assets/fotoorang2.jpeg` — gambar yang ditampilkan
- `external/softcam/` — source DirectShow filter (di-clone otomatis oleh workflow CI, tidak di-commit)

## Build

Build otomatis lewat GitHub Actions (`.github/workflows/build.yml`). Setiap push ke `main`
menghasilkan artifact `vcam-sender-win64` yang berisi:

- `sender.exe` (x64)
- `fotoorang2.jpeg`
- `softcam.dll` (x64)
- `softcam_x86.dll` (Win32, untuk aplikasi 32-bit)

Tidak ada installer dan tidak ada langkah registrasi manual. `sender.exe` membaca gambar relatif
terhadap lokasi `sender.exe`, jadi letakkan `fotoorang2.jpeg` di folder yang sama (atau di
subfolder `assets\`).

## Deploy

1. Ekstrak artifact ke satu folder.
2. Jalankan `sender.exe`. Saat pertama dijalankan, aplikasi mendaftarkan kamera
   **DirectShow Softcam** untuk user saat ini (per-user, **tanpa admin**).
   Biarkan jendela console tetap terbuka selama kamera dipakai.
3. Di aplikasi kamera (Chrome/Edge/Firefox/Zoom), pilih kamera **DirectShow Softcam**.
4. Restart browser jika kamera belum muncul di daftar.

Jika folder dipindah, cukup jalankan `sender.exe` lagi; path DLL akan diperbarui otomatis.

### Perintah

```
sender.exe                 register (per-user) lalu jalankan kamera
sender.exe --register      register saja
sender.exe --unregister    hapus registrasi untuk user ini
```

### Masih muncul OBS?

`DirectShow Softcam` adalah device kamera **baru dan terpisah**, bukan pengganti
`OBS Virtual Camera`. Selama aplikasi masih memilih `OBS Virtual Camera`, yang tampil tetap
output OBS. Pilih salah satu:

- Ganti kamera di aplikasi/browser ke **DirectShow Softcam**, atau
- Jika ingin output lewat OBS: tambahkan source **Video Capture Device** →
  **DirectShow Softcam** di OBS, lalu klik **Start Virtual Camera** dan pilih
  `OBS Virtual Camera` di aplikasi.

## Troubleshooting

- `fotoorang2.jpeg failed to load` — pastikan gambar ada di folder yang sama dengan
  `sender.exe` (atau di subfolder `assets\`). Pesan error menampilkan alasan dari stb dan
  direktori kerja saat ini.
- `scCreateCamera failed` — masih ada `sender.exe` lain yang berjalan (hanya satu instance yang
  diizinkan), atau `softcam.dll` tidak cocok dengan arsitektur `sender.exe` (x64).
- Kamera belum muncul di browser — tutup lalu buka ulang browser. Beberapa aplikasi hanya
  membaca daftar kamera saat start.
