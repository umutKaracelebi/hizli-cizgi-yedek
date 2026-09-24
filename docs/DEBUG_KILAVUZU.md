# ATLAS 1.4.3 — donanım debug kılavuzu (veriye dayalı teşhis)

Tarih: 24 Eylül 2026. Bu belge, 24 Eylül koşusunda görülen dört semptomun kök nedenini **ölçümle** bulmak için yazıldı. Kod değişikliği bu belgenin kapsamı dışındadır: önce veri toplanır, sonra karar verilir.

Kurallar:
1. Testler `T0 → T1 → T2 → T3 → T4` sırasıyla yapılır; bir test "geçti" işaretlenmeden sonrakine geçilmez.
2. Ölçülmeyen bir hücre tahminle doldurulmaz; **"ölçülmedi"** yazılır.
3. Robot piste çıkmadan önce `T1` ve `T2` geçmiş olmalıdır.
4. Her testte kullanılan yazılım ve kart (ATLAS_Debug / ATLAS_Takim, ATmega328P / Uno) kaydedilir.
5. Şüpheli bir bağlantı deneyerek doğrulanmaz; multimetreyle veya üretici şemasıyla doğrulanır.

---

## 0. Mevcut veri (24 Eylül koşusu — kullanıcı beyanı, bağımsız ölçüm değil)

| # | Gözlem (beyan) | Kayıtlı olmayan bilgi |
|---|---|---|
| S1 | `ATLAS_Takim` yüklüydü; güç açılışında MEBSTART'a basıldı, hiçbir tepki yok | Robot o anda hangi aşamadaydı (mod bekliyor / ARMED / kalibrasyon)? |
| S2 | SW1'e basıldı, robot çalıştı (kalktı) | SW1 "mod seçimi/kalibrasyon bitirme" miydi, yoksa koşu gerçekten SW1'den sonra mı başladı? Kalibrasyonda hangi LED çaktı? |
| S3 | Düz kesim takip edildi; virajda çizgiden çıktı | Çıkıştan sonra robot kendi kendine durdu mu, durduysa LED deseni ne oldu? Taban PWM kaçtı? |
| S4 | Türbin motoru çalışmadı | Türbin balans soketinden beslendi mi? Türbin konnektöründe gerilim ölçüldü mü? |

**S1 notu (koddan doğrulandı):** `ATLAS_Takim.ino:107-138` akışında güç açılışında robot önce **SW1 veya SW2 basışını** bekler (çizgi modu seçimi + kalibrasyon). MEBSTART bu aşamada **hiç dinlenmez**; yani S1 "arıza" değil, akışın sonucudur. Kalibrasyon bitip `run()` çağrılana kadar MEBSTART'ın START'ı etkisizdir.

---

## 1. Bağlantı haritası (ne, nereye, hangi sırayla)

### 1.1 Pin haritası — `ATLAS_Debug` ve `ATLAS_Takim` aynı karta aynı pinleri kullanır

| İşlev | Arduino pini | ATmega pini | Açıklama |
|---|---|---|---|
| Sol motor yön | D6 | PD6 | `INL1` |
| Sol motor PWM | D9 | PB1 / OC1A | `INL2`, Timer1 |
| Sağ motor yön | D5 | PD5 | `INR1` |
| Sağ motor PWM | D10 | PB2 / OC1B | `INR2`, Timer1 |
| Türbin PWM | D11 | PB3 / OC2A | `PWMC`, Timer2 |
| Motor sürücü inhibit | D12 | PB4 | `INH` (1 = etkin) |
| LED0 | D2 | PD2 | |
| LED1 | D8 | PB0 | |
| LED2 | D13 | PB5 | |
| SW1 (buton 1) | A5 | PC5 | dahili pull-up, aktif-LOW |
| SW2 (buton 2) | D7 | PD7 | dahili pull-up, aktif-LOW |
| MEBSTART sinyal | D4 | PD4 | `GO`, aktif-LOW |
| READY (kullanılmıyor) | D3 | PD3 | `RDY`, boşta |
| Sensör MUX seçim | A0-A3 | PC0-PC3 | 16 sensör, ADC kanal 4 |
| Sensör şeridi | FFC/FPC | — | takılı kalır, sökülmez |

Timer yapılandırması iki sketch'te de aynıdır: `ICR1=399`, faz-doğru PWM, ~20 kHz (sürüş); Timer2 ~31 kHz (türbin). `ATLAS_Debug:125-133`.

### 1.2 Güç ve türbin beslemesi

```text
  [2S LiPo]
     ├── XT30 ana soket ──────────► robot kartı (ana Vs, sürüş motorları)
     └── Balans soketi (JST-XH)
              ├── 2S uçları (toplam ≈7.4–8.4 V) ──► TÜRBİN beslemesi
              └── SB1 köprüsü ──► (kılavuz s.25) DOKUNMA / değiştirme

  Kural: türbin ASLA 3S paketin tamamına bağlanmaz. Yalnız kılavuzun
  gösterdiği 2S uçları kullanılır.
```

- Ölçüm noktaları: paket gerilimi (XT30), hücre gerilimleri (balans uçları), **türbin konnektörü**, SB1 sürekliliği (güç kapalı, multimetrenin süreklilik/uyarı kademesi).
- Bu hattı ölçmeden S4 hakkında karar verilmez.

