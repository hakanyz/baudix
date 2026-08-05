# Baudix – Senior Embedded & Qt Developer Code Review

**Tarih:** 2026-08-04  
**Reviewer perspektifi:** Senior Embedded Systems + Qt Developer  
**Repo:** https://github.com/hakanyz/baudix  
**Versiyon:** 1.2.20  

Bu döküman, mevcut kod tabanının profesyonel değerlendirmesini, teknik borçları ve **Modbus/RTU master-slave** eklenmeden önce yapılması gereken mimari hazırlıkları içerir.  
Antigravity / AI coding assistant ile çalışırken doğrudan referans olarak kullanılabilir.

---

## 1. Genel Mimari Değerlendirme

Mevcut yapı **klasik Qt “MainWindow her şeyi bilen”** anti-pattern’ine yakındır.

| Katman                    | Durum          | Yorum |
|---------------------------|----------------|-------|
| `SerialPortController`    | İyi            | İnce, tek sorumluluklu, signal-slot doğru kullanılmış. Korunmalı. |
| `Updater`                 | Yeterli        | Yan proje için temiz ve işlevsel. |
| `MainWindow`              | **God Object** | Connection, Terminal, Logging, Macros, Tools, Tray, Settings, Update UI hepsi burada. |
| UI / Logic ayrımı         | Zayıf          | View ve business logic iç içe. |
| Protocol abstraction      | Yok            | Modbus eklenince sorun olacak. |

**Sonuç:**  
Şu an “çalışan ve kullanışlı bir araç” seviyesinde başarılı.  
“Profesyonel, uzun ömürlü, genişletilebilir bir terminal + protocol tool” seviyesinde henüz değil.

---

## 2. Güçlü Taraflar

- Qt 6 + CMake + GitHub Actions + `.deb` + udev rules + Inno Setup zinciri doğru kurulmuş.
- Dark theme (One Dark tarzı) ve QSS tutarlı.
- Macro, periodic send, logging, hex/ascii, highlight gibi özellikler gömülü debug ihtiyacını iyi karşılıyor.
- Port refresh (timer + `eventFilter`) pratik ve doğru yerde.
- In-app updater ürün hissi veriyor.
- `SerialPortController` sorumlulukları net ayrılmış.

---

## 3. Kritik Teknik Borçlar (Öncelik Sırasıyla)

### 3.1 Terminal Performansı (En Kritik)

`appendToTerminal` her paket geldiğinde HTML string oluşturup `QTextEdit::append()` yapıyor.

**Sorunlar:**
- Yüksek baud rate + continuous stream altında HTML parse + layout maliyeti birikir.
- `setMaximumBlockCount` olsa bile UI thread kilitlenebilir.
- Scroll ve arama pahalı hale gelir.
- “Both” modu + timestamp + highlight kombinasyonu en kötü senaryoyu yaratır.

**Beklenen çözüm:**
- `QPlainTextEdit` + `QTextCharFormat` kullanmak, **veya**
- Kendi model + virtualized view yapmak.
- HTML’den tamamen kurtulmak.

### 3.2 MainWindow Şişmesi (God Object)

`setupCentralWidget`, `setupDockWidgets` ve constructor içindeki lambda’lar uzun ve karışık.

Modbus RTU tab’ı (master/slave, register map, polling, exception handling) eklendiğinde bu class sürdürülebilirliğini kaybeder.

### 3.3 SerialPortController Hâlâ Basic

- `readAll()` + direkt emit → buffer yönetimi ve back-pressure yok.
- Port ismi `split(" - ")` ile parse ediliyor → kırılgan.
- `writeData` sadece `write()` çağırıyor, partial write / `waitForBytesWritten` handling yok.
- İstatistik yok (RX/TX byte count, error count, overrun).
- Her şey UI thread’inde. 1–2 Mbps continuous data’da risk var.

### 3.4 Persistence Eksikleri

- Macrolar uygulama kapanınca kayboluyor → profesyonel terminalde kabul edilemez.
- Connection preset’leri yok.
- View mode, send format, highlight filter gibi ayarlar tam persist edilmiyor.

### 3.5 Hardcoded Renkler ve Stil

- QSS’te renkler tanımlı.
- `appendToTerminal` içinde de HTML renkleri hardcode (`#61afef`, `#98c379` vb.).
- Tema değiştirmek veya light mode eklemek zorlaşır.

### 3.6 Diğer Edge Case’ler

- `onSendFileClicked` tüm dosyayı memory’ye alıyor → büyük binary riski.
- Logging her satırda `flush()` → yüksek data rate’te disk I/O darboğazı.
- Updater sadece Windows `.exe` ve Linux `.deb` biliyor. Asset seçimi daha esnek olmalı.

---

## 4. Modbus/RTU için Mimari Hazırlık (Kritik)

