# ATLAS — bilgi eksikleri ve doğrulama listesi

Tarih: 16 Eylül 2026. Amaç: inceleme kapsamındaki belirsizlikleri görünür tutmak; eksik bilgi yerine tahminle bağlantı veya satın alma kararı vermemek. Bu liste bütün olası kusurların bulunacağı garantisi değildir.

## Durum ve kanıt kuralları

- **Doğrulandı:** kaynağı ve kapsamı açık. Derleme/model testi, fiziksel test yerine geçmez.
- **Beyan:** kullanıcı veya satıcının söylediği; bağımsız ölçüm olduğu varsayılmaz.
- **Aday:** OCR veya dolaylı ipucu; kesin parça/pin teşhisi değildir.
- **Açık:** gereken kanıt henüz yok.
- Her kapanan madde için gelen dosya/sayfa/ürün modeli, tarih, bulgu ve varsa çelişki kaydedilir.
- Pin dizilimi, besleme kutbu, pil kimyası veya gerilim üst sınırı açık kaldığında ilgili bağlantıya onay verilmez.

## Görsel incelemenin mevcut sınırı

Yerel dosya okuma aracı JPEG'i binary dosya olarak reddetti. Kullanılabilir araçlarda doğrudan görüntü açma yeteneği yok; PNG dönüşümü bu yeteneği eklemez. Altı konnektör fotoğrafına OCR uygulandı; görsel teşhis tamamlanmadı.

İngilizce kılavuzun gömülü metni ve pistlerin vektör geometrisi okundu. Kılavuzdaki fotoğraflar, bağlantı çizgileri ve akış şemalarının görsel yorumu tamamlanmadı. OCR ile çizgilerin hangi pine gittiği kesinleştirilmez.

PNG paylaşım kopyaları için `pdf-sayfalari/` ayrıldı: `kilavuz-01.png`–`kilavuz-30.png`, `pist1-01.png`, `pist2-01.png`. Asıl ölçüm kaynağı PDF'dir; 2400 piksel uzun kenarlı PNG önizlemesinden hassas fiziksel ölçü çıkarılmaz. İki dilli belge teknik asıl yerine kullanılmadığından ayrıca çoğaltılmadı.

Sohbet arayüzü görsel eki destekliyorsa kullanıcı görüntüleri doğrudan sohbete ekleyebilir. Ekler yalnız yol/metin olarak iletilirse görsel inceleme yapılmış sayılmaz; etiket metni, şema veya başka doğrulanabilir kaynak gerekir.

## A. Donanım ve konnektörler — öncelikli açık maddeler

- [x] **A1a — Teslim/kullanım beyanı alındı:** robot elinizde, 2S pil bağlandı; orijinal B6 ile 8.4 V'a şarj edildi. Bu bilgi kullanıcının devam mesajından; bağımsız elektriksel ölçüm değil.
- [ ] **A1b — Kalan bilgiler:** kullanılan 2S pilin kesin modeli ve etiketi, kart revizyonu, diğer parçaların mevcut/siparişte/adayı ayrımı. Kullanılan pilin Tattu 106696 olduğu varsayılmayacak.
- [ ] **A2 — connector1:** ön/arka, USBasp yazısı, jumper etiketleri, iki kablo ucunun yönleri ve pin-1 işaretleri. OCR USBasp adayını destekliyor; hedef besleme jumper'ı ve ICSP pin eşleşmesi açık.
- [ ] **A3 — connector2:** ön/arka tüm pin yazıları ve varsa üzerindeki entegre kodu. Pasif UART ara kartı mı, aktif USB–seri devresi mi; start adaptörüyle ilişkisi açık.
- [ ] **A4 — connector3:** ön/arka, entegre kodu, jumper ve header etiketleri. 3.3/5 V seçiminin beslemeyi mi lojik seviyeyi mi değiştirdiği şema/datasheet ile doğrulanacak.
- [ ] **A5 — Robot karşılıkları:** ICSP, UART, start portu, türbin bağlantısı ve SB1'in okunaklı yakın çekimi/üretici pin şeması. Güç kapalıyken çekilmeli; fotoğraf için şüpheli bağlantı yapılmamalı.
- [ ] **A6 — MEBSTART:** tam model, mevcut bağlantı, açılış/START/STOP davranışı. Satıcı aktif-LOW bilgisi mevcut; eldeki modül ve ATLAS'a entegrasyon açık.
- [ ] **A7 — Kronometre:** gerçek MZ80 etiketi, LCD kart modeli, kurulu devrenin genel görünümü ve gözlenen davranış. Eski belgede yazan parça listesi güncel envanter sayılmayacak.

