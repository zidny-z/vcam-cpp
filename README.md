# v-cam-sender

Virtual camera sender untuk Windows 10, kompatibel dengan Chrome & Firefox.
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
- `softcam.dll` (x64) dan `softcam_x86.dll` (Win32)
- `softcam_installer.exe` (x64) dan `softcam_installer_x86.exe` (Win32)
- `register.bat` dan `register_x86.bat`

`sender.exe` membaca gambar relatif terhadap lokasi `sender.exe`, jadi letakkan
`fotoorang2.jpeg` di folder yang sama (atau di subfolder `assets\`).

## Deploy

1. Ekstrak artifact ke satu folder. Jangan pindahkan file setelah registrasi, karena
   installer menyimpan path absolut ke `softcam.dll`.
2. Klik kanan `register.bat` → **Run as administrator** (mendaftarkan `softcam.dll` x64).
   Jika ada aplikasi 32-bit yang perlu memakai kamera ini, jalankan juga `register_x86.bat`.
3. Jalankan `sender.exe`. Console akan menampilkan
   `Select "DirectShow Softcam" as the camera in your app`.
4. Di aplikasi kamera (Chrome/Edge/Firefox/Zoom), pilih kamera **DirectShow Softcam**.
5. Restart browser jika kamera belum muncul di daftar.

### Masih muncul OBS?

`DirectShow Softcam` adalah device kamera **baru dan terpisah**, bukan pengganti
`OBS Virtual Camera`. Selama aplikasi masih memilih `OBS Virtual Camera`, yang tampil tetap
output OBS. Pilih salah satu:

- Ganti kamera di aplikasi/browser ke **DirectShow Softcam**, atau
- Jika ingin output lewat OBS: tambahkan source **Video Capture Device** →
  **DirectShow Softcam** di OBS, lalu klik **Start Virtual Camera** dan pilih
  `OBS Virtual Camera` di aplikasi.

`regsvr32 softcam.dll` bisa dipakai sebagai alternatif `register.bat`, tetapi harus dijalankan
dari Command Prompt **as Administrator** dengan path lengkap ke DLL.

## Troubleshooting

- `fotoorang2.jpeg failed to load` — pastikan gambar ada di folder yang sama dengan
  `sender.exe` (atau di subfolder `assets\`). Pesan error sekarang menampilkan alasan dari
  stb dan direktori kerja saat ini.
- `scCreateCamera failed` — `softcam.dll` belum terdaftar, masih ada `sender.exe` lain yang
  berjalan (hanya satu instance yang diizinkan), atau arsitektur DLL tidak cocok.
