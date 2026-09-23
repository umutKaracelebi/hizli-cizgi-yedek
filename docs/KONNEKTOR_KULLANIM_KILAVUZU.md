# ATLAS 1.4.3 — Konnektörler ve Bağlantı Rehberi

Tarih: 22 Eylül 2026. Bu dosya, hobi elektroniği deneyimi olmayan bir takım üyesinin bile robotu güvenle takıp sökebilmesi için yazıldı. Fotoğraflardaki üç harici parça (`connector1/2/3`) bu oturumda görsel olarak doğrulandı.

## 0. Altın kurallar — her bağlantıdan önce oku

1. **Bağlantı takarken/çıkarırken pil takılı olmasın.** İstisna: koşu veya test sırasında pil bilinçli takılır.
2. **Konnektörler "oturuyor" diye doğru takılmış sayılmaz.** Aynı boyuttaki soketler farklı voltaj/sinyal taşıyabilir. Doğrulama = kart üzerindeki **yazılı etiketi** okumak, kablo rengine güvenmek değil.
3. **Pin-1 işareti:** Şerit kablolarda kırmızı tel = pin 1. Kart üzerinde pin-1 genelde bir üçgen/ok veya köşeli lehim yamasıyla gösterilir. Adaptör üzerinde MOSI/MISO/SCK/RST/VCC/GND yazıyorsa, robot tarafındaki aynı etiketlerle **birebir aynı sırada** hizalanır.
4. **İki 5 V kaynağını aynı anda paralel bağlama.** USBasp'dan gelen 5 V ile robotun kendi regülatörü (78M05) çakışabilir. Kural: **USBasp bağlıyken pil takılıysa**, USBasp'nin hedef-besleme jumper'ı kapalı olmamalı (aşağıda anlatılıyor).
5. **Türbin yalnızca 2S alır.** Türbin kablosu pilin **balans soketinden** beslenir ve asla 3S paketin tamamına bağlanmaz (kılavuz s.5, 19).
6. Emin değilsen dur. Multimetre ile ölç; deneme-yanılma ile pil bağlama.

---

## 1. Robotun üzerindeki giriş/çıkış noktaları

| Nokta | İşlev | Ne takılır |
|---|---|---|
| **XT30** (büyük sarı soket) | Ana pil girişi, sürüş motorlarının beslemesi | 2S LiPo pil (ilk testlerde yalnız 2S) |
| **Balans soketi** (JST-XH, çok pinli) | Pil hücre uçları + **türbin beslemesi** | Pilin balans fişi + türbinin 2S kablosu |
| **ICSP (6 pin)** | Firmware yükleme | USBasp zinciri (10→6 adaptörle) |
| **UART (6 pin)** | Seri debug/iletişim, 115200 baud | USB-TTL (connector2) + panel adaptör (connector3) |
| **Start portu (4 pin)** | START/STOP modülü | MEBSTART: sinyal → **GO** pini |
| **FFC/FPC** (ince şerit) | 16'lı çizgi sensörü | Sensör barı şerit kablosu — takılı kalır |

Kılavuz s.22 fotoğrafında start portu 4 pinli: **+ − GO RDY** sıralaması görülüyor; yine de takmadan önce kendi kartındaki etiketleri oku.

---

## 2. connector1 = USBasp programcı (firmware yüklemek için)

**Ne işe yarar:** Bilgisayardaki `.ino` kodunu robota yazar. Seri port değildir; COM portu seçmezsin.

**Görünüm (fotoğraftan doğrulandı):** Mavi PCB, USB-A erkek fiş, üzerinde ATmega8A entegresi ve "USBASP v2.0" yazısı. İki uçlu **10 pin şerit kablo** + küçük bir **10→6 pin adaptör kartı** (üzerinde MOSI, MISO, SCK, RST, VCC, GND etiketleri).

**Bağlantı zinciri:**

```
PC (USB) ── USBasp ── 10 pin şerit ── 10→6 adaptör ── Robot ICSP portu
```

**Adım adım:**
1. Robot kapalı, pil bağlantısı kararı §5'teki gibi verilmiş olsun.
2. Şeridin kırmızı teli (pin 1) her iki sokette de pin-1 işaretine gelsin.
3. Adaptörün 6 pini robot ICSP'sine etiketler aynı yönde oturacak şekilde takılır.
4. İlk kullanımda Windows sürücüsü gerekir: **Zadig** → "USBasp" seç → sürücü **WinUSB (v6.1.7600.16385)** → Install Driver (kılavuz s.11).
5. Arduino IDE: **Kart = Arduino Nano, İşlemci = ATmega328P, Programlayıcı = USBasp.** COM portu SEÇİLMESİN.
6. **Taslak → Programlayıcı ile Yükle** (Ctrl+Shift+U). Normal "Yükle" butonu değil.
7. 5–15 sn sonra "Done uploading." → USBasp'yi sökebilirsin.

**Önemli:** Bu yöntem bootloader kullanmaz/siler; bu normal ve beklenen.
**Uyarı:** USBasp üzerindeki hedef-besleme jumper'ı robot 5 V'ını besleyebilir. Pil takılıyken bu jumper kapalıysa iki 5 V hattı çakışır. İlk denemede: pil takılı, USBasp hedef-besleme jumper'ı **açık** (varsa). Robot bu sırada kendi piliyle açık olmalı (kılavuzun fotoğrafları bu düzeni gösteriyor).

---

## 3. connector2 = CH340 USB-TTL dönüştürücü (seri monitör için)

**Ne işe yarar:** Robotun debug/DebugMode çıktısını bilgisayarda görmek içindir. Kod yükleyemez.