### 1.2.1 Güç kaynağı: pil mi, USB mi? (en sık sorulan soru)

**Kısa kural:** CH340 / USB-TTL dönüştürücü robota **güç vermez**; o yalnız veri hattıdır (TX/RX/GND). Beslemeyi ya **2S pil (XT30)** ya da **USBasp'in hedef-besleme jumper'ı** sağlar. **İkisi aynı anda asla.**

| Test | 2S pil (XT30) | Neden |
|---|---|---|
| T0 türbin besleme ölçümü | **Zorunlu** | Ölçümün kendisi paket üzerinde yapılır |
| T1 `[1]` buton / MEBSTART | Önerilir (USBasp beslemesiyle de olur) | MEBSTART modülü 5 V'u robottan alır |
| T2 `[2]` sensör | Önerilir | 16 IR LED + MCU akımı USBasp beslemesini zorlayabilir |
| T3 `[3]` tekerlek | **Zorunlu** | Motorlar XT30 ana hattından (Vs) beslenir |
| T4 `[4]` türbin | **Zorunlu** | Türbin, balans soketinin 2S uçlarından beslenir |
| T5 yarış koşusu | **Zorunlu** | Ek olarak ≤45 sn açık kalma kuralı |

Güvenli/tehlikeli kombinasyonlar:

