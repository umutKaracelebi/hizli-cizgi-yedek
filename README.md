# Team Turquoise — ATLAS 1.4.3 Hızlı Çizgi İzleyen Robot

Bu depo, Ödemiş Ayhan Kökmen Fen Lisesi robotik takımı **Team Turquoise** tarafından yürütülen hızlı çizgi izleyen robot projesinin tüm kaynak kodlarını, donanım mimarisini, test protokollerini, tur kronometresini ve teknik yarışma dokümantasyonunu içerir.

---

## 1. Donanım Mimarisi ve Bileşenler

Robot platformu, Meksika menşeli **ATLAS Rev. 1.4.3** (Exotic Team MX) şasisi ve kart tasarımı üzerine kuruludur.

| Alt Sistem | Bileşen / Parametre | Teknik Not |
|---|---|---|
| Mikrodenetleyici | Microchip ATmega328P-AU | 16 MHz harici kristal, 5 V lojik |
| Çizgi Sensörü | 16 × Vishay QRE1113GR | Analog multiplexer (PORTC 0..3), ADC kanal 4, sola hizalı 8-bit okuma (`ADLAR=1`) |
| Motor Sürücüler | 2 × Infineon BTN9960LV | H-Köprüsü, `INH` pini D12 (bekleme/etkinleştirme), Timer1 PWM |
| Sürüş Motorları | 2 × Coreless 1023 (7.4 V) | Tersine çevrilebilir sürüş, ~20 kHz PWM frekansı |
| Emiş Türbini | 1 × Coreless 1023 (7.4 V) + Fan | Timer2 PWM (~31 kHz), zemine vakum tutunması |
| Ana Güç Girişi | XT30 Konnektör | 2S LiPo batarya (nominal 7.4 V, tam dolu 8.4 V) |
| Türbin Beslemesi | Pil Balans Soketi (JST-XH) | Yalnızca 2S hücre gerilimi; asla 3S doğrudan bağlanmaz |
| Başlatma Modülü | MEBSTART (Robotus) | 3 pinli (Sinyal, GND, +5V), aktif-LOW (0 V tetik, 5 V bekleme) |
| Programlama Arayüzü | 6 pin ICSP Header | USBasp v2.0 (10 pin şerit + 10→6 pin adaptör) |
| Seri Telemetri / Tanı | 6 pin UART Header | CH340 USB-C TTL dönüştürücü (115200 baud, yalnızca TX/RX/GND) |
| Kullanıcı Arayüzü | 2 Buton + 3 LED | SW1 (PC5/A5), SW2 (PD7/D7); LED0 (PD2), LED1 (PB0/D8), LED2 (PB5/D13) |

---

## 2. Depo Dizin Haritası

