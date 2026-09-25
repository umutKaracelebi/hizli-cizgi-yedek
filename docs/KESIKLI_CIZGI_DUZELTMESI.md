# Kesikli çizgi (dash) geçiş sorunu — inceleme ve düzeltme

Tarih: 25 Eylül 2026. Kapsam: `ATLAS_Takim/antigravity-takim/` kodu. Bu belge, sahada bildirilen "beyaz kesikli çizgiye gelince robot sapıtıyor" sorununun kök nedenlerini, uygulanan yazılım düzeltmesini ve sahada doğrulama planını kaydeder.

> Statik inceleme + hedef karta derleme + masaüstü kontrol-mantığı derlemesi yapıldı. **Gerçek robotta/pistte doğrulama henüz yapılmadı.**

## 1. Bildirilen belirtiler

1. İlk testte robot beyaz boşlukta geldiği çizgiden **geriye** gitti.
2. Başka bir testte pistten çıktı ve **pist dışındaki zemindeki siyah çizgiyi** takip etti.
3. Genel izlenim: beyaz kısma gelince robot **ileri-sağa** gidiyor; sanki çizgi arıyor.

## 2. Kök nedenler

### K1 — Kod derlenmiyordu (en kritik)

`RunControl.ino` içinde `stopRequested` bloğu iki kez kopyalanmış durumdaydı. İkinci kopya, değişkenin kapsamı dışında kaldığı için derleyici hata veriyordu:

```text
RunControl.ino:219: error: 'stopRequested' was not declared in this scope
RunControl.ino:231: error: expected constructor, destructor, or type conversion before ';' token
RunControl.ino:232: error: expected declaration before '}' token
```

Ek olarak, bozuk süslü parantez yapısı Arduino'nun otomatik prototip üreticisini (ctags) de bozduğu için `UI.ino` ve `Sensors.ino` fonksiyonları hiç tanınmıyordu: `'UIInit' was not declared in this scope`, `'sensorsInit' ...`, `'updatePeriod' ...`.

Bu dosya olduğu gibi karta yüklenemezdi. Masaüstünde `arduino-cli compile --fqbn arduino:avr:nano` ile birebir doğrulandı. Düzeltildikten sonra: **flash 7504/30720 B, SRAM 305/2048 B, uyarı yok.**

### K2 — "Kesik köprüsü" sabiti tanımlıydı ama kullanılmıyordu

`antigravity-takim.ino` içinde:

```c
#define OFFLINE_GAP_BRIDGE_MS 15     // kodda kullanılan tek köprü
#define OFFLINE_DASH_BRIDGE_MS 75    // HİÇBİR YERDE KULLANILMIYOR
```

`RunControl.ino` offline dalı yalnızca `OFFLINE_GAP_BRIDGE_MS` (15 ms) ile çalışıyordu. `pist1.pdf`'te iki kesikli parçanın merkez çizgisi uçları arası yaklaşık **55.7 mm**; beyaz boşluk ~40–56 mm. 15 ms'lik kör devam, ~1 m/s'de yalnızca ~15 mm demektir. Yani robot **daha beyaz alanın içindeyken** üreticinin sert pivot manevrasına giriyordu.

### K3 — Pivot yönü bayat bilgiye dayanıyordu

Kurtarma yönü `lastDetectedSide` ile seçiliyor; bu değişken yalnızca `sensorValues[0..1]` veya `sensorValues[14..15]` aktifken güncelleniyor. Kesikli çizgide hat **merkezde** (6–9) kaybedildiği için değişken en son virajdan kalan değerinde kalıyordu. Sonuç: beyaz boşlukta yön fiilen rastgele → bazen 180° dönüp geldiği çizgiye geri gitme (belirti 1), bazen sağa savrulma (belirti 3).

### K4 — Manevra tam güçte ve çok uzundu

Eski arama manevrası: dış teker `+200` (maksimum ileri), iç teker `-120`, süre sınırı `OFFLINE_FAILSAFE_MS = 1500 ms`. Bu, robotun pistten çıkıp herhangi bir siyah çizgiye kilitlenmesi için yeterli süre ve tork demek (belirti 2).

### K5 — Çizgiyi yeniden yakalarken türev darbesi (derivative kick)

Köprü boyunca `previousError` güncellenmiyordu. Çizgi yeniden bulunduğu ilk döngüde `deltaError = error - previousError` yapay olarak büyüyor, `KD = 0.35` ile çarpılıp `linePWM` doyuma (`±1000`) gidiyordu. Robot çizgiyi bulduğu anda sert savruluyordu — yanlış çizgiye kilitlenmenin yazılım tarafındaki ikinci nedeni.

### K6 — Yabancı çizgi kapısı yoktu

Sensör barının ucunda beliren her siyah, "çizgi bulundu" sayılıyordu. Pist dışı zemin çizgisi, geri dönüş hattı veya gölge/ek yeri dahi geçerli kabul ediliyordu.

## 3. Yeni algoritma

