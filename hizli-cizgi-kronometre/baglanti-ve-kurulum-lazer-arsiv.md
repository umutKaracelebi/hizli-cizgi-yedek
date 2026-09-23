# Arşiv — lazer–BPW34–LM393 bağlantısı

> Bu belge eski lazer devresini saklar; aktif kurulum değildir. Güncel MZ80 bağlantısı için `baglanti-ve-kurulum.md` dosyasını kullanın. Aşağıdaki ekran metinleri eski yazılıma aittir; güncel kod MZ80 için hazırlanmıştır. İki sensör çıkışını aynı D2 hattına bağlamayın.

## Düzen ve çalışma

Lazer ile fotodiyot robotun geçiş koridorunun iki tarafına sabitlenir; 200 × 280 cm pistin tamamını kesmeleri gerekmez. Robotun gövdesinin kesin keseceği bir yükseklik seçin. Tek ışın yönü veya robot kimliğini ayırt etmez: el, başka robot veya geri geçiş de algılanabilir.

İlk ışın kesilmesi başlatır, ikinci kesilme durdurur. Yeni ölçüm için butona basılır. Son ve en iyi süre RAM'de tutulur; buton bunları silmez, güç kesilmesi siler. Bilgisayar gerekmez. Robotu yerleştirirken ışını keserseniz başlangıç sayılır; robotu yerleştirdikten sonra butonla yeniden hazırlayın.

5 mW kırmızı lazer göz için tehlikelidir. Göze doğrultmayın; yansımalardan kaçının. Işın yolunu erişimden koruyun, alıcının arkasında mat ve opak bir ışın durdurucu kullanın. Ayarı ışına bakarak yapmayın. Korunamayan, insanların eriştiği bir ışın yolu yerine uygun kapalı bir ışık bariyeri tercih edin.

## Ekran

Bu bağlantı yalnızca paylaşılan Waveshare 2.4inch LCD Module içindir. Üretici, 5 V beslemede 5 V lojik çalışmayı ve Uno bağlantısını açıkça belirtir; aynı sürücüyü kullanan başka çıplak ekranlar buna uygun olmayabilir.

- VCC → Uno 5V
- GND → Uno GND
- DIN → Uno D11 (MOSI)
- CLK → Uno D13 (SCK)
- CS → Uno D10
- DC → Uno D7
- RST → Uno D8
- BL → Uno D9

Kabloların renklerine veya konnektör sırasına değil, ekran üzerindeki pin etiketlerine göre bağlayın. MISO bağlantısı gerekmiyor. Uno'nun sınırlı RAM'i nedeniyle kod tam ekran görüntü tamponu ayırmaz.

Üretici: https://www.waveshare.com/wiki/2.4inch_LCD_Module
Ürün: https://www.robotistan.com/24-inc-lcd-ekran-modulu

## LM393 DIP-8 pinleri

Entegreye üstten, çentik yukarıda bakıldığında:

```text
            çentik
           +--U--+
 OUT1   1  |     |  8  VCC
 IN1-   2  |     |  7  OUT2
 IN1+   3  |     |  6  IN2-
 GND    4  |     |  5  IN2+
           +-----+
```

Bağlantıları güç kapalıyken yapın. Entegreyi breadboard'un orta kanalını köprüleyecek şekilde yerleştirin.

## Lazer ve fotodiyot devresi

Lazerin üretici tarafından belirtilen + besleme ucunu 5V'a, - ucunu GND'ye bağlayın. Bu, sürücülü 5 V lazer modülüdür; Arduino sinyal pininden beslemeyin. Gövdeli modülün uçlarını renk varsayımı yerine etiketi/belgesiyle doğrulayın.

Fotodiyot için K = katot, A = anot. BPW34 bacaklarını elinizdeki üreticinin pin çizimiyle veya multimetre diyot testiyle belirleyin; yalnızca bacak uzunluğuna güvenmeyin.

### Eldeki parçalarla ön deneme

Aşağıdaki şema, mevcut 10 kΩ dirençlerle, **1 MΩ direnç ve kondansatör olmadan ön deneme** içindir. 1 MΩ direnç çalışmanın zorunlu koşulu değildir; eşik çevresindeki kararsızlığı azaltır. 100 nF besleme kondansatörü olmadan da devre çalışabilir, ancak parazite karşı kararlılık garanti edilmez; son kurulumda eklenmesi önerilir.

**OUT ile SENSE arasına hiçbir bağlantı yapmayın: kablo, 10 kΩ veya 220 Ω ile köprülemeyin.** Şemadaki diğer iki 10 kΩ direnç gereklidir. Eldeki 220 Ω dirençler bu devrede kullanılmıyor. Kondansatörün yerine de kablo veya direnç takmayın; LM393 pin 8'in 5V ve pin 4'ün GND besleme bağlantıları aynen kalır.