```text
├── ATLAS_1.4.3.ino           # Üretici orijinal temel sketch'i (ellenmemiş referans)
├── Motors.ino                 # Üretici motor sürücü fonksiyonları ve Timer1/2 tanımları
├── RunControl.ino             # Üretici ana koşu ve offline kontrol döngüsü
├── Sensors.ino                # Üretici ADC MUX okuma ve kalibrasyon rutini
├── UI.ino                     # Üretici LED ve buton port işlemleri
├── ATLAS_Takim/               # [YARIŞMA SÜRÜMÜ] Optimize edilmiş 5 sekmeli ana kod
│   ├── ATLAS_Takim.ino        # Parametreler, kurulum ve çizgi modu/kalibrasyon kapısı
│   ├── Motors.ino             # BTN9960LV ve Timer yapılandırması
│   ├── RunControl.ino         # MEBSTART durum makinesi, P0 düzeltmesi, kesik köprüsü
│   ├── Sensors.ino            # MUX ADC okuma ve kontrast doğrulama kapısı
│   └── UI.ino                 # Dahili pull-up'lar ve MEBSTART aktif-LOW sinyal mantığı
├── ATLAS_Debug/               # [DONANIM DOĞRULAMA] Bağımsız, hafif tanı sketch'i
│   └── ATLAS_Debug.ino        # String'siz, seri ve bilgisayarsız LED+buton menülü test aracı
├── ATLAS_MEBSTART_Test/       # Arduino Uno tabanlı izole LED durum makinesi testi
│   └── ATLAS_MEBSTART_Test.ino
├── hizli-cizgi-kronometre/    # Bağımsız tur kronometresi alt sistemi
│   ├── robot_kronometre/      # Uno + MZ80 IR engel sensörü + Waveshare 2.4" SPI TFT
│   └── tests/                 # TimingCore masaüstü C++ birim testleri
├── docs/                      # Detaylı teknik kılavuzlar ve inceleme raporları
│   ├── KONNEKTOR_KULLANIM_KILAVUZU.md # Soket eşleştirmeleri, bağlantı ve elektriksel güvenlik
│   ├── ILK_KOSU_PLANI_23EYL.md        # Saha koşu protokolü, sorun giderme ve karar matrisi
│   ├── TEKNIK_INCELEME.md             # Firmware analizi, açıklar ve kontrol döngüsü matematiği
│   ├── DONANIM_VE_KONNEKTORLER.md     # Batarya, şarj, türbin ve arayüz elektriksel analizi
│   ├── TEST_VE_YARISMA_PLANI.md       # Sezonluk test ajandası ve veri kayıt şablonu
│   └── DOGRULAMA_LISTESI.md           # Açık/kapalı mühendislik doğrulamaları takip tablosu
├── sartnameler/               # Yarışma kural kitapçıkları (Robotex, Fibonacci v1.1)
├── connector-photos/          # USBasp, CH340 ve panel adaptörü doğrulama fotoğrafları
├── pist1.pdf, pist2.pdf       # 200 x 280 cm ve 200 x 300 cm yarışma pist vektörleri
├── NASIL_YUKLENIR.txt         # Okul veya harici bilgisayarlar için hızlı flash kılavuzu
└── ATLAS_YARIS_PAKETI_23EYL.zip # Sıkıştırılmış taşınabilir flash paketi
```

---

## 3. Firmware Sürümleri ve Farklar

### 3.1. Üretici Orijinal Kodu (`ATLAS_1.4.3.ino`)
- Sinyal mantığı: Aktif-HIGH (READY=D3, GO=D4).
- Çizgi kaybı hatası: `RunControl.ino` 205. satırda `const unsigned long offlineElapsedTime = currentTime - offlineElapsedTime;` ifadesi tanımsız davranış üretir (`-Wmaybe-uninitialized`).
- Bellek: `debugMode()` fonksiyonundaki metinler dinamik belleği şişirir (SRAM 1557 / 2048 B, %76 doluluk).
- MEBSTART uyumsuzluğu: 5 V bekleme sinyali doğrudan start olarak yorumlanır.

### 3.2. Takım Yarışma Sürümü (`ATLAS_Takim/`)
- **P0 Hata Düzeltmesi:** `offlineElapsedTime = currentTime - offlineStartTime` yapılarak çizgi arama süresi deterministik kılındı.
- **MEBSTART Entegrasyonu:** D4 pini aktif-LOW durum makinesine bağlandı. Sinyal beklemede 5 V (HIGH), START komutunda 0 V (LOW).
- **Ön-Vakum Sıralaması:** START komutu algılandığında motorlar dururken türbin 1000 ms rampayla (`START_IMPELLER_RAMP_TIME_MS`) tam basınca ulaşır, ardından sürüş başlar. Ön-vakum sırasında STOP gelirse robot kalkmadan güvenle durur.
- **Kesikli Çizgi Köprüsü (`OFFLINE_GAP_BRIDGE_MS = 60`):** Çizgi kaybedildiğinde ilk 60 ms boyunca robot son hızla düz devam eder; kesik bölgeyi atlar. Süre aşılırsa üretici arama manevrasına geçer.
- **Güvenlik Koruması (`OFFLINE_FAILSAFE_MS = 400`):** 400 ms boyunca çizgi bulunamazsa robot kendi kendine 500 ms frenleme rampasıyla durur ve kilitlenir.
- **Kalibrasyon Doğrulama Kapısı (`CAL_MIN_CONTRAST = 30`):** 16 sensörden herhangi biri açık/koyu arasında yeterli kontrast göremezse koşu başlatılmaz (LED0/LED1 hızlı çakar).
- **Yüksek Bellek Kazancı:** Statik `debugMode()` kaldırılarak flash 6518 B'a, SRAM kullanımı **291 B'a (%14)** düşürüldü.
- **Acil Durdurma:** Koşu sırasında SW1+SW2 butonlarına birlikte basıldığında donanımsal acil duruş devreye girer.