Çizgi kaybı artık **sınıflandırılıyor** ve iki farklı davranış uygulanıyor:

```text
ÇİZGİ VAR  ──► PD takip (lastOnLinePosition / lastLinePWM kaydedilir)
    │
    └─ çizgi kayboldu ──► OFFLINE olayı başlat + bağlamı sınıflandır
                              │
        |konum-7500| ≤ 3500 VE |son linePWM| ≤ 150 ?
                              │
              evet ──► "DÜZ KESİK"  → köprü = 120 ms
              hayır ─► "DÖNÜŞ"      → köprü = 15 ms

KÖPRÜ (kör devam):
   - Düz kesik: iki teker eşit hızda ileri; son direksiyon ±80 ile
     sınırlı biçimde yumuşakça azaltılır (hafif eğim toleransı).
   - Dönüş     : direksiyon tamamen bırakılır (düz devam).
   → Bu pencerede robot DÖNMEZ; beyaz boşluk düz geçilir.

KÖPRÜ DOLDU, ÇİZGİ YOK → ARAMA (yerinde dönüş):
   dış teker +120, iç teker -120  → robot ileri kaçmaz, barını çevirir
   250 ms sonra yön ters çevrilir (iki yönlü tarama)

ÇİZGİ GÖRÜLDÜ → KABUL KAPISI:
   1) en az OFFLINE_MIN_CONFIRM_LOOPS (3) ardışık döngü boyunca görülmeli
   2) düz kesik bağlamında ve köprü penceresi içinde konum merkez
      bandında (±4500) olmalı  → barın ucundaki yabancı çizgi reddedilir
   3) kabul edilince deltaError = 0 alınır → türev darbesi yok

OFFLINE_FAILSAFE_MS (1200 ms) boyunca çizgi yoksa → 500 ms fren + kalıcı kilit
```

## 4. Parametre değişiklikleri

| Sabit | Eski | Yeni | Gerekçe |
|---|---|---|---|
| `OFFLINE_GAP_BRIDGE_MS` | 15 | 15 | Dönüş/keskin köşe için kısa kör devam korundu |
| `OFFLINE_DASH_BRIDGE_MS` | 75 (kullanılmıyordu) | **120** | 40–56 mm boşluk ≈ 0.5–1 m/s'de 40–110 ms; artık gerçekten devrede |
| `OFFLINE_DASH_CENTER_BAND` | – | 3500 | Kayıp anında hat merkezde miydi? |
| `OFFLINE_DASH_STEER_MAX` | – | 150 | Kayıp anında düz mü gidiliyordu? |
| `OFFLINE_DASH_HOLD_STEER_MAX` | – | 80 | Köprüde direksiyon kalıntısını sınırlar |
| `OFFLINE_DASH_REACQUIRE_BAND` | – | 4500 | Kesik sonrası devam çizgisinin beklenen bandı |
| `OFFLINE_MIN_CONFIRM_LOOPS` | – | 3 | Tek sensörlük parazit "çizgi bulundu" sayılmaz |
| Arama PWM'i | dış +200 / iç -120 | ±120 | Tam güç yerine sınırlı tork, **yerinde** dönüş |
| `OFFLINE_SEARCH_PHASE_MS` | – (sürekli tek yön) | 250 | 250 ms'de bir ters yöne tarama |
| `OFFLINE_FAILSAFE_MS` | 1500 | 1200 | Kalıcı kayıpta daha erken güvenli fren |
| Türev darbesi | `previousError` bayat | `deltaError = 0` | Yeniden yakalamada savrulma yok |
| Merkez sabiti | satır içi `15000/2` | `LINE_CENTER_POSITION` | Tek kaynak, okunabilirlik |

Sürüş (PD) katsayıları, hız seçimi (60–250 PWM), türbin PWM'i ve kalibrasyon eşiği **değiştirilmedi**: sorun kontrol/kurtarma mantığındaydı, ayar değerlerinde değil. Bir değişkeni bir kerede değiştirme kuralı korundu.

## 5. Doğrulama

### 5.1 Hedef karta derleme (yapıldı)

```powershell
$cli = "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
& $cli compile --fqbn arduino:avr:nano --warnings all ATLAS_Takim/antigravity-takim
```

