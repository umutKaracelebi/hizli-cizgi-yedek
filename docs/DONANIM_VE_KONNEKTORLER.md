# ATLAS — donanım, konnektör ve tedarik rehberi

Tarih: 16 Eylül 2026. Kılavuz referansları yerel İngilizce `ATLAS_Rev1.4_Manual_1.0_17012026(ENG).pdf` dosyasınadır. Belgelenmiş işlev, olası fotoğraf eşleşmesi ve fiziksel doğrulama birbirinden ayrılmıştır.

## 1. Bağlam: aynı görünen soketler aynı işi yapmaz

ATLAS'ın motor beslemesi, türbin beslemesi, programlama, seri haberleşme ve start modülü farklı arayüzlerdir. Soketin mekanik olarak oturması elektriksel uyumluluk kanıtı değildir. Kablo rengi, konnektör cinsiyeti veya fotoğraftaki sağ/sol sırası yerine kart üzerindeki pin etiketleri, üretici şeması ve multimetre ölçümü kullanılmalı.

Bağlantıları enerji kesikken kurun. Belirsiz pin çiftlerine deneme amacıyla pil bağlamayın. Regüle 5 V girişine doğrudan 2S/3S pil vermeyin. ATLAS motorları USBasp, USB–TTL veya Uno GPIO hattından beslenmez.

## 2. Konnektörlerin görevleri

### XT30 — robotun ana pil girişi

Kılavuz s.5: sürüş motorları ana Vs hattından, XT30 üzerinden besleniyor; 2S–3S desteği belirtilmiş. Bu şarj dengeleme soketi veya haberleşme portu değildir. Pilde farklı ana soket varsa doğru kutuplu ve uygun akım kapasiteli kablo gerekir. Adaptör, gerilimi dönüştürmez ve zayıf soketin akım kapasitesini artırmaz.

### Pil balans soketi — hücre uçları ve türbin beslemesi

Standart bir 2S balans bağlantısında 3, 3S bağlantısında 4 hat bulunur; bunlar toplam eksi ve ardışık hücre uçlarıdır. Yaygın gövde JST-XH'dir; eldeki pilin soket ailesi ve pin sırası ayrıca doğrulanmalı. Pin sayısı tek başına kutup sırasını söylemez.

Kılavuz s.5 ve 19 türbinin pilin balans soketinden **yalnız 2S gerilim** almasını istiyor. 3S pakette tüm paketin +/− uçları türbine bağlanmaz. Rastgele iki pin seçmeyin; hangi uçların robot devresiyle ortak olduğu üretici şemasıyla doğrulanmalı. Enerjili balans pinlerinde prob kaydırmak kısa devre yapabilir; uygun ölçüm aparatı veya deneyimli mentor desteği kullanın.

3S paketten yalnız iki hücreyi türbin için kullanmak, bu hücreleri üçüncüden daha fazla boşaltabilir. Paket toplam voltajı tek başına yeterli değil; hücre gerilimleri ayrı izlenmeli. Şarj sırasında robot ve türbin pilden ayrılmalı.

### ICSP / ISP — USBasp ile firmware yükleme

Kılavuz s.12–15: bilgisayar → USBasp → robot ICSP bağlantısı. İşlevsel hatlar MOSI, MISO, SCK, RESET, VCC ve GND'dir. Adaptör üzerinde 10→6 pin dönüşümü olabilir; fotoğraflardan eldeki pin dizilimi kesinleştirilmedi. Kırmızı şerit/pin-1 işareti, karşı kart etiketiyle birlikte kontrol edilmeli.

Arduino IDE'de üretici yöntemi: Arduino Nano / ATmega328P / USBasp / “Upload Using Programmer”. Seri COM port seçimi bu ICSP işleminin temeli değildir. Sıradan Upload düğmesiyle aynı yöntem değil. USBasp bir USB–TTL dönüştürücü veya pil şarj cihazı değildir.

USBasp'ın hedef beslemesini sağlayıp sağlamadığı ve pilin hangi durumda ayrılacağı kendi programlayıcı/kart şemasından doğrulanmalı; iki beslemeyi gelişigüzel paralel bağlamayın. `Burn Bootloader` normal firmware yüklemesinin yerine rastgele uygulanmamalı.

