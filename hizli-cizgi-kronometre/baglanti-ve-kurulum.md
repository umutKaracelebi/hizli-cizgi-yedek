# Robot tur kronometresi — MZ80 bağlantı ve kurulum

## Kullanılan sensör ve çalışma mantığı

Bu sürüm, donanım listesindeki **Robotistan MZ80 (E18-D80NK), 5 V** yansımalı kızılötesi sensör içindir. Karşı tarafa lazer veya ayrı alıcı koyulmaz; sensör robotun gövdesinden yansıyan kızılötesi ışığı algılar. Analog uzaklık ölçmez, dijital engel var/yok bilgisi verir.

İlk algılama süreyi başlatır. Robot algılama alanından çıkar; sonraki giriş süreyi durdurur. Yeni ölçüm için butona basıp bırakılır. Son ve en iyi süre RAM'de tutulur; buton silmez, güç kesilmesi siler. Bilgisayar gerekmez. Sensör robot kimliğini veya yönünü ayırt etmez; el, başka robot veya geri geçiş de sayılabilir.

Ürün sayfası 5 V besleme, yaklaşık 25–100 mA tüketim ve yüzeye bağlı 3–80 cm algılama aralığı belirtir. **80 cm garanti çalışma mesafesi değildir.** Robotun rengi, açısı, ortam ışığı ve sensör ayarı tetikleme noktasını etkiler. Bu değişiklik parazitsiz çalışma veya ±1 ms ölçüm doğruluğu garantisi vermez.