**Görünüm (fotoğraftan doğrulandı):** USB-C fiş, CH340 entegresi, 6 pin header. Etiket sırası: `5V  VCC  3V3  TXD  RXD  GND`. Sarı jumper **VCC–5V** arasında → çıkış/lojik 5 V seviyesi (robot için doğru).

**Kurallar:**
- **Bağlantı sırası (kılavuz s.17):** önce robotun UART portuna tak, sonra USB'yi bilgisayara tak.
- **Çapraz bağlantı:** dönüştürücünün TXD'si robotun RX'ine, RXD'si robotun TX'ine gider. Panel adaptör (connector3) bu çaprazlamayı fişiksel olarak yapar; elde tek tek kablo çekersen TX→RX, RX→TX, GND→GND.
- **VCC/+5V hattı:** robot pilden besleniyorsa USB-TTL'nin 5V pinini robotun 5V'sine BAĞLAMA; sadece TX/RX/GND yeter. Robotu yalnız USB-TTL'nin 5V'siyle beslemeye kalkma — motorlar o hattan beslenemez.
- Jumper'ın VCC–3V3 konumuna **almayın**; 5V'ta kalır.

---

## 4. connector3 = Panel adaptör kartı (ara bağlantı)

**Ne işe yarar:** USB-TTL'nin 6 pinini robotun UART ve start portlarının fiziksel düzenine çeviren pasif kart (üzerinde lojik tersleme yoktur — sadece pin düzeni).

**Görünüm (fotoğraftan doğrulandı):** Üzerinde entegre YOK (pasif). Bir yüzde `DTR RX TX GND +5V +5V`, diğer yüzde `GND RX TX +3V VCC` etiketleri; beyaz JST soketler ve gökkuşağı şerit kablo.

**Kullanım:** CH340 ↔ panel adaptör ↔ robot UART. Etiket eşleştir her iki tarafta da; DTR otomatik reset içindir, bizim yüklememiz ICSP'den yapıldığı için DTR kullanılmıyor (bağlı kalabilir, zararsız).

**Bilinen açık:** Adaptörün MEBSTART'in aktif-LOW sinyalini terslemediği kabul ediliyor (içinde transistör/yonga yok). Takım firmware'i bu yüzden aktif-LOW'u yazılımda tersler.

---

## 5. MEBSTART başlatma modülü (yarının konusu)

**Ne işe yarar:** Hakem kumandasından IR ile START/STOP alır; robotun yarışmaya uygun uzaktan başlatılması/durdurulması.

**Elektriksel gerçek (satıcı beyanı + kod doğrulandı):**
- 3 pin: **Sinyal (S), Eksi (−), Artı (+5V)**. 3.3 V uyumlu DEĞİL.
- Beklemede sinyal = **5 V (HIGH)**; START/STOP komutunda **0 V (LOW)** — yani **aktif-LOW**, tek sinyal.
- Modül üzerinde LED var (şartnamedeki LED testi bununla görülür).

**Robota takma:**
- Robot start portunun `+ − GO RDY` etiketlerini robot üzerinde oku.
- MEBSTART **Sinyal → GO**, **−→ −**, **+ → +**. **RDY boşta kalır** (tek sinyal iki ayrı READY/GO üretemez; takım firmware'i READY'yi kullanmaz, ön-vakumu yazılımdan yapar).
- Modülün IR alıcı penceresi robotun üstünde dışa dönük ve gölgelenmemiş olsun (kılavuz/şartname gereği).

**Yazılım tarafı (ATLAS_Takim sürümünde hazır):** GO pini aktif-LOW okunur; START kenarı → 1 sn ön-vakum → koşu; STOP (sinyalin 5 V'a dönmesi) → 500 ms fren → kilit (reset'e kadar). Ek emniyet: SW1+SW2'ye birlikte basmak her an STOP'tur.

---

## 6. Pil ve türbin (özet — ayrıntı DONANIM_VE_KONNEKTORLER.md'de)

- **2S LiPo, XT30** ana girişe. İlk gün yalnız 2S.
- Türbin, pilin **balans soketinin 2S uçlarından** beslenir; kılavuz s.25'teki SB1 köprüsüne DOKUNMA.
- Şarj: B6 ile LiPo balans modu, düşük akım (300 mAh ise 0.3 A), gözetimli ve yanmaz yüzeyde. Şarj sırasında robot bağlı olmaz.
- Açık kalma: koşu + bekleme dahil toplamda ~45 sn'yi aşma (78M05 ısınması, kılavuz s.27).

---

## 7. Yaygın hatalar ve kontrol listesi

| Hata | Sonuç | Doğrusu |
|---|---|---|
| Normal "Yükle" butonu | "port yok" hatası / yüklemez | Programlayıcı ile Yükle (Ctrl+Shift+U) |
| 10→6 adaptör ters takılması | Programcı robotu görmez | Pin-1 hizala, etiketleri oku |
| USB-TTL 5V'sini robota bağlama + pil takılı | 5V çakışması | UART'ta yalnız TX/RX/GND |
| MEBSTART sinyali RDY'ye takmak | Robot çalışmaz/tuhaf davranır | Sinyal her zaman **GO** |
| Türbinü 3S balansın tamamına bağlamak | Türbin/motor yanar | Yalnız 2S uçları |
| Debug monitörü yanlış baud | Anlamsız karakterler | **115200** ayarla |

**Her oturumun ilk 60 saniyesi:** pil voltajı ölç (2S dolu ≈ 8.4 V), kablolar gözden geçir, sensör şeridi (FFC) yerine oturmuş mu bak, robotu ilk açılışta tekerlekleri havadayken aç.