İlk görsel tur için connector2 ve connector3 ön/arka ile kılavuz s.6, 19, 22, 25 önceliklidir. Sonraki turda s.15, 17, 18, 23 ve kalan sayfalardaki görsel ayrıntılar kontrol edilir. Bu sıra kalan sayfaların incelenmiş olduğu anlamına gelmez.

## B. Erişilemeyen ürün sayfaları — kullanıcı metniyle kısmen tamamlandı

Doğrudan erişimler HTTP 403 verdi. Kullanıcı devam mesajında iki pilin teknik alanlarını, B6 açıklaması/kutu içeriğini ve adaptör başlığını paylaştı. Aşağıdaki eski talep kapsamının bir bölümü artık karşılandı; yalnız kalan bilgiler tekrar istenecek. Fiyat bütçe için yararlı; adres, sipariş numarası ve ödeme bilgisi gerekmez.

- [x] **B1/B2 temel özellikler alındı:** DOGCOM 107764, 3S 11.4 V HV LiPo, 300 mAh, 100C, XT30; uzunluk/genişlik/yükseklik 16/54/18 mm. Tattu 106696, 2S 7.6 V LiPo, 300 mAh, 75C, PH2.0; aynı sırayla 47/17/13 mm. Satıcı beyanıdır. Stok ve 1 iş günü ifadesi Türkiye teslim garantisi değildir.
- [x] **B3 şarj özellikleri ve tekli kutu içeriği alındı:** 11–18 V giriş, 50 W şarj, 5 W deşarj, 0.1–5 A şarj, 1–6S lityum, balans/storage; Li-ion/LiPoly/LiFe listeleniyor, LiHV belirtilmiyor. Tekli kutuda yalnız B6. URL'deki 80 W ile açıklamadaki 50 W çelişkisi açık.
- [x] **B4 paket farkı beyanı alındı:** aynı satıcı diğer seçeneğe adaptör eklemiş; adaptör modeli/etiketi yok.
- [x] **B5 başlık bilgisi alındı:** 12 V, 7 A, 84 W. Fiş, polarite ve regülasyon bilgisi yok.
- [x] **B6 kullanım durumu alındı:** orijinal B6 mevcut, 2S pili 8.4 V'a şarj etmekte kullanıldı, geri teslim edilecek. Model kodu/şarj modu ve akımı açık.

**Açık kalan ayrıntılar (tamamı isteniyor anlamına gelmez):**

