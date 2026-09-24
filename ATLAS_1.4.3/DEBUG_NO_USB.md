# ATLAS 1.4.3 — Debug modunu USB-TTL olmadan çalıştırmak

## 1. Debug modu nasıl devreye giriyor?

`ATLAS_1.4.3.ino` → `setup()`:

```cpp
// debug mode trigger
if (DEBUG_MODE_ALWAYS_ON || readButton_1() || readButton_2()) {
  confirmAnimation(500, 1);
  debugMode();          // veya debugModeNoSerial()
}
```

* **Giriş koşulu:** güç verilirken / reset anında **SW1 (PC5)** veya **SW2 (PD7)** basılı olmalı.
  (Kart robot üzerinde montajlıysa ilgili pini boot anında GND'ye çekmek yeterlidir.)
* **Orijinal `debugMode()` tamamen seri port üzerinden çalışır:** `Serial.begin(115200)`,
  TX = **PD1 (Arduino pin 1)**, RX = **PD0 (pin 0)**, 115200 8N1.
  Menü seçimi **robot üzerindeki butonlarla** yapılır (SW2 = değiştir, SW1 = onayla);
  PC sadece **metni okur**.

Yani: *debug moduna girmek* için seri port gerekmez, *çıktıyı görmek* için gerekir.
USB-TTL adaptörün tek işlevi bu metni PC'ye taşımaktır. Firmware'de PD0/PD1 başka hiçbir
işlevde kullanılmıyor, dolayısıyla alternatif yollar açıktır.

## 2. Kodu nasıl yüklüyorsunuz? (karar tablosu)

| Yükleme yolu | Debug çıktısını nasıl okursunuz | Ek donanım |
|---|---|---|
| Kartın kendi USB'si / USB-serial köprüsü (CH340, FT232, ATmega16U2) | Aynı USB kablosu + Arduino IDE **Serial Monitor** (115200) | **Hiç yok** — USB-TTL almaya gerek yok |
| USBasp / USBtinyISP / "Arduino as ISP" (6 pinli ISP header) | **EEPROM trace modu**: `DEBUG_MODE_USE_SERIAL 0` + `tools\dump_eeprom.ps1` | ISP programlayıcı (zaten var) |
| Boşta duran bir Arduino Uno/Nano | Onu USB-UART köprüsü yapın (bkz. §6) | 3 kablo |
| Hiç PC yok / saha testi | LED testleri (`displayNumber()`) | Yok |

## 3. Senaryo A — Kartın USB'si varsa (kod değişikliği YOK)

1. Arduino IDE → **Board:** Arduino Uno / Pro Mini (ATmega328P, 16 MHz), **Port:** kartın COM portu.
2. **Upload**.
3. **Tools → Serial Monitor**, baud **115200**.
4. Kartı resetlerken / enerji verirken **SW1 veya SW2'yi basılı tutun**, menü akar:

```
======================================================
 ATLAS REV 1.4.3    Copyright (C) 2026 EXOTIC TEAM MX
======================================================
[ TEST MODE ENABLED ]
[0] WHEELS MOTORS TEST
[1] IMPELLER MOTOR TEST
[2] SENSORS TEST
[3] UI TEST
[4] YOUR TEST NAME
SELECT OPERATION (CONFIRM [1] / CHANGE [2])
> 0
```

> Ayrı USB-TTL adaptörüne gerek kalmaz: Arduino IDE'nin Serial Monitor'ü zaten USB↔UART
> köprüsünü kullanır. Harici adaptör sadece kartın kendi USB'si **yoksa** gerekir.

## 4. Senaryo B — Sadece ISP programlayıcı varsa (bu repoya eklenen çözüm)

`TraceMode.ino` + `ATLAS_1.4.3.ino` içindeki bayraklarla, seri port olmadan test yapılır:
ölçümler ATmega328P'nin **dahili 1024 byte EEPROM'una** yazılır, sonra **kodu yüklediğiniz
aynı programlayıcı** ile geri okunur.

### 4.1 Firmware'i hazırla

`ATLAS_1.4.3.ino` (USER PARAMETERS altındaki yeni bölüm):

```cpp
#define DEBUG_MODE_ALWAYS_ON      0   // 1 = buton basmadan her açılışta debug moduna gir
#define DEBUG_MODE_USE_SERIAL     0   // 0 = EEPROM trace (USB-TTL yok)  <-- bu senaryo
#define DEBUG_MODE_START_OPERATION 1  // açılışta menüde ön-seçili işlem (0-7)

// tribün PWM seviyeleri (LED_1 blink sayısı = seviye), rampa ve güç izi uzunlukları
#define IMPELLER_TEST_PWM_1 50   /  _PWM_2 100  /  _PWM_3 150  /  _PWM_4 200
```

* `DEBUG_MODE_USE_SERIAL 0` → `debugModeNoSerial()` derlenir, `debugMode()` çağrılmaz
  (linker'ın `--gc-sections`'ı ile seri konsol kodu tamamen çıkar).
* Yükleme: Arduino IDE → **Tools → Programmer: USBasp** → **Sketch → Upload Using Programmer**
  (Ctrl+Shift+U). ISP bağlantısı: MISO/MOSI/SCK/RESET/VCC/GND.

### 4.2 Testi çalıştır (butonlar + LED) — PC'siz saha kullanımı

| İşlem | LED geri bildirimi |
|---|---|
| Seçim numarası | 3 LED sayıyı ikili olarak gösterir (0–7, `displayNumber()`) |
| Debug moduna girerken | Okunmamış bir log duruyorsa **LED0 kısa süre yanar** (yeni test onun üzerine yazacak) |
| SW2 | numarayı artır / 7'den sonra 0'a döner |
| SW1 | seçimi onayla (2 kez blink) |
| EEPROM testi bitti | 3 LED 3 kez yanıp söner, sonra **LED2 sabit yanar** → butona basınca menüye döner |
| İç testlerden çıkış | **SW1 + SW2 birlikte 500 ms** basılı tutulur |

Test listesi (menü 0–7):

| No | Test | PC gerekir mi? | Kaynak |
|---|---|---|---|
| 0 | WHEELS MOTOR TEST — LED0/LED1 sırayla tekerlek başına | Hayır | `motorTest()` (orijinal) |
| 1 | **TRİBÜN / IMPELLER TEST** — LED dilli fan testi + otomatik EEPROM güç izi | **Hayır** | aşağıda §4.4 |
| 2 | SENSOR TEST — kalibrasyon + canlı LED gösterge | Hayır | aşağıda §4.5 |
| 3 | UI TEST — LED'ler SW1/SW2/READY/GO | Hayır | aşağıda §4.6 |
| 4 | EEPROM: kalibrasyon anlık görüntüsü | Sonra okumak için evet | MAX + MIN + TH (3 × 16 byte) |
| 5 | EEPROM: ham sensör izi | Sonra okumak için evet | 16 sensör × 63 kayıt, 50 ms aralık (~3,1 s) |
| 6 | EEPROM: kalibre sensör izi (önce kalibrasyon) | Sonra okumak için evet | 16 sensör × 63 kayıt, 50 ms aralık |
| 7 | EEPROM: çizgi konum izi (önce kalibrasyon) | Sonra okumak için evet | konum (0–15000) + online flag × 300 kayıt, 10 ms aralık (3 s) |

> Tribün testi (No 1) çalışırken güç izi **otomatik** yazılır: tribün PWM + ADC6 + ADC7,
> 337 kayıt × 20 ms ≈ 6,7 s. Fan her açılışında (SW2) pencere baştan başlar.

### 4.4 Tribün (emme fanı) testi — LED dili ve adım adım prosedür

LED haritası (`UI.ino`): **LED_0 = PD2 · LED_1 = PB0 · LED_2 = PB5** (orijinal firmware'de
LED_2 impeller göstergesi olarak da kullanılır).

| Durum | LED_0 | LED_1 | LED_2 | Anlam |
|---|---|---|---|---|
| Uyarı fazı (mode girişi) | **500 ms yavaş blink** | kapalı | kapalı | Fan/pervane bağlı mı? Robotu sabitle, tekerlekler yere değmesin! **SW1 = başlat**, SW1+SW2 = çıkış |
| Seviye gösterimi | kapalı | **seviye kadar kısa blink: 1…4** | kapalı | 1 → PWM 50, 2 → 100, 3 → 150, 4 → 200 (`IMPELLER_TEST_PWM_1…4`) |
| Rampa (0 → hedef PWM, 1 s) | kapalı | kapalı | **~100 ms hızlı blink** | PWM tırmanıyor |
| Hedef PWM'de çalışıyor | kapalı | kapalı | **sabit yanar** | Fan hedef devirde |
| Fan kapandı | kapalı | **2 kısa blink** | kapalı | PWM = 0 (fren yok, serbest yavaşlama) |
| Çıkış | — | — | — | SW1+SW2 500 ms → 2 kez blink, menüye dön |

Buton haritası: **SW1 = seviye değiştir** (1→2→3→4→1, her değişimde yeni seviyeye tekrar
1 s rampalar) · **SW2 = fan aç/kapat** · **SW1+SW2 (500 ms) = çıkış**.

**Önerilen tribün test prosedürü (PC'siz, sahada):**

1. Hazırlık: pil **tam şarjlı**, tribün konnektörü takılı. Robot **yatay, düz ve boş** bir
   zeminde; tekerlekler serbest dönebilsin (pleksi üstündeyse fanın emiş deliği kapanmamalı!).
2. `DEBUG_MODE_USE_SERIAL 0` firmware'ini yükle, enerjiyi verirken SW1/SW2 basılı tutarak
   debug menüsüne gir (açılışta bir kez 3 LED yanıp söner).
3. SW2 ile **1**'i seç, SW1 ile onayla → LED_0 yavaş blink (uyarı fazı).
4. Güvenliği kontrol et, **SW1'e bas** → 2 kez blink + LED_1 seviyeyi blinkler (başlangıç: 4 = tam güç).
5. **SW2'ye bas** → rampa başlar (LED_2 hızlı blink, ~1 s) → LED_2 sabitlenir. Şimdi gözlemle:
   * Fan **sesli** olarak devreye giriyor mu? (LED_2 sabit + fan sesi = çıkış sinyali OK)
   * Başlangıçta **gecikme / takılma / titreme** var mı? → seviye 1'e (PWM 50) indirip
     "en düşük devirde dönmeye başlıyor mu?" kontrolü yap (kritik eşik testi).
6. Seviye taraması: SW1 ile 1→2→3→4 geç, her seviyede 3–5 s dinle/izle:
   * Her kademede devir **monoton artmalı**. PWM artarken devir artmıyorsa sürücü (BTN9960LV
     tarafı, `PWMC = pin 11`) veya besleme hattı şüphelidir.
   * SW2 ile birkaç kez aç/kapa yap → her açılışta rampa aynı mı? (EEPROM izi son açılışı tutar.)
7. Çıkış: **SW1+SW2 500 ms** → 2 blink → menüye dön. Logu sonra §4.3'teki gibi okuyup
   `IMPELLER / POWER` izinde **PWM–ADC6–ADC7** eğrisini incele: PWM büyürken ADC'nin çökmesi
   (sag) pil/konnektör zayıflığı demektir.

> Seviye varsayılanını değiştirme: `IMPELLER_TEST_PWM_1…4` (`ATLAS_1.4.3.ino`).
> Sadece tribün testini hızlı açmak için: `#define DEBUG_MODE_ALWAYS_ON 1` ve
> `#define DEBUG_MODE_START_OPERATION 1` yap → açılışta menü doğrudan 1'de açılır, SW1'e bas yeter.

### 4.5 Canlı LED sensör testi (No 2)

Önce `calibrateSensors()` çalışır (LED'ler kalibrasyon animasyonunu yapar), sonra:

* LED'ler = **o anda çizgiyi gören sensör sayısı** (0–7, 7+ doygun).
* **SW2 basılıyken** = **çizginin konumu** 0–7 (pozisyon/2000; 0 ≈ S0–S1 bölgesi … 7 ≈ S14–S15 bölgesi).
* Tek sensör kontrolü: sensörün üstüne siyah/beyaz kartı yaklaştır/uzaklaştır → sayının
  artıp azaldığını izle. (Hassas değerler için No 6 EEPROM izi + PC gerekir.)
* Çıkış: SW1+SW2 500 ms.

### 4.6 LED UI testi (No 3)

* **LED_0 = SW1**, **LED_1 = SW2** (basılıyken yanar).
* **LED_2 = READY (PD3) ve GO (PD4) girişleri**: ikisi de aktif → sabit yanar;
  sadece biri aktif → yavaş blink; hiçbiri aktif değil → sönük.
* Çıkış: SW1+SW2 500 ms.
* Not: `readButton_1 = PC5`, `readButton_2 = PD7` çekmeli (pull-up) girişlerdir; basılınca GND'ye çeker.

### 4.3 Logu PC'ye çek ve çözümle

Kart enerjili + ISP takılı iken:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\dump_eeprom.ps1
```

Bu betik avrdude'yi `%LOCALAPPDATA%\Arduino15\packages\arduino\tools\avrdude` içinden bulur,
EEPROM'u `tools\logs\atlas_log_<tarih>.bin` olarak okur, **çözümler** ve yanına `.csv` yazar.

Elle yapmak isterseniz:

```powershell
& "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools\avrdude\8.0.0-arduino1\bin\avrdude.exe" `
  -c usbasp -p m328p -U eeprom:r:atlas_log.bin:r
powershell -ExecutionPolicy Bypass -File .\tools\dump_eeprom.ps1 -DecodeOnly .\atlas_log.bin
```

Ek seçenekler:

```powershell
# başka programlayıcı / port (Arduino as ISP, USBtiny vb.)
... -Programmer arduinoasisp -Port COM7
# logu temizle (0xFF ile doldur)
... -Clear
# sadece daha önce alınmış bir .bin'i çöz (donanım gerekmez)
... -DecodeOnly .\tools\logs\atlas_log_20260924_101500.bin
```

## 5. EEPROM log biçimi (1024 byte)

| offset | içerik |
|---|---|
| 0–3 | magic `'A' 'T' 'L' 'S'` (geçerli log işareti) |
| 4 | log sürümü (`EEPROM_LOG_VERSION = 1`) |
| 5 | mod (0 ham sensör, 1 kalibre sensör, 2 kalibrasyon, 3 konum, **4 tribün/güç**) |
| 6 | kayıt sayısı |
| 7 | kayıt uzunluğu (byte) |
| 8 | flags (bit0 = `invertSensorReads`) |
| 9–10 | örnekleme periyodu (ms, little endian) |
| 11 | ayrılmış |
| 12… | kayıtlar — adres = `12 + index * kayıt uzunluğu` |

Örnek çözümleme çıktıları:

```
Mode       : CALIBRATION (MAX/MIN/TH)
Kayit      : 1 adet x 48 byte, periyot 0 ms

MAX       200 199 198 197 196 195 194 193 192 191 190 189 188 187 186 185
MIN        20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35
TH        110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125
```

```
Mode       : CALIBRATED SENSORS
Kayit      : 3 adet x 16 byte, periyot 50 ms
 #  t(ms)     S0  S1 ... S5 ... S15
  0      0      0   0 ...255 ...   0  .....^..........
^ = o anda cizgiyi goren sensor
```

```
Mode       : LINE POSITION
 #  t(ms)   position  pos/1000  online
  1     10       4500       4,5       1     <- 4. ve 5. sensor arasi
```

```
Mode       : IMPELLER / POWER (tribun)
Kayit      : 7 adet x 3 byte, periyot 20 ms
 #  t(ms)   pwm  ADC6  ADC7   (pwm 0 = fan kapali)
  0      0    50   190   180
  1     20   100   185   176
  2     40   150   180   170
  3     60   200   172   160
  4     80   200   171   159
  5    100     0   200   195
  6    120     0   200   195

Fan acik ornek : 5   fan kapali ornek: 2
ADC6 (fan acik) : min 171  max 190
ADC7 (fan acik) : min 159  max 180
```

PWM büyürken ADC'nin çökmesi = pil/konnektör/kablo zayıflığı. `ADC6_value`/`ADC7_value`,
orijinal firmware'deki `readOtherADCs()` fonksiyonunun okuduğu iki yardımcı analog giriştir
(ATmega328P **AU** paketine özel ADC6/ADC7 pinleri); hangisinin neye bağlı olduğu kart
revizyonuna göre değişebilir, izden doğrulayın.

`dump_eeprom.ps1` ayrıca `.csv` üretir; Excel'de açıp (TR yerel ayarı virgüllü ondalık
kullanır) çizgi takibi / kalibrasyon verisini grafikleyebilirsiniz.

## 6. Senaryo C — Boşta duran Arduino'yu "bedava USB-TTL" yapmak

Adaptör almak istemiyorsanız, elinizdeki herhangi bir USB'li Arduino kartı USB↔UART köprüsü
olarak kullanılabilir; çünkü kart üzerindeki USB-serial çip (CH340 / FT232 / 16U2) ile
ATmega çipi birbirinden bağımsızdır:

1. Yedek Arduino'nun **RESET pinini GND'ye bağlayın** (kendi MCU'su devre dışı kalır, USB-serial
   çipi çalışmaya devam eder).
2. Bağlantı: `ATLAS TX (PD1 / pin 1) → yedek kartın RX (pin 0)`, `GND → GND`.
   (Yedek kartın TX'i gerekmez; sadece ATLAS'tan PC'ye veri akışı yeterli.)
3. Arduino IDE'de **yedek kartın** COM portunu seçin, Serial Monitor 115200.
4. ATLAS'ı debug modunda başlatın → menü metni görünür.

> Not: ATLAS kartında UART, ISP veya ayrı bir header üzerinde çıkarılmamışsa MCU'nun
> PD1 (pin 1) pad'ine lehim/tel lehim gerekebilir.

## 7. Ölçülen bellek kullanımı (bu repoda derlenerek doğrulandı)

| Yapılandırma | Flash | SRAM |
|---|---|---|
| `DEBUG_MODE_USE_SERIAL 1` (orijinal seri konsol) | 12.102 B (%36,9) | **1.578 B (%77,1)** |
| `DEBUG_MODE_USE_SERIAL 0` (EEPROM trace + LED tribün testi) | **9.064 B (%27,7)** | **318 B (%15,5)** |

Seri debug modu, `String` sınıfı yüzünden ~1,27 kB SRAM yiyor (kaynaktaki uyarı notu da bunu
söylüyor). EEPROM trace modunda bu yük tamamen kalkar; robot pistte koşarken yığın taşması
riski çok azalır.

Derlemeyi kendiniz doğrulamak için (Arduino IDE'ye gerek yok, avr-gcc + ctags kullanır):

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_check.ps1                  # seri debug
powershell -ExecutionPolicy Bypass -File .\tools\build_check.ps1 -NoSerialDebug   # EEPROM trace
```

Betik `%TEMP%\atlas_build\ATLAS_1.4.3.hex` dosyasını da üretir.

## 8. Debug modunu butonsuz başlatmak

Kart robot üzerinde montajlı olduğu için butonlara erişemiyorsanız:

* **Kodla:** `#define DEBUG_MODE_ALWAYS_ON 1` → her enerji verilişinde doğrudan debug moduna
  girer (pist kodu hiç çalışmaz; bench/servis modu).
* **Donanımla:** boot anında PC5 (SW1) veya PD7 (SW2) pinini kısa süre GND'ye çekmek yeterli.

## 9. Debug sırasında bulunan bir yazılım hatası (RunControl.ino)

`updatePeriod()` içindeki "offline method" bloğunda:

```cpp
// get offline elapsed time
const unsigned long offlineElapsedTime = currentTime - offlineElapsedTime;
```

`offlineElapsedTime` kendi kendisiyle başlatılıyor → **tanımsız/çöp değer**. Yani robot çizgiyi
kaybettiğinde "iç teker fren süresi" (`OFFLINE_INNER_MOTOR_BRAKE_TIME_MS`) rastgele çalışır;
pist dışına çıkış davranışı ve zamanlama bundan etkilenir. Doğrusu büyük olasılıkla:

```cpp
const unsigned long offlineElapsedTime = currentTime - offlineStartTime;
```

Bu bir kontrol davranışı değişikliği olduğu için **bilerek değiştirmedim**; isterseniz düzeltirim
(1 satır). Derleyici `-w` ile uyarılar kapatıldığı için gözden kaçmış.

## 10. Kısa özet (TL;DR)

* USB-TTL **sadece** seri metni PC'ye taşımak için gerekir; adaptörsüz de debug yapılabilir.
* Kartın kendi USB'si varsa: yükle → Serial Monitor 115200 → açılışta buton basılı tut.
  Ekstra hiçbir şey gerekmez.
* Yükleme USBasp/ISP ile yapılıyorsa: `#define DEBUG_MODE_USE_SERIAL 0` yap, yükle, LED'lerden test seç,
  sonra `tools\dump_eeprom.ps1` ile logu oku → sensör, kalibrasyon ve konum verisini tablo/CSV olarak al.
* ISP ile butonsuz başlatmak için: `#define DEBUG_MODE_ALWAYS_ON 1`.
* Sahada sadece tribün testi: `DEBUG_MODE_ALWAYS_ON 1` + `DEBUG_MODE_START_OPERATION 1` →
  açılışta menü doğrudan tribün testinde (1) açılır; SW1'e bas, başla.
* Boşta Arduino varsa onu USB-UART köprüsü yaparak orijinal seri `debugMode()`'u bedavaya kullanabilirsin.