> “Ayrı sekmede Modbus/RTU master-slave ekleyeceğim” kararı, mevcut mimariyi şu anki haliyle **hazır bulmaz**.  
> Ancak doğru refactor ile kolayca hazır hale getirilebilir.

### Hedef Mimari

```text
┌─────────────────────┐
│   MainWindow        │  ← sadece orchestration + tab yönetimi
└─────────┬───────────┘
          │
    ┌─────┴─────┐
    │           │
┌───▼───┐   ┌───▼──────────────┐
│ UART  │   │ Protocol Layer   │
│Transport│  │ (Modbus RTU,    │
│         │  │  ileride CAN..) │
└───┬───┘   └───┬──────────────┘
    │           │
    └─────┬─────┘
          │
   SerialPortController  ← transport olarak kalsın
```

### Yapılması Gerekenler

1. **`ISerialTransport` arayüzü tanımla**
   ```cpp
   class ISerialTransport {
   public:
       virtual ~ISerialTransport() = default;
       virtual bool open(...) = 0;
       virtual void close() = 0;
       virtual bool write(const QByteArray& data) = 0;
       virtual bool isOpen() const = 0;
   signals: // veya callback
       void dataReceived(const QByteArray& data);
       void connectionStateChanged(bool isOpen, const QString& error);
   };
   ```
   `SerialPortController` bu arayüzü implement etsin.

2. **Modbus RTU için ayrı class’lar**
   - `ModbusRtuMaster`
   - `ModbusRtuSlave`  
   Bu class’lar sadece `ISerialTransport*` bilsin. UI bilmesin.

3. **UI ayrımı**
   - Terminal tab’ı → raw UART (`TerminalWidget`)
   - Modbus tab’ı → `ModbusWidget` (kendi controller’ı olsun)
   - `MainWindow` sadece tab’ları yönetsin, protocol logic içermesin.

4. **Bu ayrımı Modbus kodu yazılmadan önce yap.**  
   Aksi takdirde hem terminal hem Modbus kodu birbirine karışır ve teknik borç katlanır.

---

## 5. Öncelikli Aksiyon Listesi

### Kısa Vade (1–2 hafta – mevcut kaliteyi yükselt)

| # | Görev | Öncelik | Açıklama |
|---|-------|---------|----------|
| 1 | Macroları kalıcı hale getir | Yüksek | `QSettings` veya JSON |
| 2 | Terminal rendering’i düzelt | **Kritik** | `QPlainTextEdit` + `QTextCharFormat` veya model-based |
| 3 | Connection preset’leri ekle | Yüksek | En azından son kullanılan ayarları hatırla |
| 4 | `SerialPortController` güçlendir | Yüksek | İstatistik + sağlam port ismi + write handling |

### Orta Vade (Modbus’a hazırlık)

| # | Görev | Öncelik | Açıklama |
|---|-------|---------|----------|
| 5 | UI parçalarını ayır | Yüksek | `TerminalWidget`, `ConnectionPanel`, `MacroPanel`, `LoggingPanel` |
| 6 | `ISerialTransport` arayüzü çıkar | **Kritik** | Protocol katmanı için temel |
| 7 | Worker thread opsiyonu | Orta | Yüksek baud için |

### Uzun Vade

| # | Görev | Öncelik | Açıklama |
|---|-------|---------|----------|
| 8 | Protocol strategy / plugin yapısı | Orta | Modbus RTU, ileride ASCII / custom frame |
| 9 | Unit test | Orta | Özellikle hex parse, version compare, frame handling |
| 10 | Tema sistemi | Düşük | Renkleri hardcode’dan kurtar |

---

## 6. Özet Not

Şu an **iyi bir “kişisel + ufak takım” aracı** seviyesindesin.

Profesyonel seviyeye geçmek için en büyük engeller:
1. **MainWindow’un her şeyi bilmesi**
2. **Terminal rendering yaklaşımı (HTML + QTextEdit)**

Modbus ekleyeceksen, o sekme gelmeden önce **transport / protocol ayrımını** mutlaka yap.  
Aksi takdirde “Modbus ekledim ama terminal bozuldu / kod okunmaz hale geldi” durumuna düşülür.

---

## 7. Antigravity / AI Assistant İçin Notlar

Bu dökümanı referans alarak çalışırken:

- Öncelikle **Kısa Vade** maddelerine odaklan.
- `MainWindow`’a yeni özellik eklemek yerine önce **UI parçalarını ayırmayı** teklif et.
- Serial işlemleri her zaman `SerialPortController` / `ISerialTransport` üzerinden yap.
- Terminal’e veri basarken HTML string oluşturma yaklaşımından uzak dur.
- Modbus kodu yazmaya başlamadan önce `ISerialTransport` arayüzünün var olduğundan emin ol.

---

*Bu review, 2026-08-04 tarihinde mevcut `master` branch (v1.2.20) üzerinden yapılmıştır.*