| Durum | USBasp hedef-besleme jumper | USB-TTL 5V pini | Sonuç |
|---|---|---|---|
| **Pil takılı (normal)** | **Açık** | Bağlanmaz | Önerilen; tüm testler yapılabilir |
| Pil yok (masa testi) | Kapalı/etkin (PC'den 5 V) | Bağlanmaz | Yalnız mantık: LED, buton, MEBSTART seviyeleri, sensör okuma. **Motor ve türbin çalışmaz** |
| Pil takılı + USBasp jumper kapalı | ✗ | — | İki 5 V hattı çakışır → **yasak** |
| Pil takılı + USB-TTL 5V bağlı | — | ✗ | **Yasak** |

Proje dokümanlarından birebir:
- `KONNEKTOR_KULLANIM_KILAVUZU.md:53` → "Pil takılıyken bu jumper kapalıysa iki 5 V hattı çakışır. İlk denemede: pil takılı, USBasp hedef-besleme jumper'ı **açık** (varsa). Robot bu sırada kendi piliyle açık olmalı."
- `KONNEKTOR_KULLANIM_KILAVUZU.md:66` → "Robot pilden besleniyorsa USB-TTL'nin 5V pinini robotun 5V'sine **BAĞLAMA**; sadece TX/RX/GND yeter. Robotu yalnız USB-TTL'nin 5V'siyle beslemeye kalkma — motorlar o hattan beslenemez."

Neden USB'den motor/türbin çalışmaz:
1. Sürüş motorları **Vs** hattından (XT30) beslenir; 5 V mantık hattı yalnız MCU/sensör/LED içindir.
2. Türbin, pilin **balans soketinin 2S uçlarından** beslenir; USB tarafında böyle bir hat yok.
3. USB portu ≈500 mA ile sınırlıdır; 3 coreless motor yükte çok daha fazlasını çeker.

Ek uyarılar:
- CH340 üzerindeki jumper **VCC–5V** konumunda kalsın; 3V3'e alınmaz (lojik seviye).
- Bağlantı sırası: **önce robota, sonra USB'ye**.
- USBasp beslemesiyle kararsızlık (kendiliğinden reset, kısık LED, seri kopma) görürsen kaynak yetersizdir → testi **pille** yap.
- Pil ile çalışırken toplam açık kalma ≤45 sn (78M05 ısınması); koşular arası 1–2 dk.

### 1.2.2 Açma/kapama sırası (pilin mi, USB'nin mi önce olduğu sorusu)

**Kural: pil = açma/kapama anahtarı.** XT30 **en son** takılır, **ilk** çıkarılır. Sinyal kabloları (UART, ICSP, MEBSTART, FFC) **enerjisiz** kurulur.

**AÇMA sırası:**
1. Pil ve USB sökülü; tekerlekler havada tutulur.
2. Sinyal kablolarını enerjisiz kur:
   - MEBSTART: S → **GO**, − → −, + → +5V, **RDY boş**;
   - sensör FFC yerine oturmuş;
   - izleme için CH340 + panel adaptör → UART (**yalnız GND/TX/RX**, 5V yok);
   - **CH340 sırası: önce robota, sonra PC'ye.**
3. Seri monitörü aç (115200). Robot henüz kapalı olduğu için çıktı görünmez — bu normaldir (COM portu görünür).
4. **En son pili bağla:** önce **balans/türbin kablosu**, sonra **XT30**. Robot açılır; debug yazılımı yüklüyse **3 LED birlikte 3 kez** çakar, ardından menü.
5. Testleri yap; pil ile toplam açık kalma **≤45 sn**.

**KAPAMA sırası:**
1. **XT30'u çıkar** (robot kapanır) → sonra balans/türbin kablosu.
2. Sonra USB'yi sök, en son adaptörü robottan ayır.

**Yükleme sırasında:** proje kılavuzunun doğrulanmış düzeni → **pil takılı + USBasp hedef-besleme jumper'ı AÇIK**. Pilsiz yüklemek istersen jumper **kapalı/etkin** olur (robotu PC besler). İkisi asla birlikte değil.

**Neden bu sıra:** (i) enerjili karta sinyal fişi takmak, yanlış hizalanmada (bir pin kayması) pin yakabilir; (ii) pil takılı geçen her saniye 45 sn ısı bütçesinden gider; (iii) projenin ilk koşu planı da bu sırayı verir (`ILK_KOSU_PLANI_23EYL.md` §2–3).

### 1.3 USBasp zinciri (firmware yükleme) ve USB-TTL zinciri (seri izleme)

```text
  YÜKLEME : PC(USB) ─ USBasp ─ 10 pin şerit ─ 10→6 adaptör ─ robot ICSP
            (kırmızı tel = pin 1 her iki sokette pin 1'e)

  İZLEME  : PC(USB) ─ CH340 USB-TTL ─ panel adaptör(connector3) ─ robot UART
            TXD → RX, RXD → TX, GND → GND   (5V HATTI BAĞLANMAZ)
```

- Arduino IDE ayarı: **Kart = Arduino Nano, İşlemci = ATmega328P, Programlayıcı = USBasp**; COM portu seçilmez; **Taslak → Programlayıcı ile Yükle (Ctrl+Shift+U)**.
- USBasp üzerindeki hedef-besleme jumper'ı: pil takılıyken **açık** (iki 5 V hattı çakışmasın).
- Seri bağlarken: önce robota, sonra USB'yi PC'ye. Baud **115200**.

### 1.4 MEBSTART bağlantısı

| MEBSTART | Robot start portu |
|---|---|
| Sinyal (S) | **GO** |
| − | − / GND |
| + | + / 5V |
| — | **RDY boşta** |

Bağlamadan önce (güç yokken) multimetreyle **Sinyal–GND** ve **+5V–GND** arasında kısa devre olmadığını doğrula. Modülün IR penceresi robotun üstünde, dışa dönük ve gölgelenmemiş olmalı.

### 1.5 Kırmızı çizgiler (bu testlerde asla yapılmaz)

- Türbini 3S'e veya 5 V regülatör hattına bağlamak.
- Pil takılıyken USB-TTL'nin **5V** pinini robota bağlamak.
- USBasp hedef-besleme jumper'ı kapalıyken pili takılı bırakmak.
- MEBSTART sinyalini RDY'ye takmak veya kutupları deneme-yanılma ile değiştirmek.
- 78M05 ısınma sınırı: açık kalma toplamı **45 sn'yi geçmez**; koşular arası 1–2 dk mola; gövde ısındıysa enerjiyi kes.
- Türbin testinde dönen fan açıklığına erişilmez; motor testinde tekerlekler havada ve el uzakta olur.

---

## 2. Yazılımı yükleme (ATLAS_Debug) — her testin başlangıcı

1. Pil bağlı, tekerlekler havada; USBasp → 10 pin → 10→6 adaptör → ICSP (kırmızı tel pin 1).
2. IDE: `ATLAS_Debug/ATLAS_Debug.ino` aç → Kart `Arduino Nano`, Processor `ATmega328P`, Programmer `USBasp` → **Ctrl+Shift+U**.
3. Beklenen ölçü (kılavuz): flash ≈ **6034 bayt**, SRAM ≈ **204 bayt**. Çok farklıysa yanlış kod derlenmiş; dur.
4. Yükleme sonrası USBasp'i çıkar; seri izleme için CH340 + panel adaptör → UART (GND/TX/RX), 115200 baud, önce robota sonra PC'ye.
5. **Doğru yazılım işareti:** güç verildiğinde **3 LED birlikte 3 kez çakar**. Bu çakma yoksa robota yarış yazılımı yüklü demektir (bu testler için yanlış sketch).

Bilgisayarsız kullanım (seri bağlamadan da çalışır): **SW2 kısa bas = menü seçimi** (LED1=1, LED2=2, LED0=4 → toplamı oku, ör. 3 = LED1+LED2), **SW1 kısa bas = seçileni çalıştır**. Canlı testlerden çıkış: SW1+SW2 birlikte 2 sn (türbin testinde **SW2**).

---

### 2.1 Seri monitör ve COM portu — çıktıyı nerede göreceğim?

**Çıktı, Arduino IDE'nin Seri Monitöründe görünür** — ama COM portu **USBasp'ten değil, CH340 (USB-TTL) dönüştürücüden** gelir.

| Soru | Cevap |
|---|---|
| COM portunu kim oluşturur? | **CH340 (connector2 + panel adaptör)**: IDE'de `COM5 (USB-SERIAL CH340)` gibi görünür |
| USBasp COM portu oluşturur mu? | **Hayır.** USBasp bir programlayıcıdır (WinUSB/libusb cihazı); yükleme yapar, seri port vermez |
| Monitör nerede? | IDE 2.x: sağ üstteki **büyüteç (Serial Monitor)** simgesi. IDE 1.8.x: **Araçlar → Seri Monitör** (Ctrl+Shift+M) |
| Baud? | **115200** (sketch `Serial.begin(115200)`, `ATLAS_Debug.ino:140`) |
| Port seçimi nerede? | IDE **Araçlar → Port**. Bu seçim yalnız izleme içindir; **yükleme ICSP'den yapılır, port kullanmaz** |

**Doğru sıra:** ① USBasp ile yükle → ② USBasp'i sök → ③ CH340'ı **robota**, sonra USB'yi **PC'ye** tak → ④ IDE'de `Port` = CH340'ın COM'u → ⑤ Seri Monitör + **115200** → ⑥ en son pili tak.

**Beklenen çıktı (ATLAS_Debug yüklüyken):**
```text
==========================================
 ATLAS 1.4.3 - TEAM TURQUOISE DONANIM TEST
==========================================

[1] Buton + LED + MEBSTART pinleri (canli)
[2] 16 cizgi sensoru ham degerler (canli)
[3] Tekerlek motorlari testi (TEKERLER HAVADA!)
[4] Turbin testi (duz zemin, el uzakta)
Seri: numara gonder. Buton: SW2=sec, SW1=baslat.
Canli test cikis: SW1+SW2 2sn (turbinde SW2).
```
- `[1]` testinde 150 ms'de bir: `SW1=0 SW2=0 GO(D4)=1(bekleme) RDY(D3)=1`
- `[2]` testinde her örnekte 16 sensörün ham değeri (0..255)
- Komut tuşları: **`1`–`4`** test başlatır, **`0` / `h` / `?`** menüyü yeniden yazar, canlı testlerde **`x`** çıkar (buton alternatifi: SW1+SW2 2 sn). Satır sonu ayarı fark etmez; "No line ending" en temizidir.

**Kritik tuzak:** `ATLAS_Takim` (yarış) sürümünde **`Serial.begin()` yoktur** → o yazılım yüklüyken seri monitör **tamamen boş** kalır. Bu bir kablo/port arızası değildir; yalnız `ATLAS_Debug` çıktı verir.

| Belirti | Muhtemel neden | İlk bakılacak |
|---|---|---|
| Port listesinde CH340 yok | USB kablosu veri desteklemiyor / sürücü / USB takılı değil | Başka USB kablosu; Windows'ta `USB-SERIAL CH340` görünüyor mu |
| Port var, çıktı yok | Robot kapalı (pil yok) veya yarış yazılımı yüklü | Pil tak + 3 LED çakması; `ATLAS_Debug` yükle |
| Anlamsız karakterler | Baud yanlış | **115200** seç |
| Port meşgul / açılmıyor | Başka bir seri terminal portu tutuyor | Diğer programları kapat |
| Tuşa basınca tepki yok | Monitör odakta değil veya yanlış testte | Monitörün giriş kutusuna `1` yazıp Enter; `x` ile çık |

### 2.1.1 "Yalnız COM1 görünüyor / çıktı yok" — adım adım ayrım

**Teşhis cümlesi:** `COM1` neredeyse her zaman bilgisayarın **kendi anakart seri portudur**. Listede başka port yoksa **CH340 hiç tanınmamıştır**; yani sorun IDE ayarı veya baud değil, **aygıt / sürücü / kablo** tarafındadır.

1. **Windows'a sor (en kesin kaynak):** `Aygıt Yöneticisi → Bağlantı Noktaları (COM ve LPT)`, üstteki menüden `Görünüm → Gizli aygıtları göster`.
   - `USB-SERIAL CH340 (COMx)` **varsa** → port var; IDE'de o COM'u seç (115200) ve 6. adıma geç.
   - `USB2.0-Serial` / `Bilinmeyen aygıt` + **sarı ünlem** → sürücü eksik veya yanlış.
   - Hiçbir şey yok ve **tak-çıkarırken liste değişmiyor** → kablo (yalnız şarj kablosu) veya USB girişi.
2. **Sürücü:** WCH **CH340 sürücüsünü** kur (üretici kaynağı: wch.cn). Kurulumdan sonra cihazı çıkar-tak, IDE'de port listesini yenile (IDE 2.x port seçicinin yanındaki yenile simgesi).
3. **Zadig tuzağı:** Zadig **yalnız USBasp** için kullanılır (WinUSB). Yanlışlıkla CH340'a WinUSB kurduysan **COM portu bir daha oluşmaz** → Aygıt Yöneticisi'nde cihaza sağ tık → **Aygıtı kaldır** + *"Bu aygıtın sürücü yazılımını sil"* → tak-çıkar → CH340 sürücüsünü yeniden kur.
4. **Kablo / USB yuvası:** veri destekli kablo kullan; hub olmadan doğrudan bilgisayar portuna tak. Takarken Aygıt Yöneticisi'nde bir şeyin belirip kaybolduğunu izle.
5. **Loopback testi (kesin ayrım — robot bağlı değilken):** adaptörün **TXD ile RXD** pinlerini tek bir jumper kabloyla birbirine bağla → Seri Monitörü 115200'de aç → giriş kutusuna bir şeyler yaz.
   - Yazdıkların ekranda **görünüyorsa**: adaptör + sürücü + port sağlam → sorun **robot tarafında** (6. adım).
   - **Görünmüyorsa**: adaptör / sürücü / kablo / USB yuvası sorunlu.
6. **Robot tarafı kontrol listesi:** pil takılı mı (veya USBasp beslemesi etkin mi), `ATLAS_Debug` yüklü mü (**3 LED × 3 çakma**), kablo **UART** portuna mı takılı (**6 pinli UART** ↔ 6 pinli ICSP ↔ 4 pinli start portunu karıştırma), TX↔RX çapraz, **GND ortak**, CH340 jumper'ı **VCC–5V**, **5V pini bağlı değil**, baud **115200**.

**Seri olmadan da ilerleyebilirsin:** `[1]` testi aynı bilgiyi LED'lerde verir → SW1 basılı = **LED0**, SW2 basılı = **LED1**, MEBSTART START = **LED2**. En kritik veri olan "MEBSTART mandal mı, darbe mi" (T1) **serial olmadan** da alınabilir; `[2]`'nin 16 sayısı serial çözülünce tamamlanır.

## 3. Test protokolü

### T0 — Güç yolu ve türbin beslemesi (multimetre, ~10 dk)

**Amaç:** S4'ün (türbin çalışmadı) donanım mı yazılım mı olduğunu kesinleştirmek. Kod türbini RUN boyunca 200/255 ile sürüyor (`RunControl.ino:188, 223, 294, 351`); bu yüzden önce enerjinin var olduğunu ölçmek gerekir.

| Ölçüm | Nerede | Beklenen | Ölçülen | Not |
|---|---|---|---|---|
| Paket gerilimi | XT30 uçları | 7.4–8.4 V | | |
| Hücre 1 / Hücre 2 | Balans soketi ardışık uçlar | ~4.0–4.2 V / ~4.0–4.2 V | | fark >0.1 V ise not et |
| **Türbin konnektörü + ve −** | Türbin kablosu ucunda (güç açık) | ≈8.0–8.4 V (paketle aynı) | | **Kritik satır** |
| SB1 köprüsü süreklilik | Kart üzeri SB1 (güç kapalı) | Kılavuza göre | | direnç/kopukluk |
| Açık kalma süresi | kronometre | ≤45 sn | | |

**Geçme kriteri:** Türbin konnektöründe ≥8.0 V okunuyor. Okunmuyorsa türbin sorunu **güç yolundadır** (balans kablosu/SB1/konnektör); yazılımda `IMPELLER_PWM` veya Timer2 aranmaz.

### T1 — `[1]` Buton + LED + MEBSTART izleme (canlı)

**Amaç:** (a) MEBSTART çıkışının **mandallı mı, darbeli mi** olduğunu, (b) fiziksel SW1/SW2'nin yazılımdaki karşılığını, (c) LED eşleşmesini kesinleştirmek.

**Neden gerekli:** `ATLAS_Takim` start/stop mantığı "START'ta LOW'da kalan, STOP'ta HIGH'a dönen **mandallı** sinyal" varsayar (`RunControl.ino:140-147, 193-201, 243-248`). Sinyal kısa bir darbe ise ön-vakum (1 sn) 30 ms teyitle **iptal** olur ve robot hiç kalkmaz — S1/S2'nin olası açıklamalarından biri budur.

**Adımlar:** Seri monitörden `1` gönder (veya SW2 ile menüyü 1'e getir, SW1 ile başlat). Seri çıktı 150 ms'de bir şu satırı yazar: `SW1=.. SW2=.. GO(D4)=..(START/bekleme) RDY(D3)=..`

| Adım | İşlem | Kaydedilecek |
|---|---|---|
| T1.1 | Hiçbir şeye dokunmadan 10 sn izle | `GO(D4)` değeri: 1 mi 0 mı? (1 = bekleme) |
| T1.2 | Kumandadan START'a **kısa bas ve bırak** | GO bir an 0 olup 1'e dönüyor mu (darbe), yoksa **0'da kalıyor mu (mandal)**? |
| T1.3 | 5 sn bekle, tekrar bak | GO hâlâ 0 mı? |
| T1.4 | Kumandadan STOP'a bas ve bırak | GO 1'e döndü mü? |
| T1.5 | START'a basılı tut, bırak | (T1.2 ile aynı davranış mı?) |
| T1.6 | SW1'e bas | `SW1=1` olan satır doğru mu? **LED0** yandı mı? |
| T1.7 | SW2'ye bas | `SW2=1` olan satır doğru mu? **LED1** yandı mı? |
| T1.8 | MEBSTART kablosunu (güç kapalı) söküp yeniden tak, enerji ver | GO ne okunuyor? (kopuk kablo davranışı) |

**Geçme kriteri:** T1.2/T1.4 sonucu **tek cümleyle** yazılabilir: "MEBSTART çıkışı (mandallı/darbeli)". Ayrıca T1.6/T1.7 ile fiziksel buton etiketleri yazılımla eşleşmiş olmalı.

**Sonuç yorumu (hazır):**
- **Mandallı** ise yarış yazılımının START/STOP teyit mantığı geçerlidir.
- **Darbeli** ise: START onayı (20 ms) geçer ama PREVACUUM'un ilk 30 ms'inde sinyal HIGH'a döner → iptal → robot hiç kalkmaz; RUN'da da anında STOP. Bu durumda yarış yazılımının başlatma mantığı için düzeltme gerekir (veri T1 sonrası planlanır).

### T2 — `[2]` 16 sensör, ham değerler ve **çizgi kutbu** (canlı)

**Amaç:** (a) 16 kanalın hepsinin çalıştığını, (b) siyah çizgide ham değerin **düşük mü yüksek mi** olduğunu (= yarış yazılımında hangi butonun "siyah çizgi modu" olduğunu), (c) kontrastın yeterli olduğunu ölçmek.

**Neden gerekli:** S3'ün (virajda çıkış) en güçlü adaylarından biri **yanlış çizgi modu**dur. `ATLAS_Takim.ino:119` → `invertSensorReads = readButton_2()`; yani **SW2 = ters (siyah)**, SW1 = ters değil. Yanlış modda robot beyaz alanı "çizgi" sayar: düzde merkez ≈ 7500 olduğu için düz gider, virajda beyazın merkezi dışa kayar → **dışa kaçar**. Gözlemle birebir uyan davranış budur.

**Adımlar:** seri `2` gönder (veya SW2+SW1 ile seç/başlat). Her satırda 16 sayı (0..255) yazılır; LED2 yanıyorsa en az bir sensör 128 üstünde.

| Adım | Yerleştirme | Kaydedilecek (16 değer) |
|---|---|---|
| T2.1 | Bar tamamen **beyaz** zeminde, hareketsiz | 16 sayı (bir satır) |
| T2.2 | Barın **orta sensörleri siyah çizgi üzerinde** | 16 sayı (bir satır) |
| T2.3 | Barı soldan sağa yavaşça gezdir (1-2 sn) | Değişmeyen kolon var mı? |
| T2.4 | Beyaz → siyah geçişi, her kolon | Beyaz değer, siyah değer, fark (kontrast) |

**Geçme kriteri:** 16 kolon da değişiyor (ölü kanal yok) **ve** her kolonda |siyah − beyaz| ≥ 30.

**Karar (hazır):**
- **Siyah çizgide değer DÜŞÜK** → yarış yazılımında **SW2 = siyah mod** doğru çalışıyor (mevcut dokümanla uyumlu).
- **Siyah çizgide değer YÜKSEK** → yarış yazılımında doğru buton **SW1**'dir; README/kılavuzdaki "SW2 = siyah" eşleşmesi bu kart için geçersiz demektir (raporlanmalı, sonra karar).
- Bir kolonda kontrast < 30 ise kalibrasyon kapısı (`Sensors.ino:177-184`) o yüzden reddediyor olabilir; FFC şeridi/oturuş ve sensör yüksekliği kontrol edilir.

> Bilgisayarsız hızlı kontrol: `[2]` testi açıkken robotu çizgi/zemin üzerinde gezdir; **LED2 her geçişte değişiyorsa** dizi canlıdır.

### T3 — `[3]` Tekerlek motorları (yön doğrulaması)

**Amaç:** Sol/sağ ve ileri/geri yönlerinin doğru olduğunu ve yarış yazılımıyla aynı polariteyi kullandığını doğrulamak.

**Güvenlik:** Tekerlekler **havada**, robot sabit, el uzakta. Test, tüm LED'ler hızlı çakarak 8 sn onay bekler; **SW1'i 2 sn basılı tut** (veya seri `y`). Onay gelmezse iptal olur.

| Adım | Beklenen | Kaydedilecek |
|---|---|---|
| T3.1 | LED0 yanarken SOL teker **ileri**, sonra **geri** | yön doğru/ters |
| T3.2 | LED1 yanarken SAĞ teker **ileri**, sonra **geri** | yön doğru/ters |
| T3.3 | PWM seviyesi | test PWM'i 60 (düşük) |

**Not (koddan doğrulandı):** `ATLAS_Debug:215-241` içindeki `setMotorL/setMotorR` polaritesi, `ATLAS_Takim/Motors.ino:91-125` ile **birebir aynıdır** (sağ motorda işaret ters; `INR1=1` + `399-pulse`). Bu yüzden T3'te görülen yön, yarış yazılımında da geçerlidir; yön ters çıkarsa kaydedilir ve `MOTOR_L_SIGN/MOTOR_R_SIGN` ile düzeltme kararı verilir (şimdi değişiklik yapılmaz).

### T4 — `[4]` Türbin (PWM 200, INH beklemede)

**Amaç:** Türbinin sürücü/Timer2 tarafından gerçekten sürülebildiğini görmek ve T0 ile birlikte S4'ü ikiye ayırmak.

**Adımlar:** Robot düz ve temiz zeminde; fan ağzı açık, el uzakta. Seri `4` veya butonla seç → **SW1 (veya `t`) aç/kapa**, LED2 durumu gösterir, **SW2 (veya `x`) çıkış**.

| Adım | Gözlem | Kaydedilecek |
|---|---|---|
| T4.1 | Türbin dönüyor mu? | evet / hayır |
| T4.2 | Avuçla hissedilen emiş | var / yok / zayıf |
| T4.3 | Ses/akım/titreşim | not |
| T4.4 | Açıkken paket gerilimi (varsa) | V |

**Karar (T0 ile birlikte):**

| T0 (türbin beslemesi) | T4 (PWM ile dönüş) | Sonuç | Sonraki adım |
|---|---|---|---|
| Gerilim **yok** | (dönmez) | **Güç yolu sorunu** | Balans soketi / SB1 / türbin konnektörü düzeltilir |
| Gerilim **var** | **Dönüyor** | Türbin donanımı sağlam | Neden yarış yazılımında dönmedi? → T5'te `INH`/ön-vakum sırası izlenir |
| Gerilim **var** | **Dönmüyor** | Sürücü/pin (D11→OC2A) veya türbin motoru | `[4]`'te LED2 yanıyor ama dönmüyor ise pin/sürücü ölçülür |

**Teknik not:** `ATLAS_Debug` türbin testinde `INH` **LOW** (sürücüler beklemede) ve PWM doğrudan `OCR2A`'ya yazılır (`:121, 243-245`). Yarış yazılımı da RUN'da `setPWM_Impeller` ile aynı yolu kullanır, ancak **ön-vakum aşamasında `INH` hâlâ LOW**'dur (RUN'a girişte `enableMotorDrivers()` çağrılır). Bu nedenle "T4'te dönüyor, yarışta dönmüyor" çıkarsa ilk şüpheli bu sıralama/zamanlamadır.

### T5 — Yarış yazılımı (`ATLAS_Takim`) ile kontrollü koşu

**Ön koşul:** T0'da türbin beslemesi doğrulandı, T1 ve T2 geçti, T3/T4 sonuçları kayıtlı.

**Adımlar ve kayıt:**

| # | İşlem | Kaydedilecek |
|---|---|---|
| T5.1 | `ATLAS_Takim` yükle (flash ≈ **6518**, SRAM ≈ **291**) | IDE çıktısı |
| T5.2 | Tekerlekler havada; **T2'nin sonucuna göre doğru buton** ile mod seç (beklenen: SW2 = siyah) | Hangi butona basıldı? |
| T5.3 | Kalibrasyon sırasında hangi LED çakıyor? → `Sensors.ino:152-154`: **LED0+LED1** = ters (siyah mod); **yalnız LED2** = ters değil (beyaz mod) | LED deseni (mod kanıtı) |
| T5.4 | Robotu çizgi üzerinde 3-4 kez soldan sağa salla, butonla bitir | Kapı geçti mi? Geçmediyse LED0/LED1 hızlı çakma = kalibrasyon hatası |
| T5.5 | ARMED: LED0 0.5 sn'de bir yanıyor mu? SW1 ile **PWM 60**'a düş | ARMED görüldü mü, PWM kaç |
| T5.6 | ARMED'da MEBSTART START ver | Önce **1 sn türbin rampası** (LED2 hızlı çakma), sonra tekerlekler |
| T5.7 | Koşu (PWM 60), ilk 1-2 tur, video | Video dosya adı |
| T5.8 | Kaçış/tur sonu | Robot **kendi kendine mi durdu** (çizgi kaybından ~0.4–0.9 sn sonra; `RunControl.ino:311` failsafe) yoksa STOP/kilit ile mi? |
| T5.9 | Durduktan sonraki LED deseni | **LED0+LED1 1 sn arayla** = kilit (reset gerekir, `RunControl.ino:364-371`) |
| T5.10 | Süre/sıcaklık | Açık kalma ≤45 sn, gövde sıcak mı |

**Yorum rehberi:**
- T5.3'te **yalnız LED2** çaktıysa: o koşuda **beyaz (yanlış) mod** seçilmişti → S3'ün nedeni büyük olasılıkla bu; tekrar denemede SW2 kullanılır.
- LED0+LED1 çaktıysa mod doğruydu; S3 için sıradaki şüpheliler: emişsiz koşu (T4), PWM 100 (yüksek), failsafe kilidi.
- T5.6'da 1 sn rampa görülmediyse ya ön-vakum iptal edildi (T1'de "darbeli" çıkarsa beklenen) ya da yazılım yüklü değil.
- T5.8/T5.9 failsafe-kilit yolunu doğrular; bu davranış "kaçış" gibi görünür ama aslında robotun kendini güvenli duruşa aldığı andır.

