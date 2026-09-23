# Team Turquoise — test ve yarışmaya hazırlık planı

Tarih: 16 Eylül 2026. Bu belge uygulanmış test sonuçları değil, mevcut analizden türetilen çalışma planıdır. Yapılmış derleme ve model testleri [teknik incelemede](TEKNIK_INCELEME.md), takım bağlamı [ana belgede](../PROJE_BAGLAMI.md) kayıtlı.

## 1. Strateji: önce tekrarlanabilir bitiriş, sonra hız

ATLAS güçlü bir öğrenme ve geliştirme platformu olabilir; hazır robot almak kendi başına şampiyonluk garantisi değildir. Yarışma başarısı şartname uyumu, start/stop, pist geometrisi, emiş/lastik bakımı, pil durumu, kontrol ayarı ve ekip prosedürünün toplamıdır.

Takım küçükken işleri üç sahiplikle bölmek yararlı olur: donanım/tedarik, firmware/test, pist/ölçüm/kayıt. Roller aynı kişide birleşebilir; her koşuda bir operatör ve durdurma/alan kontrolünden sorumlu bir gözlemci bulunmalı. Hedef, rastgele en hızlı tek tur yerine tekrarlanabilir başarı oranı ve anlaşılabilir hata kayıtlarıdır.

## 2. P0 — güç vermeden ve pistte koşmadan önce

- Pil etiketi, hücre sayısı, kimyası, ana/balans soketleri ve türbinin 2S uçları kesinleşsin. Multimetre erişimi edinin; bu projede gerekli temel ölçüm aracıdır.
- Üreticinin 12.4 V/3S/HV açıklaması ve USBasp besleme düzeni alınsın. Türbine 3S toplam gerilimi uygulanmasın. SB1'e rastgele köprü atılmasın.
- MEBSTART'ın kendi başına açılış/bekleme/START/STOP seviyeleri doğrulansın. Aktif-LOW ile mevcut aktif-HIGH GO uyumsuzluğu giderilmeden motorlu deneme yapılmasın.
- Üretici kodundaki offline süre hatası kontrollü çalışma sürümünde giderilip regresyon testine alınsın; orijinal arşiv korunsun.
- STOP, çizgi kaybı ve azami koşu süresi davranışları açıkça tanımlansın. Başlatma modülü çıkışı koparsa veya açılışta aktif kalırsa beklenen davranış test edilsin.
- Pist üzerindeki parça, kablo ve insan geçişi engellensin; robotun kaçabileceği yönde zarar vermeyecek boş alan oluşturulsun. Yüksek hızda robot elle yakalanmaya çalışılmasın.

Bu aşamanın kabul koşulu: **bilinmeyen güç pini veya belirsiz stop davranışı kalmaması**. Derlemenin geçmesi tek başına yeterli değil.

## 3. P1 — ayrı alt sistem testleri

### A. LED ve start/stop

Ayrı Uno LED düzeneğinde: açılışta pasif giriş, açılışta aktif giriş, normal START, ön-vakum sırasında STOP, ilk 100 ms'de STOP, koşuda STOP, stop sonrası tekrar kumanda, reset ve sinyal kablosu ayrılması senaryolarını deneyin. Her senaryoda beklenen/gerçekleşen durumu kaydedin.

Ön-vakumda iptal hiçbir zaman sürüş LED'ini yakmamalı; stop sonrası yeni START ile yeniden çalışmama davranışı doğrulanmalı. Seri çıktının zamanlamayı etkileyebildiği unutulmamalı. Bu düzenin onaylanması gerçek ATLAS pin eşleşmesini veya motor frenini onaylamaz.

### B. ATLAS sensör ve motorları

Üretici debug moduyla önce 16 sensörün ham/açık/koyu değerleri ve kalibrasyon aralığı kontrol edilsin. Tüm sensörlerin çizgiyi ve zemini görmesi sağlansın; tek sensör arızası ağırlıklı merkezi kaydırabilir.

Sürüş motoru testi üreticinin belirttiği gibi tekerlekler zeminden ve nesnelerden uzak, robot sabitlenmişken yapılmalı. Sağ/sol ve ileri/geri yönü düşük güçte doğrulanmalı. Türbin testi ise üretici talimatına uygun yatay, temiz, düz yüzeyde yapılmalı; dönen fan açıklığına erişim olmamalı. Tekerlek testi ile emiş testi aynı yerleştirme koşulu değildir.

### C. Kronometre

1. Alan boşken 30 saniye gözleyin; kendiliğinden başlamamalı. **Bu bekleme robotun pilini 30 saniye açık tutmayı gerektirmez.**
2. Hedefi sabit tutun; tek geçiş ikinci geçiş gibi sayılmamalı.
3. Alanı boşaltın, ikinci geçişi yapın; son/en iyi süreyi ve butonla yeni ölçümü doğrulayın.
4. Düzensiz/kısa gölgeler, butona basılı tutma, yeni ölçüm sırasında engel, motor paraziti ve değişen ışıkla tekrar deneyin.
5. Aynı gövdeyi aynı yönde ve aynı yanal konumda geçirerek tetikleme noktasının tekrarlanabilirliğini değerlendirin.
6. En yüksek planlanan hızda kaçan veya fazladan geçiş olup olmadığını inceleyin. Ekranın 100 ms yenilemesi kısa darbeleri görsel olarak saklayabilir.