- [ ] **B1 — [Tattu 106696](https://droneshop.nl/tattu-300mah-7-6v-75c-2s-lipo-accu-ph20):** kalanlar hücre başına maksimum şarj gerilimi, HV tanımı, önerilen şarj akımı, ağırlık ve balans soketi. PH2.0 ve boyut artık satıcı metninden biliniyor.
- [ ] **B2 — [DOGCOM 107764](https://droneshop.nl/dogcom-300mah-100c-3s-114v-hv-lipo-xt30):** kalanlar maksimum şarj gerilimi, önerilen şarj akımı, ağırlık ve balans soketi. HV/XT30 ve boyut artık satıcı metninden biliniyor.
- [ ] **B3 — [Tekli B6](https://www.n11.com/urun/candagrup-imax-b6-80w-adaptorlu-rc-lityum-pil-sarj-cihazi-18650-pil-sarj-18744849?magaza=oyuncakhobi):** kesin cihaz modeli/etiketi, 50/80 W çelişkisi ve şarj kabloları. LiHV destek iddiası yok; böyle bir iddia gelirse ayrıca kılavuzla doğrulanmalı.
- [ ] **B4 — [Adaptörlü B6](https://www.n11.com/urun/hubstein-imax-b6-dijital-sarj-cihazi-ac-dc-sarj-adaptoru-eu-42139510?magaza=oyuncakhobi):** paket adaptörünün etiketi, fiş/polarite uyumu ve aynı B6 modelinin verildiğinin teyidi.
- [ ] **B5 — [Ayrı adaptör](https://www.hepsiburada.com/imax-b6-uyumlu-12v-7a-adaptor-12-volt-7-amper-84w-adaptor-trafo-pm-HBC000075Q57Q):** kalanlar fiş iç/dış çapı, merkez polaritesi, regüle DC çıkış/etiket ve seçilen B6'yla uyum. 12 V/7 A/84 W tekrar istenmeyecek.
- [ ] **B6 — Ödünç cihaz:** kesin model adı/kodu, kullanılan şarj modu/akımı ve hücre gerilimleri. Kullanım ve geri teslim durumu artık biliniyor.

Metin olarak kopyalama tercih edilir. Sayfa PDF olarak kaydedilecekse teknik özellik ve kutu içeriği sekmeleri önce açılmalı; dinamik sekmeler kayıtta eksik kalabilir. Bilgi hiç yazmıyorsa satıcıya sorulacak, varsayılmayacak.

## C. Üretici teyidi gerekenler

- [ ] **C1 — Gerilim:** kılavuzdaki 12.4 V üst sınır ile 3S desteğinin açıklaması; standart 3S 12.6 V, 3S HV 13.05 V ve 2S HV 8.7 V için robot/türbin ayrı onayları.
- [ ] **C2 — Türbin:** 3S balans soketinde kullanılacak kesin pinler, ortak GND düzeni ve hücre dengesizliğinin yönetimi. SB1'in mevcut durumu.
- [ ] **C3 — Start adaptörü:** yalnız pin sırası mı değiştiriyor, sinyal tersleme de var mı? MEBSTART için önerilen donanım ve firmware.
- [ ] **C4 — Programlayıcı:** USBasp hedef beslemesi, pilin ayrılması, UART lojik seviyesi ve jumper düzeni.
- [ ] **C5 — Firmware:** offline süre hatasına güncel düzeltme ve GO anında türbin sıfırlamasının amacı.
- [ ] **C6 — Mekanik:** keskin/kesikli parkur desteği, minimum dönüş geometrisi; güncel kılavuz ve varsa STL dosyaları.
- [ ] **C7 — Lisans:** üretici kodunu değiştirme ve değiştirilmiş sürümü paylaşma kapsamı. “Customizable” satış ifadesiyle ND bildiriminin açıklaması.

## D. Pist, yarışma ve fiziksel doğrulama

- [x] **D1a — Baskı ölçüsü/beyan:** gerçek pist 200 × 280 cm, elinizde; kullanıcı yanlış ölçülü PDF gönderdiğini ve pistte sorun olmadığını belirtti. Ölçü farkından kaynaklı baskı hatası şüphesi kapatıldı.
- [ ] **D1b — Yüzey envanteri:** kalınlık, tek/parçalı oluşu, kaplama türü ve iki yüzün genel görseli; rutin donanım kaydı, saptanmış arıza değil.
- [ ] **D2 — Geometri kaynağı:** gerçek baskıya ait doğru PDF/görsel. Yerel yanlış ölçülü PDF'nin çizgi eni, kesinti veya dönüş bulguları gerçek piste doğrudan aktarılmayacak.
- [ ] **D3 — Yarışmalar:** öncelikli etkinlik, yıl ve tam kategori; organizatörün güncel şartname bağlantısı/PDF'si. Hazır kit, vakum, boyut, start modülü ve süre tanımı tek tek incelenecek.
- [ ] **D4 — Bütçe/envanter:** harcanabilir tutar, elde olan/ödünç/siparişte/alınacak ayrımı. Satın alma listesi buna göre sadeleştirilecek.
- [ ] **D5 — Deney kayıtları:** gerçek tur süreleri, kaçış/stop denemeleri, sensör kalibrasyon değerleri ve varsa videolar. Model testinden fiziksel performans sonucu çıkarılmayacak.

## Tamamlanmış incelemeler — kapsamı sınırlı doğrulamalar

- [x] Beş üretici `.ino` sekmesi, MEBSTART LED sketch'i ve kronometre kaynak/test dosyaları incelendi.
- [x] Üç sketch hedef kartlar için derlendi; iki mevcut masaüstü testi geçti. Ölçüler ve uyarılar teknik incelemede.
- [x] Kronometrede yeniden kurulma boşluğu için ek kısa-darbe modeli çalıştırıldı; 30 ms garantisinin sağlanmadığı senaryo kaydedildi.
- [x] İngilizce PDF metni ve pist vektörleri çıkarıldı; PDF sayfa ölçüleri doğrulandı.
- [x] İngilizce kılavuzun 30 sayfası ve iki pist, `pdf-sayfalari/` altında 32 PNG olarak oluşturuldu; dosyaların PNG bütünlüğü kontrol edildi. Toplam yaklaşık 8.74 MB. Bu işlem görsel inceleme değildir.
- [x] Exotic Team MX, Robotus, SkyRC, Waveshare ve Robotistan'ın ilgili sayfa metinlerine erişildi. Gömülü tüm resimlerinin incelendiği iddia edilmiyor.

Açık maddeler yeni bilgi gelmeden işaretlenmeyecek. Donanım üzerinde işlem veya kod düzeltmesi bu listenin hazırlanmasının parçası değildir.