---

## 4. Veri formu — ölçümlerden sonra bu formatta gönder

```text
T0 GÜÇ
  paket: ___ V | hücre1: ___ V | hücre2: ___ V
  türbin konnektörü: ___ V | SB1: ___ (süreklilik/kopuk)

T1 MEBSTART (tek cümle)
  çıkış: [mandallı / darbeli]  | STOP'ta 1'e dönüyor mu: [evet/hayır]
  SW1 → seri SW1=1 mi: [ ] LED0 yandı mı: [ ]
  SW2 → seri SW2=1 mi: [ ] LED1 yandı mı: [ ]
  RDY(D3) boşta: ___

T2 SENSÖR (16 sayı, boşlukla)
  beyaz : __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __
  siyah : __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __
  ölü kolon: ___ | min kontrast: ___
  KUTUP: siyah çizgide değer [düşük / yüksek]

T3 MOTOR
  sol ileri/geri: [doğru/ters] | sağ ileri/geri: [doğru/ters]

T4 TÜRBİN
  dönüyor: [evet/hayır] | emiş: [var/yok/zayıf]

T5 YARIŞ KOŞUSU
  seçilen buton: ___ | kalibrasyon LED'i: [LED0+LED1 / yalnız LED2]
  PWM: ___ | START sonrası 1 sn rampa: [görüldü/görülmedi]
  çıkış: [kendiliğinden durdu / STOP / hiç durmadı] | LED deseni: ___
  video: ___
```