Kronometredeki 30 ms boşluk açığını giderip modele yeni kısa-darbe testi eklemek ayrı yazılım adımıdır. Gerçek sensörün tepki süresi ve yüzey bağımlılığı yazılımla ortadan kalkmaz.

## 4. P2 — pist ve emiş uygunluğu

Kullanıcı düzeltmesine göre gerçek basılı pist **200 × 280 cm**, elinizde ve sorun bildirilmedi. Yerel yaklaşık 200 × 300 cm PDF yanlış ölçülü dosya olarak paylaşılmış. Bu nedenle baskıda sıkıştırma/kırpma hatası olduğu varsayımı kaldırıldı. Parkura özgü geometri analizi için doğru baskı dosyası veya gerçek pist görseli kullanılmalı; aşağıdaki genel yüzey kontrolleri rutin test prosedürüdür, saptanmış bir baskı hatası değildir.

Her yüz için:

- Ekleme, bombe, çizik, pürüz, baskı katmanı ve temizlik durumunu kontrol edin.
- Sensör kalibrasyonunu aynı ışık/zemin üzerinde tekrar yapın.
- Etek yüzeyini bozacak mekanik müdahaleden kaçının; emişin çevre boyunca tutarlı olduğunu doğrulayın.
- Kronometreyi tek bir geçiş koridoruna bakacak şekilde sabitleyin. Yanındaki paralel çizgiden geçen robotu da algılamamalı.
- Yerel `pist1` dosyasındaki keskin zigzag/kesinti bulgularını, doğru baskı dosyasıyla eşleşmeden gerçek pistin özelliği saymayın. Gerçek parkurda bu bölümler varsa ayrıca test edin. Başlangıç hız testi için geniş, kesintisiz ve üretici geometrisine uygun bir bölüm tercih edin.

Kılavuz s.29 geniş dönüş çapını >1.5 m olarak belirtiyor. Mevcut pistleri bastırmış olmak boşa yatırım anlamına gelmez; bunlar kontrollü düşük hız ve zorlayıcı geometri testinde kullanılabilir. Ancak emiş veya sensör barına hasar veren fiziksel koşullar görülürse durun, “yazılımla aşılır” diye sürdürmeyin.

## 5. P3 — kontrollü hız ve PD ayarı

Önerilen ilk hedef: üreticinin önerdiği 2S ile, menünün düşük ucundan başlayarak, aynı koşulda hatasız bitiriş. PWM 60 bu yazılımda mevcut alt uçtur; belirli pist için güvenli hızı garanti etmez. Gerekirse daha düşük kontrollü test ayarı üreticiyle değerlendirilir.

Kılavuz s.24 hız seçiminde en fazla 160, türbinde en fazla 215 öneriyor; varsayılan türbin 200. Bunlar deneme hedefi değil üst önerilerdir. İlk oturumda sınırları zorlamayın.

İzlenecek sıra:

1. Donanım ve kalibrasyon sabit, KP/KD varsayılan; düşük hızda yön ve temel takip doğrulansın.
2. Bir oturumda tek değişken değiştirin: önce hız veya KP veya KD. Pil/zemin/lastik değişmişse kıyaslamayı ayrı etiketleyin.
3. Aşırı salınım, virajı kesme, çizgi kaybı, düzde sağ/sol sapma gibi belirtileri video ve konumla kaydedin; hepsini sadece KP'ye bağlamayın.
4. Önce 5, sonra 10 geçerli denemede bitiriş oranını izleyin. Örneğin 10/10 bitiriş takım içi bir geçiş hedefi olabilir; yarışma standardı değildir.
5. En iyi turla birlikte medyanı, en yavaş geçerli turu ve başarısız koşu sayısını raporlayın. Başarısız koşuları veriden sessizce çıkarmayın.
6. 2S'te tekrarlanabilir temel oluşmadan 3S'e geçmeyin. 3S için voltaj ve süre onayı ayrıca gerekir.

Türev terimi örnek başına fark olduğu için kontrol periyodu değişirse yeniden ayar gerekir. Gelecekte virajda hız azaltma, çizgi güven puanı ve türev filtreleme değerlendirilebilir; önce mevcut süre hatası, stop önceliği ve ölçüm kalitesi çözülmeli. Otomatik öğrenme/PID eklemek ilk adım değildir.

## 6. Süre ve ısınma disiplini

İngilizce kılavuz s.27, 78M05 ısınması nedeniyle **koşu dahil 45 saniyeden uzun açık kalmamayı**; 3S denemelerinde **8 saniyeyi aşmamayı** öneriyor. Bu sürelerin altında kalmak her ortamda ısınmayacağı garantisi değildir; anormal ısınmada hemen durdurup beslemeyi güvenle kesin.