- [Listedeki Robotistan ürünü](https://www.robotistan.com/80cm-menzilli-kizilotesi-sensor-mz80): besleme ve menzil bilgisi.
- [DFRobot SEN0019 teknik belgesi](https://wiki.dfrobot.com/Adjustable_Infrared_Sensor_Switch__SKU_SEN0019_): benzer sensör için kahverengi = 5 V, mavi = GND, siyah = sinyal ve engelde LOW açıklaması. Bu belge Robotistan ürününün birebir kablo doğrulaması değildir.

## Ayrı alıcı gerekiyor mu? Fotodiyot / KY-022

**Hayır. MZ80'in kızılötesi vericisi ve alıcısı kendi gövdesindedir.** Robotun üzerinden yansıyan ışığı algılar; karşı tarafa BPW34 fotodiyot, KY-022 veya lazer yerleştirmeyin. Fotodiyotun küçük yüzeyine ışın hizalamak bu kurulumda gerekmiyor.

KY-022, modüle edilmiş kızılötesi kumanda sinyallerini almak için tasarlanır; sürekli yanan 650 nm kırmızı lazerin alıcısı olarak BPW34'ün yerine doğrudan takılamaz. KY-022 ile bir algılama sistemi kurulacaksa uygun modüle edilmiş IR verici, alıcının zamanlama koşulları ve farklı yazılım/devre tasarımı gerekir; mevcut MZ80 kurulumunun parçası değildir.

**Bu sürümde kullanılacaklar:** Arduino Uno, MZ80, Waveshare LCD, normalde açık buton, breadboard ve bağlantı kabloları. Lazer, BPW34, LM393 ve ayrı mavi trimpot kenarda kalır. Eski lazer devresine dönülürse uygun optik alıcı ve eşik devresi yeniden gerekir; MZ80 kullanırken bunlar elzem değildir.

## Sıfırdan kurulum sırası

Devre tamamen sökülmüşse aşağıdaki sırayı izleyin. **Tüm bağlantılar bitene kadar USB ve diğer beslemeler çıkarılmış olsun.**

1. **Besleme hatlarını hazırlayın:** Uno **5V → breadboard +**, Uno **GND → breadboard −**. İlk kurulumda aynı taraftaki, kesintisiz tek besleme bölümünü kullanın. Raylar ortadan bölünmüş olabilir; karşı taraftaki raylar da kendiliğinden bağlı değildir. Birden fazla bölüm kullanırsanız yalnızca + ile +, − ile − bölümlerini köprüleyin; + ile − birbirine bağlanmaz.
2. **LCD'yi bağlayın:** aşağıdaki LCD bölümündeki pin eşleşmelerini uygulayın. VCC → + hattı, GND → − hattı; DIN → D11, CLK → D13, CS → D10, DC → D7, RST → D8, BL → D9.
3. **MZ80'i bağlayın:** aşağıdaki kablo doğrulaması tamamlandıktan sonra VCC → + hattı, GND → − hattı, OUT → D2. Sensörün karşısına ayrı alıcı koymayın.
4. **Butonu bağlayın:** basılmadan birbirinden ayrı, basınca birleşen iki kontak grubundan biri → D4, diğeri → − hattı. Dört bacaklı butonda bacak çiftlerinin breadboard üzerinden istemeden birleşmemesine dikkat edin. Orta kanalı köprülemesi tek başına doğru kontak seçildiğini kanıtlamaz. Butona 5V bağlanmaz, ek direnç gerekmez.
5. **Kontrol edin:** +/− kısa devresi, aynı breadboard sırasında birleşen farklı sinyaller veya D2'ye bağlı eski LM393 çıkışı olmamalı. MZ80'in kablo görevleri belirsizse güç vermeden doğrulayın.
6. **Kodu yükleyin:** USB'yi takın; aşağıdaki yükleme bölümüne göre `robot_kronometre/robot_kronometre.ino` dosyasını Uno'ya yükleyin.
7. **İlk denemeyi yapın:** MZ80 önünde yaklaşık 10–20 cm'de el/kart ile `ENGEL VAR/YOK` değişimini doğrulayın. Alan boşken butona basıp bırakın, `HAZIR` yazısını bekleyin. İlk getirip çekme süreyi başlatır; bir saniye bekleyip tekrar getirip çekme durdurur.

Aşağıdaki ayrıntılı bölümler bağlantıların açıklamasıdır; sıfırdan kurulumda eski lazer devresini kurmanız gerekmez.

## Eski devreden geçiş — güç kapalıyken

1. Arduino USB beslemesini ve varsa diğer beslemeleri çıkarın.
2. **LM393 pin 1 ile D2 arasındaki kabloyu sökün. MZ80 ve LM393 çıkışlarını birleştirmeyin.**
3. Lazer, BPW34, LM393, harici trimpot ve bunlara ait direnç/geri besleme bağlantılarını aktif devreden ayırın. Eski şema `baglanti-ve-kurulum-lazer-arsiv.md` dosyasındadır.
4. LCD bağlantıları ve D4–GND butonu aynı kalır. Besleme/GND hatlarını sökerken ekranın ortak bağlantılarını koruyun.
5. Aşağıdaki MZ80 bağlantısını yapın ve güncel kodu yükleyin.

## MZ80 bağlantısı

**Eldeki sensörün kablo renkleri kullanıcı tarafından mavi, kahverengi ve siyah olarak bildirildi.** Bunlar yaygın renk düzeniyle uyuşuyor; ancak renklerin görülmesi, görevlerinin elektriksel olarak doğrulandığı anlamına gelmez.

**Önce sensör üzerindeki etiketi veya satıcının kablo şemasını doğrulayın.** Robotistan sayfasının metni kablo görevlerini belirtmiyor. Etiket yoksa veya sensör 5 V dışı besleme/başka çıkış tipi belirtiyorsa güç vermeden satıcıdan doğrulama alın. Multimetre olmadan deneme-yanılmayla besleme uçlarını seçmeyin.

Standart renk eşleşmesi doğrulanmış **5 V, engelde LOW** sürüm için bağlantı:

- **Kahverengi — VCC / +5V → breadboard + hattı → Uno 5V**.
- **Mavi — GND / 0V → breadboard − hattı → Uno GND**.
- **Siyah — OUT / sinyal → Uno D2**.

Sensörün kendi etiketi bu eşleşmeden farklıysa bu renk şemasını uygulamayın.

```text
Uno 5V  ---------------- MZ80 VCC (+5V)
Uno GND ---------------- MZ80 GND
Uno D2  ---------------- MZ80 OUT

Uno D4  ----- normalde açık buton ----- GND
```

- Sensörü D2 veya başka GPIO'dan beslemeyin; besleme 5V pininden gelir. D2 yalnızca sinyal girişidir.
- Kod D2 için `INPUT_PULLUP` kullanır: engel yokken HIGH, algılama sırasında LOW beklenir; düşen kenar yakalanır.
- Harici pull-up temel kurulumda gerekmez. Doğrulanmış açık kolektör çıkışta gerekirse eldeki **10 kΩ direnci D2 ile 5V arasına** ekleyebilirsiniz. D2 ile GND arasına pull-down koymayın; direncin yerine doğrudan 5V kablosu takmayın.
- Eski fotodiyot yük direnci, harici trimpot ve 1 MΩ geri besleme kullanılmaz. MZ80'in kendi üzerindeki ayar vidası kullanılır.
- Tüm GND hatları ortaktır. Breadboard besleme rayı ortadan bölünmüşse kullanılan bölümleri uygun şekilde köprüleyin.
- Besleme parazitine karşı **100 nF seramik kondansatör** MZ80 VCC–GND arasına, sensöre mümkün olduğunca yakın eklenebilir. İsteğe bağlı 10 µF kondansatörde + uç 5V, − uç GND olur. Bunlar yanlış bağlantıyı/optik ayarı düzeltmez; 1 MΩ'u MZ80 çıkışına geri besleme olarak bağlamayın.

## LCD — bağlantılar değişmiyor

Bu bağlantı yalnızca listedeki **Waveshare 2.4inch LCD Module, 8 pin, ILI9341** içindir. Üretici 5 V beslemede Uno/5 V lojik bağlantısını belirtir; başka çıplak ILI9341 ekranlara genellenmez.

- VCC → Uno 5V
- GND → Uno GND
- DIN → Uno D11 (MOSI)
- CLK → Uno D13 (SCK)
- CS → Uno D10
- DC → Uno D7
- RST → Uno D8
- BL → Uno D9

Kablo rengine değil ekranın pin etiketlerine göre bağlayın. MISO gerekmez.

- [Üretici belgesi](https://www.waveshare.com/wiki/2.4inch_LCD_Module)
- [Listedeki ekran](https://www.robotistan.com/24-inc-lcd-ekran-modulu)

Kod artık büyük alanları her 100 ms'de siyaha boyayıp yeniden yazmaz: yalnızca değişen karakterleri siyah arka planla günceller. Son/en iyi değerleri değişmedikçe yeniden çizilmez. Uno RAM'ini korumak için tam ekran tamponu kullanılmaz. Ekranın tamamı veya arka ışık yine yanıp sönüyorsa besleme, bağlantı veya yeniden başlama ihtimalini ayrıca kontrol edin.

## Buton ve besleme

- Normalde açık anlık butonun iki ayrı kontağı → **D4 ve GND**. `INPUT_PULLUP` nedeniyle ek buton direnci gerekmez; butona 5V bağlanmaz.
- Dört bacaklı butonda aynı gruptaki bacaklar zaten bağlı olabilir. Basılmadan ayrı, basınca birleşen iki kontağı kullanın; breadboardda aynı bağlı sıraya düşürmeyin. Sürekli `BUTONU BIRAK` yazıyorsa bağlantıyı kontrol edin.
- Uno'yu regüle 5 V USB adaptörü/powerbank ile USB soketinden besleyin. MZ80 ve LCD'nin toplam yükünü karşılayan kararlı bir besleme kullanın; powerbank düşük yükte kapanmamalıdır.
- VIN/barrel girişine 5 V vermeyin. Harici 5V hattı ile bilgisayar USB beslemesini gelişigüzel paralel bağlamayın.
- Robot motorlarını kronometrenin 5V hattından beslemeyin; robotun motor beslemesi ayrı olsun. Sensör kablosunu motor/güç kablolarından uzak tutun.

## Kodu yükleme

Arduino IDE Kütüphane Yöneticisi'nden **Adafruit ILI9341**, **Adafruit GFX Library** ve gerektiğinde **Adafruit BusIO** bağımlılığını kurun. SPI, Arduino çekirdeğindedir. MZ80 için ek kütüphane gerekmez.

`robot_kronometre/robot_kronometre.ino` dosyasını açın; `TimingCore.h` aynı klasörde kalmalıdır. Kartı **Arduino Uno**, portu kendi kartınızın portu seçerek derleyin ve yükleyin. Klon kart görünmüyorsa USB-seri çipini belirleyin; her klonun CH340 olduğunu varsaymayın.

Ekran yazıları yerleşik font için ASCII'dir. Yeni sürümde `ISIN: ACIK/KESIK` yerine **`MZ80: ENGEL YOK/VAR`** görünür. Hâlâ eski yazılar varsa doğru dosyayı, kartı ve yükleme sonucunu kontrol edin.

## Multimetresiz ilk ayar ve süre denemesi

1. MZ80'i geçiş koridorunun yanına, robotun gövdesine bakacak biçimde sabitleyin. Zemini veya karşı duvarı sürekli algılamasın. İlk denemeyi yaklaşık **10–20 cm** mesafede yapın; sensöre hedefi dayamayın.
2. Algılama alanını boşaltın. Ekranda **`MZ80: ENGEL YOK`** sabit kalmalı.
3. Önüne robot gövdesini veya mat bir kartı koyun: **`MZ80: ENGEL VAR`** olmalı. Çekince `ENGEL YOK` olmalı. Sensörün durum LED'i yardımcıdır; LED'in kesin anlamı için kendi ürün etiketini/belgesini izleyin, tek başına besleme/çıkış doğrulaması saymayın.
4. Bu değişim net değilse **sensörün kendi ayar vidasını** küçük adımlarla çevirin; sınırına gelirse zorlamayın. Hedefi algılayıp arka planı algılamayan ayarı seçin. Maksimum menzile ayarlamak zorunda değilsiniz. Eski mavi harici trimpot artık devrede değil.
5. Alan boşken butona **basıp bırakın**, yaklaşık 0.5 saniye sonra **`HAZIR - ilk gecisi bekle`** yazısını bekleyin.
6. Elinizi/kartı önüne getirip çekin: **ölçüm başlar**.
7. İlk algılamadan en az **0.5 saniye sonra**, hedefi tekrar getirip çekin: **ölçüm durur**, son ve en iyi süre güncellenir.
8. Hedefi önünde sürekli tutmak ikinci geçiş üretmemeli. Sonuçtan sonra hareketler yeni ölçüm başlatmamalı; yeniden başlatmak için buton gerekir.

Sensörü ayarlarken deneme hareketleri ölçümü başlatabilir; ayar bittiğinde alanı boşaltıp butonla yeniden hazırlayın. El testi başlangıç içindir; son ayarı gerçek robotun yüzeyiyle yapın.

## Kararlılık kontrolü ve sınırlar

- Alan boşken en az **30 saniye** bekleyin: ölçüm kendiliğinden başlamamalı.
- Hedefi sabit tutun: `ENGEL VAR` sabit kalmalı, başlayan ölçüm kendi kendine durmamalı.
- Robot motorları çalışırken ve gerçek ortam ışığında testleri tekrarlayın. En yüksek robot hızında da geçişlerin kaçmadığını doğrulayın.
- Kararsızlıkta önce konum, arka plan yansıması, hassasiyet, ortak GND, gevşek kablolar ve beslemeyi kontrol edin. Rastgele besleme kutbu değiştirmeyin. Multimetre olmadan çözülmüyorsa gerilim ölçümü veya teknik destek alın; yalnızca yazılım kilidini artırmak kesin çözüm değildir.
- D2'deki kısa darbeler kesmeyle yakalanabilir, 100 ms aralıklı ekran her darbeyi göstermeyebilir. Boş alanda ölçüm başlaması da kararsızlık belirtisidir. Kopuk sensör çıkışı pull-up nedeniyle `ENGEL YOK` görünebilir; bu yazı sensörün sağlam/bağlı olduğunu tek başına kanıtlamaz.
- `TimingCore.h` içindeki **`MIN_GAP_US = 500000`** (0.5 s), robotun tek geçişindeki parçalı algılamalardan uzun ve en kısa gerçek turdan kısa olmalıdır. `CLEAR_US = 30000` (30 ms), yeniden kurma için engelsiz bekleme süresidir. Bunlar tüm parazitleri süzmez; ilk algılama kenarı ayrıca süre filtresinden geçirilmez.
- Tek MZ80 yön/robot kimliği ayırt etmez. Yansımalı algılamada gövde rengi, açı ve mesafe tetikleme konumunu değiştirebilir. Daha tekrarlanabilir bir ölçüm çizgisi gerekiyorsa karşılıklı verici–alıcılı ışık bariyerini değerlendirin.
- Süre `dakika:saniye.milisaniye` biçimindedir. D2 kesmesindeki `micros()` zaman damgası kullanılır; ekran yaklaşık 100 ms'de bir kontrol edilir. Gösterim çözünürlüğü, sensörün tepki süresi ve Uno saat toleransı dahil mutlak doğruluk garantisi değildir.
- Ölçüm 60 dakikada zaman aşımına düşer ve kaydedilmez. Çekirdek, `micros()` taşmasını işaretsiz fark hesabıyla ele alır.

## Yazılım doğrulaması

`tests/timing_core_test.cpp` başlatma/durdurma, erken tetiklemeyi reddetme, son/en iyi kayıtları, reset, sayaç taşması ve zaman aşımını test eder.

`tests/mz80_sketch_test.cpp`, gerçek `.ino` dosyasını `tests/stubs` içindeki masaüstü GPIO/LCD modelleriyle çalıştırır: aktif-LOW algılama, pull-up, engelsiz bekleme, sürekli engelin tekrar sayılmaması, buton reseti, süre biçimi ve yalnızca değişen karakterlerin çizilmesini kontrol eder. Bu modeller yalnızca test derlemesinde kullanılır; Arduino kütüphanelerinin yerine karta yüklenmez.

C++11 veya üstü derleyici ile PowerShell'de proje kökünden:

```powershell
g++ -std=c++11 -Wall -Wextra -Werror -pedantic tests/timing_core_test.cpp -o "$env:TEMP\timing_core_test.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\timing_core_test.exe" }
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -Itests/stubs tests/mz80_sketch_test.cpp -o "$env:TEMP\mz80_sketch_test.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\mz80_sketch_test.exe" }
```

Arduino IDE'de ayrıca Uno için derleme yapın. Yazılım testleri fiziksel sensör, besleme ve ekran denemelerinin yerine geçmez.
