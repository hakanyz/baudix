# Baudix

## Modern UART / RS232 Terminal for Embedded Engineers

## Vizyon

Baudix, gömülü sistem geliştiricileri için modern, hızlı ve kullanıcı
odaklı bir UART/RS232 terminalidir. Amacı yalnızca seri port üzerinden
veri gönderip almak değil; günlük geliştirme sürecinde sürekli açık
kalan profesyonel bir mühendislik aracı olmaktır.

İlk sürüm yalnızca UART/RS232/USB CDC haberleşmesine odaklanacaktır.
İlerleyen sürümlerde Modbus (RTU/ASCII), TCP, SSH ve diğer haberleşme
protokolleri desteklenebilir.

## Kapsam Dışı (Non-Goals)

-   Network sniffer / paket yakalama aracı değildir (Wireshark benzeri
    bir hedef yok).
-   Genel amaçlı bir IDE veya kod düzenleyici değildir.
-   Scripting/otomasyon motoru (Python, Lua vb. entegrasyonu) hedefte
    yok.
-   Bulut senkronizasyonu veya hesap sistemi yok, tamamen yerel/offline
    çalışır.

------------------------------------------------------------------------

# Tasarım Felsefesi

-   Modern arayüz
-   Hızlı açılış
-   Düşük RAM kullanımı
-   Büyük loglarda akıcı çalışma
-   Tamamen klavye ile kullanılabilme
-   Basit ama güçlü kullanıcı deneyimi

> Amaç en fazla özelliğe sahip terminal olmak değil, en keyifli
> kullanılan terminal olmaktır.

------------------------------------------------------------------------

# Teknoloji

-   C++20 (GCC 11+ / Clang 14+ / MSVC 2022+)
-   Qt 6.5+
-   Qt Widgets
-   QSerialPort
-   CMake 3.21+

------------------------------------------------------------------------

# Desteklenen Platformlar

-   Linux (öncelikli hedef): Ubuntu 22.04+, Fedora, Arch — X11 ve
    Wayland
-   Windows 10/11
-   Paketleme hedefleri: `.deb`, AppImage (Linux) / installer (Windows)

------------------------------------------------------------------------

# Lisans

-   MIT (öneri — açık kaynak dağıtım ve olası ticari/entegrasyon
    kullanımına en az kısıtlamayı koyar; farklı bir lisans tercih
    edilirse burada güncellenmeli)

------------------------------------------------------------------------

# Mimari

``` text
Baudix
├── Core
│   ├── Settings
│   ├── Logger
│   ├── Session
│   ├── Theme
│   └── Utils
├── Communication
│   ├── SerialPort
│   └── Packet
├── Models
├── Views
├── Controllers
└── Plugins (Future)
```

UI hiçbir zaman doğrudan SerialPort sınıfına erişmez.

------------------------------------------------------------------------

# Roadmap

## Çekirdek

### Connection

-   COM Port
-   Baud Rate
-   Data Bits
-   Stop Bits
-   Parity
-   Flow Control
-   Connect / Disconnect
-   Auto Reconnect
-   Son kullanılan portu hatırla

### Terminal

-   ASCII
-   HEX
-   HEX + ASCII
-   RX/TX renkleri
-   Timestamp
-   Auto Scroll
-   Pause
-   Clear

### Logging

-   TXT

### Send

-   ASCII
-   HEX
-   History

------------------------------------------------------------------------

## Küçük Ekler

-   Search (Text / HEX)
-   Logging: Binary, CSV
-   Send: Dosyadan gönder

------------------------------------------------------------------------

## Packet Mode

-   Packet View
-   Packet Separator
-   Timeout
-   Header
-   Byte Count
-   Delimiter

### Highlight

-   Header
-   Payload
-   CRC

------------------------------------------------------------------------

## Packet Builder & Otomasyon

### Packet Builder

-   Header
-   Command
-   Auto Length
-   Payload
-   Auto CRC

### Scheduled Send

-   Periyodik gönderim

### Burst Send

-   N kez gönder

### Macro

-   Reset
-   Version
-   Bootloader
-   Read
-   Write

------------------------------------------------------------------------

## Trigger & İstatistik

### Trigger

Belirli veri geldiğinde: - Macro çalıştır - Ses çıkar - Log başlat

### Statistics

-   RX/TX Byte
-   Packet Count
-   Byte/s
-   Packet/s

### Bookmark

### Live Filter

------------------------------------------------------------------------

## Olgunlaşma

### Session

Port, layout, tema, makrolar ve filtreleri kaydet.

### Docking Layout

Visual Studio benzeri sürüklenebilir paneller.

### Themes

-   Dark
-   Light

### Keyboard Shortcuts

-   Ctrl+L
-   Ctrl+F
-   Ctrl+K
-   Ctrl+S
-   Ctrl+Shift+C

------------------------------------------------------------------------

# Performans

-   10 milyon satırlık log
-   1 Mbps UART trafiğinde akıcı çalışma
-   Thread-safe IO
-   Model/View mimarisi
-   Virtualized log rendering

------------------------------------------------------------------------

# Gelecek

-   Modbus RTU

------------------------------------------------------------------------

# Temel İlkeler

1.  Hız \> Özellik
2.  Basitlik \> Karmaşıklık
3.  Her özellik günlük kullanım değeri taşımalı.
4.  En sık yapılan işler en fazla iki tıklama uzakta olmalı.
5.  Büyük veri akışında akıcılık korunmalı.
6.  Mimari modüler olmalı.
7.  Kod okunabilir ve sürdürülebilir olmalı.