Hazırlık, kalibrasyon, start bekleme ve stop sonrası LED animasyonu da açık kalma süresine dahildir. `disableRobot()` regülatörü kapatmıyor. Robotu açık bırakıp bilgisayarda analiz yapmak yerine enerjiyi ayırın. Kesin bir soğuma süresi uydurmayın; motor/regülatör durumu ve üretici yönlendirmesine göre yeniden deneyin. Kronometre bağımsız beslemede açık kalabilir.

Bu sınırlar nedeniyle 3S uzun kesintisiz dayanıklılık turları planlamayın. Yarışmanın bekleme/koşu süresi bu kısıtlarla çelişiyorsa donanım üreticisinden çözüm alınmadan “yarışmaya hazır” kabul etmeyin.

## 7. Deney kayıt şablonu

Her koşu için şu alanları doldurun; burada gerçek ölçüm olmadığından örnek sonuç uydurulmadı:

- Tarih/saat, operatör, koşu kimliği.
- Firmware sürümü veya commit/hash; son değişiklik.
- Pil modeli, 2S/3S ve kimya; koşu öncesi/sonrası hücre voltajları.
- Pist dosyası/yüzü, gerçek ölçü ve yüzey/ışık/temizlik bilgisi.
- PWM, KP, KD, türbin PWM, rampalar, kontrol periyodu.
- Açık kalma/koşu süreleri, gözlenen sıcaklık veya ölçüm.
- Kronometre sensör konumu ve ayarı; birinci/ikinci geçiş tanımı.
- Tur süresi; bitirdi/çizgiden çıktı/yanlış tetikleme/iptal.
- Hata konumu ve video dosyası; gerekiyorsa sensör logu.
- Karar: aynı ayarı tekrar et, geri al veya tek değişkeni değiştir.

Kronometrede son/en iyi değer RAM'de olduğu için oturum sonunda sonuçları ayrıca yazın; güç kesilince silinir. Otomatik CSV/kalıcı kayıt henüz uygulanmış değil. USB seri kayıt eklenecekse ATLAS'ın 750 µs kontrol döngüsünü bloklamayacak şekilde tasarlanmalı.

## 8. Yarışma uygunluğu kontrolü

Robotex, Fibonacci, MEB Robot ve İztech hedefleri kullanıcıdan alındı. **Bu incelemede belirli yıl/kategori/ülke etkinliğinin güncel şartnamesi incelenmedi.** Robotun her yarışmaya uygunluğu, dünya şampiyonluğu iddiası veya hazır kit kabulü onaylanmadı. Satıcının “MEB uyumlu modül” ifadesi güncel şartname yerine geçmez.

Her hedef için organizatörün o yıl yayımladığı şartnameyi arşivleyip şu soruları cevaplayın:

- Yaş/öğrenim, takım sayısı, danışman, kayıt ücreti ve son tarih.
- Hazır ticari robot, dışarıdan alınmış kod ve özgünlük şartları.
- En/boy/yükseklik/ağırlık sınırları; sensör barı ve çıkıntıların ölçüme dahil olması.
- Vakum/türbin kullanımı, zemine temas ve iz bırakma kuralları.
- Çizgi eni/rengi, kesinti, kesişim, rampa, minimum dönüş ve parkur yüzeyi.
- Başlatma modülü standardı, sinyal mantığı, hakem kontrolü ve uzaktan durdurma.
- Süre hangi anda başlar/biter; ön-vakum/kalkış süresi dahil mi?
- Pil, teknik kontrol, saha beklemesi ve koşu süreleri.
- Tur/hak sayısı, pist dışına çıkma cezası, onarım ve ayar izinleri.

**Öneri:** önce bir öncelikli yarışma ve kategori seçin; donanımı dört farklı varsayımsal parkurun ortak en zor koşuluna göre geliştirmeye çalışmak bütçeyi dağıtabilir. Takım tanıtımında henüz elde edilmemiş sonuçları başarı gibi yazmayın; öğrenme, ölçüm ve kendi geliştirmelerinizi öne çıkarın.

## 9. Kaynak yönetimi ve sonraki adımlar

Üç iç içe Git deposu var ve henüz commit yok. Hemen topluca `git add`/push yapmak yerine proje sahipliği, üretici lisansı ve hangi klasörün ana depo olacağı netleşsin. İç depoları silmeden veya dönüştürmeden önce sahipleri ve geçmiş durumu korunmalı. Bu inceleme Git yapısını değiştirmedi.

Üretici orijinali ile takım çalışma sürümünü ayırmak; çalışan sürümlere açık isim vermek; her değişiklikte regresyon testi ve kart derlemesi yapmak önerilir. Fiziksel robotta doğrulanmamış bir derleme, yarışma sürümü olarak etiketlenmemeli.

## 10. Bilgi eksiklerinin takibi

Açık maddeler, gereken kanıt ve karar sınırları [DOĞRULAMA LİSTESİ](DOGRULAMA_LISTESI.md) belgesinde tutulur. Yeni bilgi geldikçe kaynağı ve tarihiyle kaydedilmeli; önceki tahminler doğrulanmış bilgi gibi aktarılmamalı. Bir fotoğrafı dönüştürmek, onu görsel olarak incelemek veya elektriksel bağlantıyı ölçmek aynı işlem değildir.