> Kural: ölçülmeyen satıra **"ölçülmedi"** yaz; tahminle doldurma. Yanlış bir ölçüm, yanlış bir yazılım kararına dönüşür.

---

## 5. Karar matrisi — veri geldikten sonra hangi yola gidilir

| Semptom | Test | Beklenen/kritik veri | Veri şu ise → karar |
|---|---|---|---|
| S1 MEBSTART tepkisiz | T1 | GO seviyesi ve mandal/darbe | **Mandal**: akış doğru, sorun "önce mod butonu" idi (kullanım/prosedür). **Darbe**: yarış yazılımının START/STOP teyit mantığı bu modülle uyumsuz → başlatma mantığı için düzeltme planlanır (`RunControl.ino:140-147, 193-201`) |
| S2 SW1 ile kalkış | T1 + T5.2/T5.3 | Fiziksel buton ↔ yazılım eşleşmesi, kalibrasyon LED'i | Butonlar ters ise: doğru modu T2 kutup ölçümü belirler. Kalibrasyonda yalnız LED2 çaktıysa yanlış (beyaz) mod seçilmişti |
| S3 virajda çıkış | T2 + T5.5–T5.9 | Çizgi kutbu, PWM, durma şekli | Yanlış mod → doğru butonla tekrar. Mod doğru + emiş yok → önce türbin (T0/T4). Mod doğru + emiş var → PWM 60 ve kilit davranışı; failsafe ise `RunControl.ino:311` ayarı tartışılır |
| S4 türbin çalışmadı | T0 + T4 | Türbin konnektörü gerilimi, PWM ile dönüş | Gerilim yok → güç yolu (balans/SB1/konnektör). Gerilim var + dönüyor → yarış yazılımında sıralama/`INH`. Gerilim var + dönmüyor → sürücü/pin (D11/OC2A) veya motor |
| (yeni) düz çizgide takip zayıf | T2 | Kontrast, ölü kanal | Kontrast < 30 veya ölü kanal → sensor/FFC/yükseklik düzeltmesi; yazılım ayarına geçmeden |