### 3.3. Donanım Doğrulama Aracı (`ATLAS_Debug/`)
Yarışma kodu yüklenmeden önce tüm alt sistemleri tek tek test eden bağımsız sketch (SRAM 204 B).

Çift kullanım modu destekler:
1. **Bilgisayarla (Seri Monitör 115200 baud):**
   - `[1]`: Buton, LED ve MEBSTART canlı sinyal okuma
   - `[2]`: 16 sensör ham ADC değer akışı (0..255)
   - `[3]`: Tekerlek motor yön/PWM testi (PWM 60, sol/sağ ileri/geri)
   - `[4]`: Türbin aç/kapa testi (PWM 200)
2. **Bilgisayarsız (Sahada / Pilde):**
   - Açılışta 3 LED eşzamanlı 3 kez çakar.
   - **SW2 kısa bas:** Test numarasını değiştirir (1..4 döngüsü). Seçim LED'lerde ikilik (binary) gösterilir: `LED1=1`, `LED2=2`, `LED0=4`.
   - **SW1 kısa bas:** Seçilen testi çalıştırır.
   - Canlı testlerden çıkış: **SW1 + SW2 butonlarına 2 saniye basılı tutun** (Türbin testinde SW2 doğrudan kapatır ve çıkar).
   - Motor testi emniyeti: Seçildikten sonra LED'ler hızla çakar; başlatmak için **SW1'e 2 saniye basılı tutularak** fiziksel teyit verilir.

---

## 4. Yazılım Yükleme Prosedürü

**Kritik Kural:** Arduino IDE'de bir sketch, klasördeki **tüm `.ino` dosyalarının birleşimidir**. Asla yalnızca `ATLAS_Takim.ino` dosyasını tek başına kopyalayıp yüklemeyin.

1. Bilgisayara **USBasp** sürücüsünü kurun (Zadig aracılığıyla `WinUSB v6.1.7600.16385`).
2. Arduino IDE'yi açın: `Dosya -> Aç -> ATLAS_Takim/ATLAS_Takim.ino`.
3. IDE üst sekme çubuğunda 5 sekmenin de açık olduğunu doğrulayın:
   `ATLAS_Takim` | `Motors` | `RunControl` | `Sensors` | `UI`
4. IDE Araçlar menüsünü yapılandırın:
   - **Kart:** `Arduino Nano`
   - **İşlemci:** `ATmega328P`
   - **Programlayıcı:** `USBasp`
   - *COM Port seçilmez (ICSP yüklemesinde port kullanılmaz).*
5. Robotu bağlayın: PC → USBasp → 10 pin şerit → 10→6 pin adaptör → Robot ICSP Header.
   - Kırmızı şerit telini pin-1 ile hizalayın.
   - Robot pilini takın. USBasp üzerindeki hedef-besleme jumper'ı açık olmalıdır.
6. **Taslak -> Programlayıcı ile Yükle** (`Ctrl + Shift + U`) seçeneğine basın.
7. Yükleme 5–15 saniye sürer ve `Done uploading` çıktısı alınır.

---

## 5. Sahada Çalıştırma ve Kalibrasyon Adımları

Yarışma pistimiz **beyaz zemin üzerine siyah çizgi** geometrisine sahiptir (Fibonacci / Robotex standartları).

