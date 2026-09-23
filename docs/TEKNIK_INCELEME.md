# ATLAS — teknik inceleme

Tarih: 16 Eylül 2026. Kapsam: yerel üretici kodu, MEBSTART LED sketch'i, kronometre kaynak/test/belgeleri, kılavuz ve pist PDF'leri. **Statik inceleme ve derleme, fiziksel robotta doğrulama anlamına gelmez.** Kaynaklar değiştirilmedi.

## 1. Platform ve sürüm ayrımı

İngilizce kılavuz s.5–7: ATmega328P-AU, 16 QRE1113GR sensör, BTN9960LV sürücüler, 78M05 regülatör, üç adet 7.4 V coreless 1023 motor; iki sürüş motoru ve bir türbin. Ölçü 17 × 24.5 × 4.5 cm, pilsiz ağırlık 93 g olarak belirtilmiş.

Kılavuz Ocak 2026 ve bazı bölümleri `ATLAS_1.4.1.ino` adını kullanıyor; yerel firmware başlığı Mart 2026 / 1.4.3. Değişken adlarını kılavuzdan kopyalamak yerine mevcut koddan doğrulamak gerekir. Güncel [üretici sayfası](https://exoticteam.com.mx/atlas) eski N60 motorlu/110 g tasarımla yeni Rev. 1.4 bilgilerini bir arada sunuyor. Eski bölümdeki akım, motor veya ağırlık değerleri 1.4.3'e doğrudan taşınmamalı.

İki dilli PDF'deki “suction”, “calibration”, “autonomy” gibi terimlerin Türkçe karşılıkları yer yer hatalı. Isıtma/yerleştirme gibi fiziksel işlem tariflerinde İngilizce metni esas alın; belirsiz işlemi üreticiye sorun.

## 2. Üretici firmware'inin çalışma modeli

### Akış

`setup → UI/sensör/motor başlatma → açılış animasyonu → isteğe bağlı debug → çizgi rengi → kalibrasyon → READY/GO bekleme → run → disableRobot → reset gerektiren bekleme`.

- SW1: beyaz çizgi; SW2: siyah çizgi. Kalibrasyon her reset/açılışta yeniden yapılır.
- SW1/SW2 ile PWM 60–250 arasında 10'ar adımla seçilir; varsayılan 100.
- READY aktif-HIGH olduğunda türbin 1000 ms rampayla hazırlanır.
- GO aktif-HIGH olduğunda koşu başlar. Kod önce türbin komutunu sıfırlar, ardından koşuda 250 ms türbin rampası uygular; sürüş rampası 150 ms.
- GO düşünce 500 ms duruş aşaması başlar, sonra motorlar/türbin kapatılıp sonsuz bekleme animasyonuna geçilir.
- Bu bekleme **elektriksel kapanma değildir**; regülatör beslenmeye devam eder.

### Kontrol

16 sensör analog çoklayıcıyla okunuyor. Sensör başına min/max/eşik kalibrasyonu, eşik üzerindeki değerlerin 0–255 aralığına ölçeklenmesi ve ağırlıklı ortalama mevcut. Çizgi konumu 0–15000, merkez 7500.

Kontrol PD: `KP=0.05`, `KD=0.35`; türev terimi zamanla bölünmüş hız değil, ardışık örnekler arasındaki hata farkı. Kontrol döngüsünün hedefi 750 µs, yaklaşık 1.33 kHz. Döngü gecikmesi değişirse aynı KD farklı davranır. Bu hedefin fiziksel olarak her çevrimde tutturulduğu ölçülmedi.

Motor komutları ileri 200, geri -120 ile sınırlı. Hız menüsünün 250'ye çıkması motora 250 uygulanacağı anlamına gelmez; özellikle düzde 200 üstü taban komutu doygunluğa takılır ve viraj kontrol payını azaltır. Enkoder/hız geri beslemesi yok; PWM, m/s değildir. Pil gerilimi, yük, lastik ve zemin aynı PWM'deki hızı değiştirir.

Timer1 sürüş PWM'i, Timer2 türbin PWM'i için doğrudan yapılandırılıyor. Standart `analogWrite`, Servo veya zamanlayıcı kullanan başka kütüphaneler bu yapılandırmayla çakışabilir. 16 MHz varsayımında Timer1 yaklaşık 20.05 kHz, Timer2 yaklaşık 31.37 kHz; donanım ölçümü değildir. Timer0, Arduino zaman işlevleri için korunmuş.

## 3. Üretici kodunda bulgular

### P0 — çizgi kaybı zamanında başlatılmamış değişken

**Yer:** `RunControl.ino:205`, `updatePeriod()` içindeki offline dalı.

Mevcut ifade `const unsigned long offlineElapsedTime = currentTime - offlineElapsedTime;`. Yerel değişken başlangıç değeri atanırken yine kendisi okunuyor; davranış tanımsız. AVR derlemesi `-Wmaybe-uninitialized` uyarısı verdi.

Amaçlanan referans, son geçerli çizgi zamanını tutan `offlineStartTime`. Düzeltme tasarımı: fark `currentTime - offlineStartTime` üzerinden hesaplanmalı. İlk offline anı, 34/35/36 ms sınırları, çizgiyi yeniden bulma ve `millis()` taşması regresyon testine alınmalı. Bu dosyada düzeltme uygulanmadı.

### P0 — start modülü arayüzü MEBSTART ile aynı değil

**Yer:** `UI.ino/readReady`, `readGo`; `RunControl.ino/run`.

Üretici D3/PD3 READY ve D4/PD4 GO girişlerinde aktif-HIGH seviye bekliyor. Bu girişler için kodda dahili pull-up/pull-down tanımlanmıyor; dış devrenin onları belirli seviyede tutması gerekiyor. MEBSTART satıcısı 5 V bekleme, 0 V tetikleme belirtiyor. Doğrudan mevcut GO okumasıyla birleştirmek bekleme seviyesini START olarak yorumlatabilir.

Tek MEBSTART çıkışı iki bağımsız READY/GO sinyali değildir. Ön-vakumun tek sinyalden yazılımla türetilmesi ayrı bir entegrasyon işidir. Önce yüksüz LED doğrulaması ve kontrollü stop testi gerekir.

### P1 — çizgi yokken sınırsız arama ve duruş davranışı

**Yer:** `RunControl.ino/updatePeriod`, `updatePWMDecrementRamps`.

Offline dalı dış tekerleğe 200, iç tekerleğe ilk 35 ms için -120, sonrasında 0 komutu hedefliyor. Kalıcı çizgi kaybına özel zaman aşımı yok. GO aktif kaldığı sürece robot aramaya devam edebilir.

Duruş rampası `velPWMDecrement` üzerinden online dalını etkiliyor; offline dalı bu azaltmayı kullanmıyor. Dolayısıyla GO bırakıldıktan sonra yaklaşık 500 ms boyunca offline kurtarma komutları devam edebilir. “Stop = anında durur” varsayımı yanlış. Kontrollü yavaşlatma, çizgi kaybı ve acil duruş öncelikleri ayrı tanımlanmalı; gerçek fren mesafesi ölçülmeli.

Öneri: kalıcı çizgi kaybı limiti, koşu süresi limiti ve tüm dallardan öncelikli stop/lockout. Kesikli pist nedeniyle her anlık çizgi kaybını aynı anda kilitlemek de uygun olmayabilir; izinli boşluk stratejisi ölçüm ve şartnameye göre belirlenmeli.

### P1 — uç sensör yönü eski kalabilir

`position == 0` veya `15000` iken doğrudan offline dalına giriliyor. `lastDetectedSide` güncellemesi yalnızca normal dalın içinde. Yalnız en uç sensörün çizgiyi ilk kez gördüğü sıçramalı bir durumda kurtarma yönü önceki tarafta kalabilir. Bu her turda kesin hata değildir; özellikle 0/15'in tek başına etkin olduğu dizilerle test edilmeli. Çizgiyi bulduktan sonraki ilk türev darbesi de incelenmeli.

### P1 — kalibrasyon kalitesi doğrulanmıyor

`calibrateSensors()` düğme hâlâ basılıysa örnek toplama döngüsünü atlayabilir. Tüm sensörler için yeterli açık/koyu kontrast alındığını ve `max > min` olduğunu onaylayan kilit yok. Yetersiz kalibrasyon ile koşu başlayabilir.

Öneri: düğmenin bırakılmasını doğrulama, minimum örnekleme ve sensör başına kontrast kontrolü, başarısız kalibrasyonda start izni vermeme. **Sırf `range` sıfır olabilir diye kesin sıfıra bölme var denmedi:** mevcut koşul dallarının bölmeye ulaşma koşulları ayrıca değerlendirilmelidir. Asıl doğrulanmış eksik kalibrasyon geçerlilik kontrolüdür.

### P1 — bellek payı dar

Derleme: global/statik SRAM 1557/2048 bayt; kalan 491 bayt stack ve dinamik ayırmalar arasında paylaşılır. Debug çıktılarında `F()` kullanmayan çok sayıda metin ve `String` birleştirmesi var. Derleme özeti dinamik tepe bellek kullanımını kanıtlamaz.

Öneri: debug sabit metinlerini flash'ta tutma, geçici `String` yerine ardışık `Serial.print`, koşu döngüsünde bloklayan loglardan kaçınma. Kazanım yeniden derlenip ölçülmeli; rastgele kütüphane veya tam ekran tamponu eklenmemeli.

### P2 — diğer noktalar

- Hız değiştirme sırasında toplam 500 ms `delay` var; GO bu sırada geç fark edilebilir.
- READY ile önceden hızlanan türbinin GO anında sıfırlanıp 250 ms rampaya alınması emiş kuvvetini geçici azaltabilir. Amaçlı olup olmadığı üreticiye sorulmalı; fiziksel ölçüm olmadan kaldırılmamalı.
- Offline dalında türbin çıkışı yeniden hesaplanmıyor; son komut kalıyor, başlangıçta çizgi yoksa ilk değer 0 olabilir.
- `readOtherADCs()` tanımlı ama koşuda kullanılmıyor; doğrulanmış pil düşük-gerilim kesmesi yok. ADC6/7 bağlantıları şema olmadan varsayılmamalı.
- `Sensors.ino` ADC yorumu “right adjusted” diyor, fakat `ADLAR=1` ve `ADCH` okunması sola hizalı 8 bit okumaya karşılık geliyor. Bu yorum hatasıdır; bitleri yoruma uydurmak için değiştirmeyin.
- `runInit()` bazı durumları resetlemiyor; mevcut tek-koşu/reset mimarisinde sınırlı etki, tekrar koşu özelliği eklenirse tüm kontrol/stop durumları baştan kurulmalı.

## 4. MEBSTART LED testi

**Derleniyor, ancak üretici kodunun birebir testi değil.**

Test Uno D2 aktif-LOW, üretici ATLAS D4 aktif-HIGH kullanıyor. Test START'tan sonra sabit 1 saniye LED yakıyor; üretici ayrı READY sinyalinde ön hazırlık yapıyor. LED gerçek PWM rampası, türbin devri veya emiş kuvvetini ölçmüyor. Testin duruşu LED'leri kapatıyor; üreticide 500 ms kontrollü duruş var.

Satıcı sayfası aktif-LOW varsayımını destekliyor; gerçek modülde açılış, öğrenme, START, STOP ve güç kesilmesi seviyeleri yine ölçülmeli. Kod yorumundaki “birebir korunmuştur” ifadesi bu iki dosya için doğru değil.

Geliştirme gereksinimleri:

- Açılışta giriş LOW ise yeni komut almadan süreç başlıyor. Önce geçerli pasif seviye görülmesi, ardından yeni start geçişi aranmalı. `INPUT_PULLUP`, takılı kalmış LOW için koruma değildir.
- `DEBOUNCE_MS=100`, kararlı seviye doğrulaması değil; vakumun ilk 100 ms'sinde iptali kontrol etmeyen bir süre. Bu pencereye denk gelen kısa STOP kaçabilir.
- `elapsed % 200 < 10` bir kez çalışmayı garanti etmiyor; 9600 baud seri çıktı bloklaması zamanlamayı ve stop gecikmesini etkileyebilir. Son rapor zamanı tutulmalı.
- LED testinde fiziksel motor, sensör, READY arayüzü ve emiş ölçümü yok. Bu sketch ATLAS'a yüklenmemeli: ATLAS'ta D2 LED0, D3 READY, D4 GO olarak kullanılıyor.

Hedef entegrasyon: WAIT_IDLE → ARMED → PREVACUUM → RUNNING → STOPPING/LOCKED; STOP tüm etkin durumlarda öncelikli. Bu tasarım önerisidir, uygulanmış bir özellik değildir.

## 5. Kronometre

### Güçlü yönler

`TimingCore.h` donanımdan ayrılmış; ilk geçiş başlatıyor, ikinci geçiş bitiriyor. Reset son/en iyi süreyi koruyor; güç kesilmesi siliyor. Bitişte tekrar başlamıyor. 500 ms minimum aralık, 60 dakika zaman aşımı ve işaretsiz farkla sayaç taşması kontrolü mevcut.

ISR ekran/Serial kullanmadan `micros()` zamanını alıyor. Çok baytlı paylaşılan zaman atomik bölümde okunuyor. LCD tam ekran tamponu kullanmıyor; değişen karakterler çiziliyor. Masaüstü testleri gerçek `.ino` dosyasını GPIO/LCD modelleriyle çalıştırıyor.

### Yeniden üretilen açık: 30 ms gerçekten kesintisiz değil

**Yer:** `robot_kronometre.ino/onObjectDetected`, `serviceSensor`.

`clearTracking` yalnızca ana döngü sensörü LOW görürse temizleniyor. Yeniden kurulma beklenirken `captureEnabled=false`; ISR bu sırada gelen düşen kenarı kaydetmiyor.

Ek masaüstü senaryosu:

1. 470000 µs: HIGH görülüyor; boşluk takibi başlıyor.
2. 490000 µs: LOW kenarı ISR'ı çağırıyor, capture kapalı olduğu için kaydedilmiyor.
3. 495000 µs: giriş tekrar HIGH; arada `serviceSensor()` çalışmadı.
4. 500000 µs: servis, 470000'den 30 ms geçtiği için kuruluyor; oysa gerçek kesintisiz HIGH yalnızca 5 ms.

Çalıştırılan gözlem çıktısı: `Armed after only 5 ms of actual clear time: 1 (configured CLEAR_US=30000)`.

Bu durum geçişlerin mutlaka yanlış ölçüldüğünü kanıtlamaz; belgelerdeki kesintisiz 30 ms garantisinin desteklenmediğini gösterir. Öneri: capture kapalıyken de LOW geçişlerini izlemek, HIGH süresini doğrulanmış son açılmadan hesaplamak; gerekirse CHANGE kesmesiyle iki kenarı takip etmek. Atomiklik, bekleyen ilk geçiş zamanının korunması ve reset davranışı için testler eklenmeli. ISR'da LCD işlemi yapılmamalı.

### Ölçüm sınırları

- 500 ms'den kısa tur ölçülmez; kısa segment testinde bu sınır bilinmeli.
- İlk darbe ayrıca minimum LOW süre filtresinden geçmiyor; tek parazit ölçümü başlatabilir. Filtre eklenirse en yüksek hızdaki gerçek darbe süresiyle birlikte tasarlanmalı.
- Tek MZ80 yön/robot kimliği ayırt etmez. Geri dönüş veya el geçişi tur sayılabilir.
- MZ80 dijital yansımalı engel sensörüdür; analog mesafe ölçümü üretmez. Yüzey rengi/açı/mesafe tetikleme noktasını değiştirir.
- Ekranda milisaniye gösterilmesi ±1 ms doğruluk değildir. Standart 16 MHz AVR'de `micros()` çözünürlüğü yaklaşık 4 µs; sensör gecikmesi ve saat toleransı ayrıca vardır. 100 ms ekran yenilemesi ise ISR zaman damgasını doğrudan 100 ms'ye yuvarlamaz.
- Tek kapı tur süresini ölçer; START komutundan ilk kapı geçişine kadarki kalkış/ön-vakum sürelerini ölçmez. Yarışma süre tanımı farklıysa sonuçlar birebir karşılaştırılamaz.

## 6. Pist PDF incelemesi

**Kullanıcı düzeltmesi:** basılı pist 200 × 280 cm ve elinizde; yanlış ölçülü PDF paylaşılmış, kullanıcı basılı pistte sorun olmadığını belirtiyor. Aşağıdaki ölçüler ve geometri bulguları yalnız incelenen dosyalara aittir. Gerçek baskının hatalı veya robotla uyumsuz olduğunu kanıtlamaz; gerçek parkura özel değerlendirme için doğru dosya/görsel gerekir.

PDF metni ve vektörleri PyMuPDF ile okundu; fiziksel baskı ve yüzey test edilmedi.

- `pist1.pdf`: 2000.00 × 3000.00 mm sayfa; 65 vektör yol nesnesi. 36 stroke nesnesinin kalınlığı yaklaşık 20.001 mm. Diğer nesneler dolgu/birleşim/işaret öğelerini de içeriyor; nesne sayısı tur uzunluğu değildir.
- `pist2.pdf`: yaklaşık 2000.25 × 3000.02 mm sayfa; 359 çizgi/eğri öğesinden oluşan tek siyah dolgu yol nesnesi. Stroke kalınlığı tanımlı değil; buradan 20 mm çizgi eni çıkarılamaz. Tur uzunluğu/minimum yarıçap hesaplanmadı.
- Kronometre notlarındaki **200 × 280 cm**, kullanıcının gerçek baskı ölçüsüyle uyuşuyor. PDF'nin yaklaşık 200 × 300 cm olması yanlış ölçülü dosya paylaşımıyla açıklandı; sıkıştırma/kırpma hesabı gerçek baskıya ilişkin bulgu olarak kullanılmamalı.
- `pist1` sol tarafta ardışık yaklaşık `(205,1626) → (488,1343) → (205,1060) → (488,777) → (205,494)` mm noktalarıyla zigzag oluşturuyor. Ortak köşelerde doğrultu 90° değişiyor. Bu, geniş yaylı bir hız testiyle aynı dinamik zorluk değildir.
- Sağ üstte x≈1800 mm üzerinde kesikli segmentler var. İki komşu segmentin merkez çizgisi uçları arasında yaklaşık 55.7 mm açıklık bulunuyor; stroke uç kapakları gerçek beyaz boşluğu değiştirebilir. Tek başına bu ölçüm gerçek geçiş başarısı göstermez.
- Kılavuz s.29 tamamen düz zemin ve 1.5 m'den büyük çaplı dönüş koşullarını belirtiyor. `pist1` keskin köşeleri bu geniş dönüş koşulunu sağlamıyor. “Bu pistte kesin çalışmaz” denemez; fakat tam hız uyumluluğu varsayılamaz.

Dekota taşıyıcıdır; emişi belirleyen baskı/kaplama yüzeyi, ek yerleri, düzlük ve etek temasıdır. Arkalı önlü baskıda altta kalan yüzün çizilmesini, levhanın eğilmesini ve tozla tutuş değişimini önleyin. Bir yüzde yapılan sensör kalibrasyonu diğer yüzde yeniden yapılmalı. Çıplak gözde siyah/beyaz görünüm IR kontrastını garanti etmez.

## 7. Gerçekleştirilen yazılım doğrulaması

Ortam: Windows PowerShell; Arduino IDE içindeki `arduino-cli.exe`; Arduino AVR Boards 1.8.8. Kronometre kütüphaneleri kurulu: Adafruit ILI9341 1.6.3, GFX 1.12.6, BusIO 1.17.4.

- **Üretici sketch'i — `arduino:avr:nano`:** başarılı; flash 11518/30720 bayt, SRAM 1557/2048 bayt. `RunControl.ino:205` başlatılmamış değişken uyarısı var.
- **MEBSTART testi — `arduino:avr:uno`:** başarılı; flash 3322/32256, SRAM 188/2048 bayt.
- **Kronometre — `arduino:avr:uno`:** başarılı; flash 14736/32256, SRAM 538/2048 bayt. Bu SRAM özeti kütüphanelerin bütün çalışma zamanı heap kullanımını kapsamaz.
- Arduino çekirdeğinin `new.cpp` dosyasında ayrıca kullanılmayan `tag` parametresi uyarıları görüldü; proje hatasıyla karıştırılmadı.
- `timing_core_test.cpp`: geçti.
- `mz80_sketch_test.cpp`: geçti; bunlar simüle GPIO/LCD testleridir.
- Ek kısa-darbe gözlemi: 30 ms boşluk açığını yeniden üretti; mevcut iki test bunu kapsamıyor.

Masaüstü testleri C++11, `-Wall -Wextra -Werror -pedantic` ile derlendi. Derleme ürünleri TEMP altında tutuldu. **Hiçbir karta yükleme yapılmadı.**

Tekrarlamak için PowerShell'de proje kökünden:

```powershell
$cli = "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
& $cli compile --fqbn arduino:avr:nano --warnings all .
& $cli compile --fqbn arduino:avr:uno --warnings all ATLAS_MEBSTART_Test
& $cli compile --fqbn arduino:avr:uno --warnings all hizli-cizgi-kronometre/robot_kronometre

g++ -std=c++11 -Wall -Wextra -Werror -pedantic hizli-cizgi-kronometre/tests/timing_core_test.cpp -o "$env:TEMP\atlas_timing_core_test.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\atlas_timing_core_test.exe" }
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -Ihizli-cizgi-kronometre/tests/stubs hizli-cizgi-kronometre/tests/mz80_sketch_test.cpp -o "$env:TEMP\atlas_mz80_sketch_test.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\atlas_mz80_sketch_test.exe" }
```

CLI yolu bu bilgisayara özgü kurulum yeridir. Bu komutlar derler/test eder, yükleme yapmaz.
