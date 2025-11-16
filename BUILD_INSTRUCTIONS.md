# GflessClient - Build ve Çalıştırma Talimatları

## 🔧 Gereksinimler

### Windows için:
- **Qt 5.15.x veya üzeri** - https://www.qt.io/download
- **Visual Studio 2019/2022** veya **MinGW 64-bit**
- **Git** (zaten kurulu olmalı)

---

## ✅ YÖNtem 1: Qt Creator ile (ÖNERİLEN)

### Adım 1: Qt Kurulumu

1. [Qt Online Installer](https://www.qt.io/download-qt-installer) indir
2. Installer'ı çalıştır ve giriş yap (ücretsiz hesap oluştur)
3. Şunları seç:
   ```
   ✓ Qt 5.15.2 (veya en son 5.x)
   ✓ MinGW 64-bit compiler
   ✓ Qt Creator
   ✓ Qt WebEngine (önemli!)
   ```

### Adım 2: Projeyi Aç

1. **Qt Creator**'ı aç
2. **File** → **Open File or Project**
3. Navigate to: `GflessClient/Launcher/GflessClient.pro`
4. **Open** tıkla

### Adım 3: Configure Project

1. "Configure Project" ekranı açılır
2. **Desktop Qt 5.15.x MinGW 64-bit** seç
3. **Configure Project** tıkla

### Adım 4: Build

1. Sol altta **Debug** ↔️ **Release** değiştir (**Release** seç)
2. **Build** menüsü → **Build Project "GflessClient"**
   - Veya **Ctrl + B**
3. Build tamamlanana kadar bekle (Output penceresinde görebilirsin)

### Adım 5: Çalıştır

1. **▶️ Yeşil Play** butonuna tıkla
   - Veya **Ctrl + R**
2. Program **Administrator** olarak çalışmalı!
   - Eğer hata alırsan, Qt Creator'ı sağ tık → **Run as Administrator**

---

## ⚙️ YÖNTEM 2: Komut Satırı ile Build

### Windows (MinGW)

```bash
# 1. Qt'yi PATH'e ekle
set PATH=C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin;%PATH%

# 2. Proje klasörüne git
cd GflessClient/Launcher

# 3. qmake çalıştır
qmake GflessClient.pro

# 4. Build et
mingw32-make

# 5. Çalıştır
cd release
GflessClient.exe
```

### Windows (MSVC)

```bash
# 1. Qt'yi PATH'e ekle
set PATH=C:\Qt\5.15.2\msvc2019_64\bin;%PATH%

# 2. Visual Studio Developer Command Prompt aç
# "x64 Native Tools Command Prompt for VS 2019" ara ve aç

# 3. Proje klasörüne git
cd GflessClient\Launcher

# 4. qmake çalıştır
qmake GflessClient.pro

# 5. Build et
nmake

# 6. Çalıştır
cd release
GflessClient.exe
```

---

## 📦 DEPLOYMENT (Dağıtım)

Build ettikten sonra executable'ı başka bilgisayarlarda çalıştırmak için:

### 1. Qt DLL'lerini Kopyala

```bash
# Release klasörüne git
cd build-GflessClient-Desktop_Qt_5_15_2_MinGW_64_bit-Release/release

# windeployqt kullan (otomatik DLL kopyalama)
C:\Qt\5.15.2\mingw81_64\bin\windeployqt.exe GflessClient.exe
```

### 2. Gerekli Dosyalar

Aşağıdaki dosyaları **aynı klasöre** kopyala:

```
release/
├── GflessClient.exe         (Ana program)
├── Injector.exe             (DLL injection için)
├── GflessDLL.dll            (Oyun hook DLL'i)
├── Qt5Core.dll              (Qt kütüphaneleri - windeployqt ekler)
├── Qt5Gui.dll
├── Qt5Network.dll
├── Qt5WebEngine.dll
├── Qt5WebEngineCore.dll
└── ... (diğer Qt DLL'leri windeployqt tarafından eklenir)
```

### 3. Dağıtılabilir Paket Oluştur

```bash
# Tüm dosyaları bir klasöre topla
mkdir GflessClient-Metin2-v1.0
cd GflessClient-Metin2-v1.0

# Dosyaları kopyala
copy ..\release\*.* .
copy ..\..\Injector\Injector.exe .
copy ..\..\GflessDLL\GflessDLL.dll .

# README ekle
copy ..\..\..\README.md .
copy ..\..\..\METIN2_SETUP_GUIDE.md .

# ZIP'le
# 7-Zip veya WinRAR ile sıkıştır
```

---

## 🐛 SORUN GİDERME

### Hata: "Cannot find -lkernel32"

**Çözüm:** MinGW düzgün kurulmamış
```bash
# Qt Maintenance Tool ile MinGW'yi yeniden kur
C:\Qt\MaintenanceTool.exe
```

### Hata: "Qt5Core.dll not found"

**Çözüm:** DLL'ler PATH'de değil
```bash
# Option 1: windeployqt kullan
windeployqt GflessClient.exe

# Option 2: Qt bin klasörünü PATH'e ekle
set PATH=C:\Qt\5.15.2\mingw81_64\bin;%PATH%
```

### Hata: "metin2auth.h: No such file"

**Çözüm:** Dosya eksik veya git pull yapılmamış
```bash
git pull origin claude/text2-game-research-018wTERUFruDjW3T6GtXF7t8
```

### Build Çok Yavaş

**Çözüm:** Paralel build aktif et
```bash
# qmake ile
mingw32-make -j4  # 4 core kullan

# Qt Creator'da
# Tools > Options > Build & Run > General
# "Enable parallel builds" seç
```

### Program Administrator İstiyor

**Normal!** DLL injection için admin gerekli.
```bash
# GflessClient.exe'ye sağ tık
# Properties > Compatibility
# ✓ Run this program as an administrator
```

---

## 🏗️ GELİŞTİRME ORTAMI KURULUMU

### VS Code ile Geliştirme (Opsiyonel)

```bash
# 1. VS Code Extensionları kur
- C/C++ (Microsoft)
- Qt tools
- CMake Tools

# 2. .vscode/c_cpp_properties.json oluştur
{
  "configurations": [{
    "name": "Win32",
    "includePath": [
      "${workspaceFolder}/**",
      "C:/Qt/5.15.2/mingw81_64/include/**"
    ],
    "defines": ["QT_CORE_LIB"],
    "compilerPath": "C:/Qt/Tools/mingw810_64/bin/g++.exe"
  }]
}
```

### Debugging (Qt Creator)

```bash
# 1. Debug modda build et
# Sol altta Release → Debug değiştir

# 2. Breakpoint ekle
# Kod satırının soluna tıkla (kırmızı nokta)

# 3. Debug başlat
# F5 veya Debug ikonu

# 4. Step through
# F10 - Step Over
# F11 - Step Into
# Shift+F11 - Step Out
```

---

## 📊 BUILD ZAMANLARI (Tahmini)

| Sistem | İlk Build | Incremental Build |
|--------|-----------|-------------------|
| i5 8GB RAM | ~5 dakika | ~30 saniye |
| i7 16GB RAM | ~2 dakika | ~15 saniye |
| Ryzen 5 16GB | ~3 dakika | ~20 saniye |

---

## 🔗 Faydalı Linkler

- [Qt Documentation](https://doc.qt.io/qt-5/)
- [Qt Creator Manual](https://doc.qt.io/qtcreator/)
- [qmake Reference](https://doc.qt.io/qt-5/qmake-manual.html)
- [Qt WebEngine](https://doc.qt.io/qt-5/qtwebengine-index.html)

---

## ✨ İPUÇLARI

1. **İlk Build Uzun Sürer** - Qt kütüphaneleri derleniyor
2. **Release Build Kullan** - Daha hızlı ve optimize
3. **Antivirus Kapat** - Build sırasında yavaşlatabilir
4. **SSD Kullan** - Build süresini %50 azaltır
5. **Git Pull Düzenli** - En son değişiklikleri al

---

**Başarılar!** 🚀