- Düzeltme **öncesi**: derlenmiyor (K1'deki hatalar).
- Düzeltme **sonrası**: `flash 7504/30720 B`, `SRAM 305/2048 B`, uyarı yok.

### 5.2 Masaüstü kontrol-mantığı testi (hazır, koşturulmadı)

Dosyalar: `ATLAS_Takim/antigravity-takim/tests/dash_gap_test.cpp`, `tests/atlas_prototypes.h`, `tests/stubs/Arduino.h`.
Test, gerçek `.ino` sekmelerini olduğu gibi dahil eder; Arduino/AVR katmanını taklit ederek `updatePeriod()` döngüsünü adım adım koşturur.

```powershell
g++ -std=c++11 -Wall -Wextra -pedantic -o "$env:TEMP\atlas_dash_test.exe" `
    ATLAS_Takim/antigravity-takim/tests/dash_gap_test.cpp
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\atlas_dash_test.exe" }
```

| Test | İddia |
|---|---|
| T1 | Düz kesikli çizgide ≥3 beyaz boşluk geçilir; boşlukta **negatif PWM (dönüş) olmaz**, şeritte kalınır |
| T2 | +15 ms'de iki teker de ileri (dönüş yok), +125 ms'de yerinde dönüş başlar |
| T3 | Kenardan kayıpta (köşe) 15 ms sonra arama başlar ve yön `lastDetectedSide` ile aynıdır |
| T4 | Kesik penceresinde bar ucundaki yabancı çizgi reddedilir; gerçek devam çizgisi gelince `previousError == error` (türev darbesi yok) |
| T5 | Çizgi tamamen biterse arama ±120 ile sınırlıdır, robot yerinde döner, 1200 ms'de failsafe |
| T6 | 25 mm kaçık başlangıçta PD çizgiye oturur (işaret/yön doğrulaması) |

**Sınırlar:** Modelde PWM→hız ilişkisi doğrusal; enkoder, patinaj, vakum sürtünmesi, motor zaman sabiti ve gerçek gecikmeler yok. Test "gerçek pistte çalıştı" kanıtı değil, kontrol mantığının regresyonudur.

**Bu makinede durum:** host C++ derleyicisi (g++/clang/MSVC) kurulu olmadığı için test koşturulamadı. Test dosyası hedef için `avr-g++ -fsyntax-only -Wall -Wextra` ile derlendi: **0 hata, 0 uyarı** (tüm fonksiyon bildirimleri ve tipler gerçek `.ino` koduyla tutarlı). Koşum sonrası çıkan sayılar buraya eklenmelidir.

## 6. Sahada doğrulama planı

Her adımda tek değişken; sonuçlar [test ve yarışma planı §7](TEST_VE_YARISMA_PLANI.md) kayıt şablonuna yazılır. Koşu öncesi: `ATLAS_Takim/antigravity-takim/antigravity-takim.ino` + 4 sekme birlikte yüklenir (tek `.ino` gönderme hatası tekrarlanmaz).

| Adım | Ayar | Gözlenecek |
|---|---|---|
| A | 60–80 PWM, 5 koşu | Kesikte dönüş yok, robot çizgiye düz devam ediyor |
| B | Varsayılan 100 PWM, 5 koşu | Aynı davranış yüksek hızda da sürüyor |
| C | Sol taraftaki 90° zikzak | Arama manevrası çalışıyor, köşe dönülüyor (T3'ün saha karşılığı) |
| D | Elle kesik bölgeyi uzat / çizgiyi kapat | Robot yerinde dönerek arar, ~1.2 s içinde durur; pistten kaçmaz |
| E | Başarısız koşu analizi | Hangi aşamada? (köprüde dönüş / yabancı çizgi / failsafe) |

## 7. Ayar rehberi

| Belirti | Parametre | Yön |
|---|---|---|
| Boşluk geçilmiyor, robot daha beyazdayken dönüyor | `OFFLINE_DASH_BRIDGE_MS` | 120 → 150 (kör süre artar) |
| Gerçek virajlarda gecikme/savrulma | `OFFLINE_GAP_BRIDGE_MS`, `OFFLINE_DASH_CENTER_BAND` | 15 → 10; 3500 → 2500 |
| Kesik sonrası yanlış çizgi kabul ediliyor | `OFFLINE_DASH_REACQUIRE_BAND` | 4500 → 3000 |
| Arama çok hızlı / yavaş | `OFFLINE_SEARCH_PWM`, `OFFLINE_SEARCH_PHASE_MS` | ±120, 250 ms |
| Failsafe çok erken / geç | `OFFLINE_FAILSAFE_MS` | Köprü + faz süreleri toplamından büyük olmalı (1200 ms) |

## 8. Değişen ve eklenen dosyalar

- `ATLAS_Takim/antigravity-takim/antigravity-takim.ino` — kesik/dönüş parametreleri yeniden düzenlendi.
- `ATLAS_Takim/antigravity-takim/RunControl.ino` — kopyalanmış bozuk blok silindi; kayıp sınıflandırması, köprü, yerinde dönüşlü arama, yeniden yakalama kapısı ve türev darbesi düzeltmesi eklendi.
- `ATLAS_Takim/antigravity-takim/tests/` — **yeni**: `dash_gap_test.cpp`, `atlas_prototypes.h`, `stubs/Arduino.h`. `tests/` alt klasörü Arduino derlemesine dahil edilmez (doğrulandı: sketch yine sorunsuz derleniyor).
- `docs/KESIKLI_CIZGI_DUZELTMESI.md` — bu belge.
- `Motors.ino`, `Sensors.ino`, `UI.ino` — değişmedi.

Lisans notu: üretici tabanı CC BY-NC-ND 4.0'tır; bu sürüm yalnızca takım içi kullanım içindir, kamuya açık dağıtılamaz.