**Bilinen riskli kod noktaları (statik analiz — testle doğrulanmayı bekliyor):**

| Kod | Bulgu | Hangi test doğrular |
|---|---|---|
| `RunControl.ino:272` | Uç sensörde (`position == 0` veya `15000`) çizgi "yok" sayılıyor → kurtarma/failsafe yolu | T5.8 (kaçış/kilit) |
| `RunControl.ino:311-328` | 400 ms çizgi kaybı → 500 ms **ileri** yavaşlama rampası → kilit; kurtarmaya ~305 ms kalıyor | T5.8 (kendiliğinden durup kilitleme) |
| `RunControl.ino:158-172` | ARMED'da hız butonu basılıyken START onayı sıfırlanıyor (500 ms bloklama) | T5.5/T5.6 (START anı) |
| `ATLAS_Takim.ino:107-121` | MEBSTART ancak mod butonundan sonra dinleniyor | T1 + T5.2 |
| `Motors.ino` + `Sensors.ino` | Aktif fren yok; 16 bloklayan ADC okuması 750 µs bütçesini dolduruyor | T5.7 (hız/kararlılık), istenirse pin toggle ölçümü |

---

## 6. Güvenlik özeti (her test öncesi 10 saniye)

1. Tekerlekler **havada** + el uzakta (T3); türbin **düz, temiz zemin**, fan ağzı açık (T4).
2. Açık kalma toplamı **≤45 sn**; koşular arası **1–2 dk**; gövde ısındıysa enerjiyi kes.
3. Türbin **yalnız 2S**; 3S paketin tamamı türbine verilmez, SB1 değiştirilmez.
4. Pil takılıyken USB-TTL'nin **5V** hattı bağlanmaz; bağlantı önce robota, sonra PC'ye.
5. LiPo: şişmiş/hasarlı pil kullanılmaz; şarj gözetimli, yanmaz yüzeyde, robot bağlı değilken.
6. Koşuda robotu **elle yakalamaya çalışma**; kaçış yönünde boş ve zararsız alan bırak.

---

## 7. Kapsam, kanıt düzeyi ve sınırlar

- Bu belge **yalnızca ölçüm planıdır**; hiçbir kaynak dosya (`ATLAS_Takim`, `ATLAS_Debug`, `Sensors/Motors/UI`) değiştirilmemiştir.
- Koddan yapılan çıkarımlar statik okumaya dayanır (satır numaraları verilmiştir); fiziksel doğrulama yerine geçmez.
- Derleme ölçüleri (6518/291 ve 6034/204) mevcut proje belgelerinden alınmıştır; bu oturumda yeniden derleme yapılmamıştır.
- `ATLAS_Debug` içindeki motor polaritesi ve Timer yapılandırmasının yarış yazılımıyla aynı olduğu **koddan doğrulanmıştır** (`ATLAS_Debug:125-133, 215-241` ↔ `ATLAS_Takim/Motors.ino:54-66, 91-125`).
- Testler sonucunda hangi davranışın düzeltileceği, `DOGRULAMA_LISTESI.md` kurallarına uygun olarak **ölçüm kaydıyla** birlikte karara bağlanır; tahminle yazılım değişikliği yapılmaz.




