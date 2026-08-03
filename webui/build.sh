
#!/bin/bash

echo "🚀 [1/4] Building WebUI (Vite/React/Vue)..."
npm run build

echo "🧹 [2/4] Membersihkan webroot lama..."
rm -rf module/webroot
mkdir -p module/webroot

echo "📂 [3/4] Mengopi isi folder 'dist/' ke 'module/webroot/'..."
cp -r dist/* module/webroot/

echo "📦 [4/4] Membungkus folder 'module/' menjadi file ZIP..."
cd module
rm -f ../LUMina_Tweaks.zip
zip -r9 ../LUMina_Tweaks.zip .
cd ..

echo "✅ SELESAI! File 'LUMina_Tweaks.zip' sudah dibuat di luar dan SIAP DI-FLASH!"