```text
Uno 3.3V -------- BPW34 K
                  BPW34 A ------- SENSE ------- LM393 pin 3 (+)
                                    |
                                   10k
                                    |
                                   GND

Uno 3.3V ------- trimpot dış uç
GND ----------- trimpot diğer dış uç
trimpot sürgü ------------------------------- LM393 pin 2 (-)

Uno 5V -------------------------------------- LM393 pin 8
GND ----------------------------------------- LM393 pin 4
Uno 5V -------- 10k -------- OUT ------------- LM393 pin 1
                              |
                              +--------------- Uno D2

OUT ile SENSE arasında geri besleme bağlantısı YOK.
Bu ön denemede kondansatör bağlı değil.

LM393 pin 5 ----------------------- GND
LM393 pin 6 ----------------------- Uno 3.3V
LM393 pin 7 ----------------------- bağlantısız
```

- Tüm GND hatları ortaktır.
- Trimpot sürgüsünü üretici çiziminden veya direnç ölçümünden belirleyin. Dış uçlar arasında yaklaşık 10 kΩ sabit, sürgü ile dış uçlar arasında ayarlanabilir direnç bulunur.
- Fotodiyot ve trimpotun 3.3V'a bağlanması bilinçlidir: 5 V ile çalışan klasik LM393 girişleri pozitif besleme rayına kadar ölçüm yapamaz. SENSE ve eşik gerilimini yaklaşık 0–3.3 V aralığında tutuyoruz.
- Ön denemede 1 MΩ geri besleme olmadığı için histerezis yoktur; eşik çevresinde çıkış titreşebilir. Trimpotu ışın açık/kapalı gerilimlerinin arasına, her iki değerden de pay bırakarak ayarlayın.
- Pin 1 açık kolektör çıkıştır; 5 V'a bağlı 10 kΩ pull-up şarttır.
- Lazer alıcıya vururken OUT yaklaşık 5 V, ışın kesikken yaklaşık 0 V olmalıdır. Yazılım düşen kenarı yakalar.

### Parçalar tamamlandığında önerilen eklemeler

Gücü kapatarak aşağıdaki parçaları ekleyebilirsiniz; bunlar yukarıdaki ön deneme şemasında bağlı değildir:

- **1 MΩ direnç:** OUT (LM393 pin 1) ile SENSE (pin 3) arasına. Eşik çevresindeki titreşimi azaltan küçük bir histerezis sağlar. Ekledikten sonra trimpot ayarını ve geçiş testlerini tekrarlayın.
- **100 nF seramik kondansatör:** LM393 pin 8 (5V) ile pin 4 (GND) arasına, entegreye mümkün olduğunca yakın. Besleme parazitlerini azaltır; son kurulum için önerilir. Kutuplu değildir.
- **İsteğe bağlı 10 µF kondansatör:** + ucu 5V, - ucu GND. Bu parça 100 nF yerel bypass kondansatörünün yerine düşünülmemelidir.

Bu eklemeler veya ön denemede eksik bırakılmaları Arduino kodunda değişiklik gerektirmez. Yazılımdaki tetikleme kilidi, donanımsal parazit ve eşik kararsızlığının tamamını gidermez.

10 kΩ fotodiyot yükü başlangıç değeridir, her ortamda garantili bir optik tasarım değildir. Direkt güneş, hizalama, fotodiyot yüzeyi ve ışık miktarı ölçülerek doğrulanmalıdır. Lazerin odak noktasını alıcının arkasındaki ışın durdurucudan taşırmayın. Fotodiyodu içi mat koyu renkli kısa bir tüple ortam ışığından koruyun. Uzun sensör kablolarından kaçının; LM393'ü fotodiyoda yakın tutun.

MZ80 ve KY-022 bu devreye bağlanmıyor. Bunlar ayrıca alınması gereken parçalar değil; mevcut yedeklerdir.

## Buton ve besleme

- Normalde açık anlık butonun bir kontağı → D4, diğer kontağı → GND.
- Kod INPUT_PULLUP kullanır, ek buton direnci gerekmez.
- Dört bacaklı butonda aynı taraftaki bazı bacaklar içeriden bağlı olabilir; bastığınızda birbirine bağlanan iki ayrı kontağı kullanın.
- Uno'yu regüle 5 V USB adaptörü veya powerbank ile USB soketinden besleyin. Uno'nun VIN/barrel girişine 5 V vererek çalıştırmayı tercih etmeyin.
- Powerbank düşük yükte otomatik kapanmamalıdır. Harici 5 V hattıyla bilgisayar USB beslemesini gelişigüzel paralel bağlamayın.

## Kod yükleme

Arduino IDE Kütüphane Yöneticisi'nden **Adafruit ILI9341** ve **Adafruit GFX Library** kurun; istendiğinde **Adafruit BusIO** bağımlılığını da kurun. Depo başlangıçta boş olduğu için önceden tanımlı Arduino bağımlılığı bulunmuyor.

