# Baudix – Güncel Durum ve Öneriler

**Tarih:** 2026-08-06  
**Durum:** Tablo tabanlı yeni arayüz aktif, Linux + Windows’ta test edildi  
**Repo:** Henüz GitHub’a son hali push edilmedi (lokal geliştirme)

---

## 1. Güncel Görsel Durum (Ekran Görüntüsüne Göre)

### Ne değişmiş?
- Eski `QTextEdit` + HTML yaklaşımı terk edilmiş.
- Yerine **tablo** gelmiş:
  - Timestamp
  - Direction (TX badge)
  - Length
  - Data
- Üstte connection bar sadeleşmiş.
- Sağ panel: Macros + Logging
- Altta TX / RX byte sayacı var.

### Olumlu taraflar
- Paket odaklı görünüm profesyonel his veriyor.
- Direction + Length kolonları doğru karar.
- IO Ninja tarzı bir aileye yaklaşmış (bu kötü değil).
- Temel işlevler çalışıyor.

### Hâlâ eksik / netleşmesi gereken
- RX satırları henüz görünmüyor (sadece TX var).
- Framing kuralı (ne zaman yeni satır açılacağı) net değil.
- Uzun continuous data (örneğin 1000+ karakter JSON) için davranış tanımlanmamış.

---

## 2. En Kritik Mimari Konu: Veri / Framing Modeli

Tablo “her satır = bir anlamlı birim” varsayımıyla kurulmuş.  
Bu varsayım continuous stream ve büyük verilerde sorun çıkarabilir.

### Önerilen Minimum Set

Şu üç kuralı koy:

| Mod              | Ne zaman yeni satır açılır?                          | Amaç                          |
|------------------|------------------------------------------------------|-------------------------------|
| **Line-based**   | `\n` veya `\r\n` gelince                             | Klasik text / log             |
| **Timeout**      | X ms boyunca yeni byte gelmezse (öneri: 30–50 ms)    | Binary / genel protokoller    |
| **Max Size**     | Entry 2–4 KB’ı geçerse zorla böl                     | Hiç bitmeyen stream koruması  |

- **TX** tarafında zaten her `write` bir entry olabilir.
- **RX** tarafında yukarıdaki kurallar çalışır.
- Ham byte’lar arka planda mutlaka saklanmalı (tablo sadece görünüm).

Bu üçlü çoğu gerçek senaryoyu karşılar (kısa komut, JSON, dağınık binary).

---

## 3. Profesyonel Araçlar Ne Yapıyor?

IO Ninja, Docklight, HHD vb. araçlar genelde:

1. Ham veriyi timestamp ile saklar.
2. Üzerine farklı **görünüm / framing katmanları** koyar.
3. Tek bir “doğru paket modeli” dayatmaz.

Linux tarafında bu tarz olgun araç sayısı az.  
Bu yüzden senin tablo + direction + length yaklaşımın ekosistemde farklılaşıyor. IO Ninja’ya benzemesi normal ve doğru yönde olduğunu gösterir. Klon olmaya çalışma, kendi odağında (embedded + test/measurement + ileride Modbus) ilerle.

---

## 4. Öncelikli Yapılacaklar (Sıralı)

### Hemen
1. Framing kurallarını netleştir (Line + Timeout + Max Size).
2. RX geldiğinde de aynı tabloda düzgün görünsün.
3. Ham veriyi saklamaya devam et.

### Kısa vadede
4. Macroları kalıcı hale getir (`QSettings` veya JSON).
5. Connection preset / son kullanılan ayarları hatırla.
6. Uzun Data kolonunda okunabilirlik (gerekirse kısaltma + tooltip).

### Orta vadede (Modbus öncesi)
7. `SerialPortController`’ı transport olarak tut.
8. İnce bir `ISerialTransport` arayüzü düşün.
9. Protocol katmanını UI’dan ayır (Modbus RTU ayrı layer olacak).

---

## 5. Karar Özeti

- **Tablo yapısını bozma.** Doğru yöndesin.
- Sadece “ne zaman yeni satır açacağım?” sorusuna net cevap ver.
- Line + Timeout + Max Size ile şimdilik ilerle.
- Fazla seçenek ekleme, ihtiyaç doğunca ekle.
- IO Ninja benzerliği panik sebebi değil, referans alınıp kendi kimliğin korunmalı.

---

## 6. Claude / AI ile Çalışırken Not

- Framing mantığını değiştirirken mevcut TX davranışını bozma.
- Tablo modelini (mümkünse `QAbstractTableModel` + `QTableView`) koru / güçlendir.
- Her yeni özelliği “ham veri kaybolmasın” prensibiyle ekle.
- Modbus koduna başlamadan transport / protocol ayrımını netleştir.

---

*Bu not, 2026-08-06 tarihli ekran görüntüsü ve önceki senior review’lara dayanarak hazırlanmıştır.*