### UART — USB–TTL ile seri tanı

Kılavuz s.17 ve üretici kodu: bilgisayar → USB–TTL dönüştürücü → robot UART. Debug seri hızı **115200 baud**. Tipik bağlantı dönüştürücü TX → robot RX, RX → robot TX, GND → GND'dir. VCC bağlantısı ve lojik seviye kendi şemasına göre seçilmeli; jumper üzerindeki 3.3/5 V seçeneğinin yalnız besleme çıkışını mı yoksa UART seviyelerini de mi değiştirdiği varsayılmamalı.

USB–TTL, klasik ±RS-232 kablosu değildir. DTR otomatik reset için kullanılabilir; robotun DTR bağlantısı ve bootloader'ı doğrulanmadan UART üzerinden yükleme çalışır denemez. Üreticinin belgelenmiş ilk yükleme yolu USBasp'tır.

### Start modülü — kontrol sinyali

Kılavuz s.22: READY ve GO işlevleri var, fiziksel pin etiketleri kart üzerinde. Firmware'de READY=D3/PD3, GO=D4/PD4, ikisi de aktif-HIGH.

[MEBSTART satıcı sayfası](https://www.robotus.net/mebstart-meb-robot-yarismasi-baslatma-modulu): Sinyal/Eksi/Artı bağlantıları, 5 V besleme; beklemede 5 V, tetikte 0 V. 3.3 V uyumlu olmadığı belirtiliyor. Bu tek çıkış iki bağımsız READY/GO üretemez.

Kılavuz, ters polariteli start modülleri için USB–TTL adaptörü yanında panelize bir adaptörden söz ediyor. **Bu ifade, adaptörün aktif-LOW sinyali elektronik olarak terslediğini kanıtlamaz.** Yalnız pin/besleme sırası dönüştürücü de olabilir. Şeması görülmeden lojik uyumsuzluğu çözdüğü varsayılmamalı.

### FFC/FPC — sensör şerit kablosu

Kılavuz s.6 sensör bağlantısını FFC/FPC olarak gösteriyor. Bu ince düz kablodur; güç kablosu yerine kullanılmaz. Kilit mekanizması, temas yüzü, pin adedi ve adım aralığı üreticiye göre doğrulanmalı. Kilidi açmadan kabloyu çekmeyin; enerjili söküp takmayın.

## 3. Altı konnektör fotoğrafından elde edilenler

Görüntüler dosya aracıyla doğrudan açılamadı. Windows OCR ile orijinal ve döndürülmüş/büyütülmüş okumalar yapıldı. Sonuçlar parça rolü için ipucu verir; lehim, pin sırası, yonga modeli veya polariteyi doğrulamaz.

- **connector1 ön/arka:** ön yüzde 180° döndürülmüş OCR `USBASP X2.o` okudu. **USBasp programlayıcı olduğu güçlü adaydır**; ICSP yükleme işleviyle eşleşir. Model yazımı, jumper ve kablo pin-1 yönü ayrıca kontrol edilmeli.
- **connector2 ön/arka:** arka yüzde `VCC`, `GND` ve bozulmuş `USB ... TTL` yazısı okundu. **UART/USB–TTL bağlantı adaptörü adayıdır.** Üzerinde aktif USB–seri dönüştürücü bulunup bulunmadığı, yoksa yalnız pasif ara kart olduğu belirlenemedi.
- **connector3 ön/arka:** ön yüzde farklı OCR geçişlerinde `DTR`, RX benzeri yazı, `GND`, `VCC`, 3V/5V benzeri etiketler çıktı. **USB–TTL/UART modülü adayıdır.** Yonga modeli, jumper görevi ve pin sırası kesin değil.

Fotoğraf numarası üzerinden doğrudan “şu ucu şuraya tak” talimatı verilmedi. Kesin eşleşme için parçaları yazıları düz okunacak şekilde yakın çekim, fiş yönleri ve robot üzerindeki karşılık etiketleriyle birlikte belgelemek gerekir. Özellikle connector2'nin tüm pin etiketleri ve connector3'ün USB/entegre tarafı eksik bilgiyi tamamlar.

## 4. Pil adaylarının değerlendirmesi

### Kullanıcının ilettiği satıcı metni — 16 Eylül 2026 güncellemesi

Sayfalara doğrudan erişim 403 ile engellenmişti; aşağıdaki bilgiler artık kullanıcı tarafından kopyalanan satıcı açıklamasıyla mevcut. Bunlar bağımsız ölçüm değil, satıcı beyanıdır:

- **DOGCOM ürün kodu 107764:** 300 mAh, 100C, 3S, nominal 11.4 V; başlıkta açıkça HV LiPo ve XT30. Boyutlar satıcının kendi eksen adlarıyla uzunluk **16 mm**, genişlik **54 mm**, yükseklik **18 mm**. Eksenler tahminle yeniden sıralanmadı.
- **Tattu ürün kodu 106696:** 300 mAh, 75C, 2S, nominal 7.6 V, PH2.0; kimya alanında LiPo. Uzunluk **47 mm**, genişlik **17 mm**, yükseklik **13 mm**. Nominal 7.6 V, HV sürümü düşündürür; metin üst şarj limitini belirtmiyor.
- Her iki ilanda yeterli stok ve 1 iş günü teslim yazıyor. Bunun Türkiye'ye teslim süresi olduğu doğrulanmadı; anlık satıcı beyanı olarak kalır.
- İki pil için de ağırlık, balans soketi/pinleri, önerilen şarj akımı ve hücre başına üst şarj gerilimi metinde yok. Başlıktaki HV ifadesi tek başına 4.35 V'a şarj talimatı değildir.
- Robot elinizde, mevcut 2S pil B6 ile 8.4 V'a şarj edilerek kullanılmış. **Kullanılan pilin hangi model olduğu henüz bilinmiyor.** Standart 2S LiPo'nun toplam tam dolu gerilimi 8.4 V'tur; paket toplamı her hücrenin 4.2 V olduğunu tek başına kanıtlamaz.

Aşağıdaki elektriksel uygunluk değerlendirmesinde üst şarj sınırı ve robot üreticisi onayı gereken noktalar açık kalır.

### Tattu 300 mAh 7.6 V 75C 2S PH2.0

[Ürün bağlantısı](https://droneshop.nl/tattu-300mah-7-6v-75c-2s-lipo-accu-ph20). Site 403 verdi; kullanıcı daha sonra 106696 ürün kodlu teknik metni paylaştı. PH2.0, kapasite, nominal gerilim ve boyutlar artık satıcı beyanıyla destekleniyor; fiziksel etiket/şarj limiti henüz doğrulanmadı.

- 2S, ilk test için üreticinin önerdiği hücre sayısıyla örtüşür.
- 7.6 V nominal ifadesi LiHV kimyasına işaret ediyor; gerçekten LiHV olduğu ve hücre başı üst şarj gerilimi teyit edilmeli.
- PH2.0, robotun XT30 ana girişiyle doğrudan aynı soket değildir.
- 75C × 0.300 Ah = 22.5 A **etiket hesabıdır**, ölçülmüş sürekli çıkış veya PH2.0 soket kapasitesi değildir. Firmware başlığındaki 80C de tek başına minimum satın alma standardı sayılmamalı.
- Soket/kablo akım kapasitesi, voltaj düşümü, balans bağlantısı, ağırlık ve pil yuvası ölçüsü teyit edilmeden uygun ilan edilmemeli. Sadece adaptör almak bu sorunları çözmez.

**Öneri:** bütçe odaklı ilk pilde üreticinin onayladığı, XT30 ana soketli ve uygun balans çıkışlı standart 2S seçeneğini araştırmak, bu PH2.0 adayına körlemesine adaptör almaktan daha temiz bir başlangıçtır. Kesin marka/model önerilmedi.

### DOGCOM 300 mAh 100C 3S 11.4 V HV XT30

[Ürün bağlantısı](https://droneshop.nl/dogcom-300mah-100c-3s-114v-hv-lipo-xt30). Site 403 verdi; kullanıcı daha sonra 107764 ürün kodlu teknik metni paylaştı. HV/XT30, kapasite, nominal gerilim ve boyutlar artık satıcı beyanıyla destekleniyor; tam dolu gerilim ve robot uyumu henüz doğrulanmadı.

- XT30 adı mekanik ana soket eşleşmesi lehine; kutup, ağırlık, boyut ve kablo kapasitesi yine kontrol edilmeli.
- 3S HV, 2S başlangıç test pilinin yerine doğrudan önerilmiyor.
- Hücre başına 4.35 V ile şarj edilen bir LiHV ise toplam **13.05 V** eder. Bu değer, kılavuz s.5'te yazan 12.4 V çalışma aralığı üst sınırını aşar.
- Standart 3S LiPo tam şarjı da **12.6 V** olduğundan kılavuzdaki “3S destekli / 12.4 V üst sınır” ifadesi kendi içinde açıklama gerektiriyor. 12.4'ün yazım hatası olduğunu varsaymayın; üreticiye nominal/tam dolu değerleri açıkça sorun.
- 100C × 0.300 Ah = 30 A etiket hesabı; robotun gerçek ihtiyacı veya teslim kapasitesi ölçümü değil.

**Karar:** 3S HV için üreticiden yazılı onay ve pil üreticisinden şarj profili alınmadan satın alma/çalıştırma tavsiyesi verilmez. Türbinin 2S beslenmesi gereği aynen devam eder.

### LiPo / LiHV ve SB1

Standart LiPo için yaygın üst şarj değeri 4.20 V/hücre: 2S=8.40 V, 3S=12.60 V. 4.35 V/hücre etiketli LiHV için 2S=8.70 V, 3S=13.05 V. **Kesin ayar her zaman eldeki pilin etiket/datasheet'i ve şarj cihazı kılavuzuyla belirlenir.** LiHV desteği olmayan cihazda sırf pil HV diye hücre sayısı veya kimyayı yanlış seçerek yüksek voltaja çıkarmayın.

Kılavuz s.25, yalnız 2S kullanımında SB1 köprüsüyle ayrı türbin kablosunun kaldırılabildiğini söylüyor. Daha sonra 3S'e geçişte türbin aşırı gerilim riski yaratabilir. Takımın 2S/3S karışık planında SB1'e müdahale önermiyorum; üreticiyle şema ve geri dönüş işlemi doğrulanmalı.

Kılavuzdaki 8.1 V/12.1 V, **tutarlı performans için önerilen paket voltajlarıdır**; genel LiPo saklama gerilimi veya evrensel hücre koruma eşiği değildir. Düşük gerilim kesmesi firmware'de doğrulanmadı.

## 5. Şarj cihazı ve adaptör kararı

Aday ilanlar:

- [Candagrup iMAX B6 ilanı](https://www.n11.com/urun/candagrup-imax-b6-80w-adaptorlu-rc-lityum-pil-sarj-cihazi-18650-pil-sarj-18744849?magaza=oyuncakhobi)
- [Hubstein iMAX B6 ilanı](https://www.n11.com/urun/hubstein-imax-b6-dijital-sarj-cihazi-ac-dc-sarj-adaptoru-eu-42139510?magaza=oyuncakhobi)
- [12 V 7 A adaptör ilanı](https://www.hepsiburada.com/imax-b6-uyumlu-12v-7a-adaptor-12-volt-7-amper-84w-adaptor-trafo-pm-HBC000075Q57Q)

Doğrudan erişimler 403 verdi; kullanıcı daha sonra B6 açıklamasını paylaştı. **Satıcı metnine göre:** giriş 11–18 V, şarj gücü 50 W, deşarj 5 W; şarj akımı 0.1–5.0 A, deşarj 0.1–1.0 A; Li-ion/LiPoly/LiFe desteği, lityumda 1–6S, NiMH/NiCd'de 1–15 hücre, Pb'de 2–20 V. Balans, storage, zaman sınırı, giriş gerilimi izleme ve beş paket ayarı belleği belirtiliyor. Ağırlık 277 g, ölçüler 133 × 87 × 33 mm. Bunlar satıcı beyanı; eldeki cihaz üzerinde doğrulanmış özellikler değil.

Paylaşılan **kutu içeriğinde yalnız IMAX B6 şarj cihazı** var. Kullanıcıya göre diğer n11 seçeneği aynı satıcının adaptör eklediği paket; adaptörün özellikleri bulunamadı. Dolayısıyla adaptörün dahil olması kullanıcı beyanıyla netleşti, elektriksel uygunluğu henüz netleşmedi. Ayrıca ilan URL'sindeki 80 W ile açıklamadaki **50 W** çelişiyor; 80 W performans varsayılmamalı. LiHV modu listelenmiyor; desteklendiği kabul edilmemeli.

Hepsiburada için yalnız **12 V / 7 A / 84 W** açıklaması alındı. 12 V, B6'nın belirtilen giriş aralığında; 7 A adaptör kapasitesidir, pile 7 A zorunlu uygulanmaz. Fiş boyutu, polarite, regülasyon ve ürün kalitesi hâlâ açık. Bunlar doğrulanırsa güç kapasitesi bu 300 mAh piller için yeterli görünür; sadece başlıkla bağlantı onayı verilmez.

Orijinal B6 mevcut ve geri teslim edilecek; kalıcı seçim paket veya ayrı B6 + adaptör kombinasyonu. Aynı B6 gerçekten kullanılıyorsa paket olması şarj özelliklerini değiştirmez. Adaptör etiketi, uyumlu fiş/kutup, satıcının birlikte kullanım teyidi, garanti ve toplam maliyet belirleyici olmalı. Fiyatlar paylaşılmadığından daha ekonomik seçenek belirlenmedi. Kullanıcının önceki notuna göre klon adaylar değerlendiriliyor; bu bağımsız orijinallik testi değildir.

Erişilen [SkyRC iMAX B6 belgesi](https://www.skyrc.com/iMAX_B6_Charger), **SK-100002 klasik model** için 11–18 V DC giriş, 50 W şarj gücü, 0.1–5 A şarj akımı, 1–6S lityum desteği ve üretimden kalkmış olma bilgisini veriyor. Listelenen kimyalar Li-ion/LiPo/LiFe; bu sayfa LiHV desteğini doğrulamıyor. Bu özellikler klonlara veya farklı B6 sürümlerine otomatik aktarılmaz.

- 300 mAh pilde 1C = **0.30 A**. Pil üreticisi izin veriyorsa düşük güçlü kontrollü balans şarj, 80 W yarışından daha anlamlıdır.
- 0.30 A ile 8.4 V pakette yaklaşık 2.52 W, 12.6 V pakette 3.78 W çıkış gerekir. Bunlar gerilim × akım hesabı; gerçek giriş kayıpları ayrıca vardır.
- 12 V × 7 A = 84 W giriş adaptörü etiketi, şarj cihazından kayıpsız 80 W çıkış garantisi vermez. Küçük 300 mAh pil için bu maksimum güç zaten öncelik değildir.
- Öncelik: hücre voltajı doğruluğu, balans, storage modu, uygun kimya, küçük şarj akımı seçimi, güvenilir üretici ve kutup/fiş uyumu. Doğruluğu belirsiz klon, küçük pilin maliyetinden daha büyük zarar doğurabilir.
- İlk etapta ödünç orijinal cihazın tam modelini ve sağlıklı çalıştığını doğrulayıp kullanmak; kalıcı alımda güvenilir balans şarj cihazına bütçe ayırmak önerilir. “Orijinal” olması sınırsız güvence değildir; kablo, cihaz ve pil yine kontrol edilir.

Şarjda robot bağlı olmamalı. Uygun ana şarj kablosu ve balans bağlantısı cihaz kılavuzuna göre kullanılmalı. Pilin hücre sayısı/kimyası ve cihazın algıladığı değer uyuşmuyorsa devam etmeyin. Hasarlı, şişmiş veya aşırı ısınan pili kullanmayın; yanmaz bir alanda gözetimli şarj edin, okul/atölye sorumlusunun LiPo prosedürünü izleyin. Açık banana uçları ve balans pinleri kısa devreye karşı korunmalı.

## 6. Kronometre donanımı

Aktif düzen Uno + 5 V MZ80 + Waveshare 2.4 inç 8 pin LCD + buton. Ayrı lazer/alıcı, BPW34, LM393, KY-022 veya harici trimpot gerekmez. Eski lazer devresi yalnız arşivdir.

[Waveshare üretici belgesi](https://www.waveshare.com/wiki/2.4inch_LCD_Module), belirtilen modülde 5 V besleme ile 5 V lojik kullanımını destekliyor; çıplak veya farklı ILI9341 modüllerine genellenmez. [Robotistan MZ80 sayfası](https://www.robotistan.com/80cm-menzilli-kizilotesi-sensor-mz80) 5 V, yaklaşık 25–100 mA, yüzeye bağlı 3–80 cm aralık bildiriyor. Bu menzil hızlı robotta her yüzey için garanti değildir.

Sensör kablo renkleri önceki belgede kahverengi/mavi/siyah olarak kayıtlı. Bunların görevleri eldeki sensör etiketiyle doğrulanmalı; yalnız renk yeterli değildir. Etkin pin şeması için [mevcut kurulum belgesine](../hizli-cizgi-kronometre/baglanti-ve-kurulum.md) bakın. Gerçek 30 ms kesintisiz boşluk iddiasının yazılım sınırlaması [teknik incelemede](TEKNIK_INCELEME.md) açıklanmıştır.

## 7. Bütçe önceliği ve üreticiye sorular

Öncelik sırası: multimetre/ölçüm erişimi → onaylı 2S pil ve doğru kablolar → güvenilir balans şarj → doğrulanmış start/stop → lastik, etek, sensör şerit kablosu ve üretici önerili sarf/yedekler → kontrollü 3S deneyleri. Aynı anda birçok belirsiz adaptör ve yüksek C değerli pil almak yerine bir uyumlu temel set kurun.

Üreticiye gönderilecek sorular:

1. Teslim edilecek/edilen kart tam olarak Rev. 1.4.3 mü? Kılavuzdaki 93 g/1023 motor özellikleri bu set için geçerli mi?
2. Tam dolu standart 3S 12.6 V ve HV 3S 13.05 V destekleniyor mu? Kılavuzdaki 12.4 V sınırı neyi ifade ediyor? 2S HV 8.7 V ve türbin için üst sınır nedir?
3. Önerilen pilin kesin ölçüsü, ana/balans soketi, kimyası ve şarj profili nedir? Türkiye gönderisinde pil var mı? Güncel sayfada pil “Mexico only” olarak geçiyor.
4. 3S balans soketinden türbinin bağlanacağı doğrulanmış pin şeması nedir? Hücre dengesizliği için önerilen kullanım süresi nedir?
5. MEBSTART aktif-LOW tek sinyal için önerilen adaptör/firmware nedir? Panelize adaptör yalnız pin sırası mı değiştiriyor, lojik tersleme de yapıyor mu?
6. USBasp ve USB–TTL bağlantısında hedef besleme/pil/jumper düzeni nedir? Connector2/3 görevlerini etiketli fotoğrafla doğrular mısınız?
7. 90° köşeler ve kesikli çizgiler için desteklenen minimum geometriler/ayarlar nedir?
8. `offlineElapsedTime` başlangıç hatası için güncel düzeltme var mı? GO başlangıcında türbinin sıfırlanması amaçlı mı?
9. Değişiklik, takım içi kullanım ve değiştirilmiş kaynakların paylaşımı için lisans/izin kapsamı nedir? STL dosyaları sette varsa nasıl temin edilir?

## Kaynak ve erişim durumu

16 Eylül 2026'da yerel İngilizce kılavuz s.5–7, 12–17, 19, 22, 24–29; üretici firmware'i ve `urun-linkleri.txt` incelendi. Yukarıda bağlantılı Exotic Team MX, Robotus, SkyRC, Waveshare ve Robotistan sayfaları okunabildi. Droneshop'taki iki pil sayfası, n11'deki iki şarj ilanı ve Hepsiburada adaptör ilanı HTTP 403 verdi. Ürün URL'si bir teknik uygunluk belgesi değildir. Fiyat, stok ve Türkiye teslim maliyeti için kesin bütçe çıkarılmadı.
