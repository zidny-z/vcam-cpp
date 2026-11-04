# v-cam-sender

Virtual camera sender untuk Windows 10, kompatibel dengan Chrome & Firefox.
Menggunakan [Softcam](https://github.com/tshino/softcam) sebagai DirectShow filter.

## Komponen

- `external/softcam/` — DirectShow virtual camera filter (submodule)
- `src/sender.cpp` — aplikasi pengirim frame
- `assets/fotoorang2.jpeg` — gambar yang ditampilkan

## Build

(diisi setelah workflow siap)

## Deploy

1. Registrasi filter: `regsvr32 softcam.dll` (butuh admin, sekali saja)
2. Jalankan `sender.exe`
