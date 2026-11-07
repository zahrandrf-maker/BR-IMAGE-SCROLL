# BR Running Logo — V0.1.1 Beta

Source FFGL transparan untuk menjalankan beberapa logo seperti running text di
Resolume Arena/Avenue Windows. Original effect by **BELAJAR RESOLUME**.

## Fitur

- Memilih satu atau beberapa logo sekaligus melalui `Manage Logos`.
- Mendukung PNG transparan, JPG/JPEG, dan BMP.
- Mengatur urutan logo, menghapus logo, dan membersihkan seluruh daftar.
- `Original / Individual Size` mempertahankan perbedaan ukuran antarfoto.
- `Similar Size` menyamakan tinggi logo tanpa merusak aspect ratio.
- Alignment Top, Center, atau Bottom.
- `Logo Gap` untuk jarak antarlogo.
- `Loop End Gap` khusus untuk jarak logo terakhir menuju logo pertama.
- Static, Left, Right, Up, Down, Bounce X, dan Bounce Y.
- Speed, Start, Pause, Resume, dan Restart.
- Animation In/Out Scroll X/Y, invert, dan Play Out.
- Box width/height, padding, opacity, rounded corner, warna, dan stroke.
- Position serta rotasi 0/90/180/270 derajat.
- Output alpha/transparan untuk digunakan pada layer tersendiri.

## Build melalui GitHub

1. Buat repository GitHub kosong atau gunakan repository khusus BR Running Logo.
2. Upload **semua isi folder ini**, termasuk folder `.github`.
3. Buka tab **Actions** lalu pilih **Build BR Running Logo**.
4. Klik **Run workflow**.
5. Setelah selesai, unduh artifact `BR-Running-Logo-Windows-v0.1.1-beta`.

GitHub Actions akan menghasilkan satu file plugin Windows x64 tanpa perlu
Visual Studio pada komputer Anda.

## Instalasi

1. Tutup Resolume.
2. Salin `BR Running Logo.dll` ke folder plugin FFGL Resolume.
3. Buka Resolume kembali.
4. Masuk ke panel **Sources**, cari **BR Running Logo**, lalu tarik ke clip.

Lokasi yang umum digunakan:

`Documents\\Resolume Arena\\Extra Effects`

## Cara menggunakan

1. Buka grup `Logo Setting`.
2. Klik `Manage Logos`.
3. Klik `Add Logos`, pilih satu atau banyak file.
4. Susun daftar menggunakan `Move Up` dan `Move Down`.
5. Klik `Apply Logos`.
6. Atur Direction, Speed, Logo Gap, Loop End Gap, ukuran, alignment, dan box.

## Perbedaan pengaturan ukuran

- **Original / Individual:** ukuran relatif setiap file dipertahankan. Logo
  beresolusi besar tetap lebih besar dari logo beresolusi kecil.
- **Similar Size:** semua logo menggunakan tinggi yang sama melalui parameter
  `Similar Height`; lebarnya tetap mengikuti aspect ratio.
- **Global Logo Scale:** memperbesar atau memperkecil seluruh rangkaian tanpa
  mengubah perbandingan ukuran antarlogo.

## Penyimpanan file

Plugin menyimpan alamat file logo ke composition/preset. Jangan memindahkan atau
menghapus file setelah digunakan. File yang hilang akan dilewati agar source
tidak membuat Resolume crash. Untuk memindahkan project ke komputer lain,
salin logo dan pilih ulang file melalui `Manage Logos`.

## Catatan Beta

- Target awal: Windows 10/11 x64 dan Resolume 7.23.2 atau lebih baru.
- Maksimum atlas internal 8192 px; rangkaian yang terlalu panjang akan
  diperkecil otomatis agar tetap aman untuk GPU.
- Popup Logo Manager berjalan pada UI thread terpisah agar tidak memblokir
  antarmuka dan render Resolume.
- Uji terlebih dahulu dengan GPU, resolusi composition, serta jumlah logo yang
  biasa digunakan sebelum dipakai pada live show penting.

## Credit

**Effect by Belajar Resolume**

## Changelog

### V0.1.1 Beta

- Memperbaiki konflik nama fungsi `clamp01` dengan FFGL SDK yang menyebabkan
  Build Release gagal.
- Membersihkan warning konversi ID kontrol Windows ke `HMENU` pada Logo Manager.