```text
[Robot Başlangıç Çizgisinde]
         │
         ▼
[Pili Tak / Güç Ver] ──► LED2 açılış animasyonu (1 sn)
         │
         ▼
[Çizgi Modu Seçimi] ──► **SW2 BUTONUNA BASIN** (Siyah çizgi modu)
         │               (DİKKAT: SW1 beyaz çizgi modudur, basmayın)
         ▼
[Sensör Kalibrasyonu] ──► Robotu çizgi üzerinde 3-4 kez sağa-sola gezdirin
         │               (16 sensör de hem beyazı hem siyahı görmelidir)
         ▼
[Kalibrasyonu Bitir] ──► Herhangi bir butona basın (SW1 veya SW2)
         │
         ▼
[Kontrast Kontrolü] ──► BAŞARISIZ İSE: LED0/LED1 hızlı çakar ve kilitlenir (Resetleyin)
         │              BAŞARILI İSE: ARMED durumuna geçer (LED0 yavaş çakar)
         ▼
[Hız Ayarı (İsteğe Bağlı)] ──► SW1: Hız azaltır (-10 PWM), SW2: Hız artırır (+10 PWM)
         │                    (İlk denemelerde taban hız PWM 60-80 önerilir)
         ▼
[Yarışma Başlatma] ──► Hakem kumandasından START sinyali verilir
         │
         ▼
[Ön-Vakum Aşaması] ──► 1 saniye türbin hızlanır (LED2 hızlı çakar, tekerlekler durur)
         │
         ▼
[Otonom Koşu] ──► 750 µs PD döngüsü devrede, robot çizgiyi izler
         │
         ▼
[Durdurma] ──► Hakem STOP sinyali VEYA robot üzerinden SW1+SW2
         │
         ▼
[Frenleme & Kilit] ──► 500 ms dinamik frenleme rampa sonu motorlar kapanır
                       LED'ler 1 sn aralıkla çakar (Yeniden başlatmak için RESET tuşu)
```

---

## 6. Kritik Güvenlik ve Donanım Kuralları

1. **78M05 Regülatör Isınma Sınırı:** Robot açıkken 5 V regülatörü ısınır. Test ve koşu süresi toplamda **kesintisiz 45 saniyeyi geçmemelidir**. Koşular arasında 1–2 dakika soğuma molası verin.
2. **Türbin Gerilim Sınırı:** Emiş türbini yalnızca 2S (nominal 7.4 V, azami 8.4 V) gerilime dayanıklıdır. 3S batarya kullanılırsa türbin motoru yalnızca ilk iki hücrenin balans ucundan beslenmeli, şase üzerindeki `SB1` köprüsü kesinlikle kapatılmamalıdır.
3. **USB-TTL Bağlantı Kuralı:** Seri monitör veya debug için CH340 adaptörü takılırken robot pilden besleniyorsa **+5V pini robota bağlanmaz**. Yalnızca `GND`, `TXD` ve `RXD` hatları bağlanır (adaptör TXD → robot RX, RXD → robot TX). Bağlantı sırası: önce robota takın, ardından USB fişini bilgisayara takın.
4. **MEBSTART Bağlantısı:**
   - MEBSTART Sinyal → Robot **GO** pini
   - MEBSTART (-) → Robot **GND** pini
   - MEBSTART (+) → Robot **5V** pini
   - Robot **RDY** pini boşta bırakılır.

---

## 7. Lisans ve Kullanım Hakları

- **Orijinal ATLAS Rev 1.4.3 Firmware ve Donanım Tasarımı:**
  © 2026 EXOTIC TEAM MX. Creative Commons **Attribution-NonCommercial-NoDerivatives 4.0 International (CC BY-NC-ND 4.0)** lisansı altındadır.
  Ticari olarak kullanılamaz; üreticinin orijinal dosyaları ve değiştirilmiş türevleri üreticinin yazılı izni olmadan kamuya açık şekilde dağıtılamaz.
- **Team Turquoise Geliştirmeleri:**
  Bu depodaki MEBSTART durum makinesi, donanım tanı araçları (`ATLAS_Debug`), kronometre firmware'i ve analiz dokümantasyonu Team Turquoise üyeleri tarafından takım içi yarışma hazırlığı amacıyla geliştirilmiştir.