`robot_kronometre/robot_kronometre.ino` dosyasını açın. `TimingCore.h` aynı klasörde kalmalıdır. Kart olarak **Arduino Uno**, port olarak kartın bağlı olduğu portu seçip derleyin ve yükleyin. Klon kart port olarak görünmüyorsa USB-seri çipini belirleyin; CH340 olması yaygındır ama her klonda aynı değildir.

Ekran yazıları yerleşik fontla uyum için Türkçe karakter kullanılmadan yazılmıştır. Süre biçimi `dakika:saniye.milisaniye` şeklindedir. Ekran yaklaşık 100 ms aralıklarla yenilenir; geçişin zaman damgası D2 kesmesinde `micros()` ile alınır. Milisaniye gösterimi, ±1 ms mutlak doğruluk garantisi değildir; optik düzen, eşik ve Uno saatinin toleransı da etkilidir.

## Eşik ayarı ve deneme

1. Robot yokken lazeri fotodiyot üzerine sabitleyin; ekranın çalıştığını kontrol edin.
2. Multimetreyle SENSE–GND gerilimini ışın açıkken ve opak bir cisimle kapalıyken ölçün. Açıkken daha yüksek olmalıdır. İki durumda da benzerse trimpotla çözmeye çalışmadan hizalamayı, pinleri ve ortam ışığını düzeltin.
3. Trimpot sürgüsünü iki ölçüm arasında bir gerilime ayarlayın. Ekran açık ışında `ISIN: ACIK`, kapalı ışında `ISIN: KESIK` göstermelidir.
4. Butona basıp bırakın, ışını açık tutun ve `HAZIR` yazısını bekleyin.
5. Opak bir kartla kısa süre kesip çekin: ölçüm başlar. Işın geri geldikten ve en az 0.5 saniye geçtikten sonra yeniden kesin: ölçüm durur.
6. Işını sürekli kapalı tutmak ikinci bir geçiş üretmemelidir. Sonuçtan sonra kesmeler yeni ölçüm başlatmamalıdır.
7. Butonla yeniden hazırlayıp daha hızlı/yavaş ölçümler deneyin; son ve en iyi süreyi kontrol edin. Güç kesilince kayıtların silinmesi beklenir.
8. Gerçek robotla en yüksek hızda tekrarlı geçişler yapın. Kararlı çalışmayı doğruladıktan sonra parçaları titreşmeyecek biçimde sabitleyin.

### Eksik parçalarla kararlılık kontrolü

- Fotodiyot–LM393 ve besleme kablolarını kısa tutun. Robot motorlarını kronometrenin 5 V hattından beslemeyin; robotun motor beslemesi ayrı olsun.
- Butonla hazırladıktan sonra ışını kesmeden en az 30 saniye bekleyin: ölçüm kendiliğinden başlamamalı ve `ISIN: ACIK` sabit kalmalıdır.
- Bir kez kesip ışını kapalı tutun: ölçüm başlamalı ancak kendi kendine durmamalı; `ISIN: KESIK` sabit kalmalıdır.
- Gerçek kullanım ışığında ve robot motorları çalışırken, robot ışını kesmeden aynı kontrolleri tekrarlayın. Ekran kısa parazit darbelerini göstermeyebilir; kendiliğinden başlayan/duran süre de kararsızlık belirtisidir.
- Yanlış tetikleme varsa sonuçlara güvenmeyin. Hizalamayı, ortam ışığı siperini, eşik ayarını ve bağlantıları kontrol edin; 100 nF kondansatörü tamamlayın ve gerekirse 1 MΩ histerezis direncini ekleyip yeniden deneyin. Yalnızca yazılım kilit süresini artırmak kesin çözüm değildir.

`TimingCore.h` içindeki `MIN_GAP_US` varsayılan olarak 500000 µs (0.5 s). Bu süre robotun tek geçişindeki parçalı gölgelerden uzun, en kısa gerçek turdan kısa olmalıdır. `robot_kronometre.ino` içindeki `CLEAR_US` en az 30 ms kesintisiz açık ışın ister. Tek ışın, bu koruma süresinden sonra oluşan ilgisiz engelleri gerçek turdan ayırt edemez.

Bir ölçüm 60 dakikaya ulaşırsa zaman aşımına düşer ve kaydedilmez. `micros()` yaklaşık 71.6 dakikada taşar; kod taşma sınırını aşan kısa ölçümleri işaretsiz fark hesabıyla destekler.

## Yazılım testleri

`tests/timing_core_test.cpp`, ölçüm çekirdeğini Arduino'dan bağımsız test eder: başlatma/durdurma, erken tetiklemeyi reddetme, son/en iyi kayıtları, reset, sayaç taşması ve zaman aşımı. C++11 veya üstü bir derleyiciyle derlenebilir. Masaüstü testleri gerçek Uno derlemesinin, sensör testinin veya ekran testinin yerine geçmez.
