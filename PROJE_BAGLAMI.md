# Team Turquoise — ATLAS proje bağlamı

Güncelleme: 16 Eylül 2026. Bu belge, kullanıcının anlattıklarıyla yerel dosya incelemesinin kalıcı özetidir. Yeni AI sohbetinde önce bu dosya, ardından ilgili ayrıntı belgesi okunabilir. Yeni sohbetlerin bu dosyaları kendiliğinden okuyacağı veya önceki sohbeti hatırlayacağı varsayılmamalıdır.

## Takım ve hedef — kullanıcının beyanı

- Ödemiş Ayhan KÖKMEN Fen Lisesi öğrencileri Team Turquoise adlı robotik kulübünü kurdu.
- Takım henüz küçük; bütçe sınırlı. İlk yarışma odağı hızlı çizgi izleyen robotlar.
- Hedef Robotex, Fibonacci, MEB Robot ve İztech gibi yarışmalara katılmak, şampiyonluklarla takımın adını duyurmak.
- Kullanıcı, Robochallenge başarıları ve dünya şampiyonluğu geçmişi olduğunu belirttiği ATLAS 1.4.3 platformuna yöneldi. **Bu başarıların kategori/yıl ve satılan donanım sürümüyle eşleşmesi bu incelemede bağımsız doğrulanmadı.**
- Üreticiden alınan kılavuzlar ve kaynak kodları klasörde bulunuyor. Test pistleri dekotaya arkalı önlü bastırıldı.
- Ürün bağlantıları tedarik adaylarını içeriyor; bağlantısı olan her ürün satın alınmış kabul edilmemeli.
- `urun-linkleri.txt` notuna göre kullanıcı Deneyap öğrencisi, atölyeden orijinal iMAX B6 ödünç aldı; cihaz kalıcı olarak takımın değil. Klon şarj cihazları ve adaptör satın alma aşamasında değerlendiriliyor.
- Kullanıcının devam mesajıyla netleşti: robot elinizde, şimdiye kadar 2S pil bağlandı ve pil orijinal B6 ile 8.4 V'a şarj edildi. Kullanılan pilin kesin modeli, şarj modu/akımı ve hücre gerilimleri henüz paylaşılmadı; bu pil Tattu adayıyla otomatik eşleştirilmemeli.
- Orijinal B6 geri teslim edilecek. Kalıcı satın alma seçenekleri aynı satıcıdan B6 + adaptör paketi veya B6 ile ayrı adaptör kombinasyonu.
- Basılı pist elinizde ve **200 × 280 cm**. Kullanıcı yanlış ölçülü PDF paylaştığını ve basılı pistte sorun olmadığını belirtti. PDF ölçü farkı bir baskı hatası değildir; yerel PDF geometrisi gerçek baskıya doğrudan aktarılmamalı.
- Kart revizyonunun fiziksel etiketi, diğer ürünlerin güncel envanteri, bütçe tutarı ve yarışma yılları henüz kesinleştirilmedi.

## Belgeler

- [Teknik inceleme: üretici kodu, MEBSTART testi, kronometre ve pistler](docs/TEKNIK_INCELEME.md)
- [Donanım, konnektörler, pil ve satın alma rehberi](docs/DONANIM_VE_KONNEKTORLER.md)
- [Test, geliştirme ve yarışmaya hazırlık planı](docs/TEST_VE_YARISMA_PLANI.md)
- [Açık bilgiler, görsel inceleme sınırları ve doğrulama listesi](docs/DOGRULAMA_LISTESI.md)
- [Konnektör kullanım kılavuzu (22 Eylül, yenibaşlayanlara detaylı; üç parça görsel doğrulandı)](docs/KONNEKTOR_KULLANIM_KILAVUZU.md)
- [İlk koşu günü planı — 23 Eylül, MEBSTART'lı akış ve sorun tablosu](docs/ILK_KOSU_PLANI_23EYL.md)

## Mevcut klasörün haritası

- `ATLAS_1.4.3.ino`, `Motors.ino`, `RunControl.ino`, `Sensors.ino`, `UI.ino`: üretici firmware'i, tek Arduino sketch'inin beş sekmesi. **Ellenmemiş orijinal.**
- `ATLAS_Takim/` (22 Eylül 2026): takımın yarış sürümü. Üretici tabanlı; MEBSTART durum makinesi (aktif-LOW GO, 1 sn ön-vakum, 30 ms STOP teyidi, SW1+SW2 emniyet freni), offlineElapsedTime düzeltmesi, kesik köprüsü (OFFLINE_GAP_BRIDGE_MS=60), offline failsafe (400 ms), kalibrasyon kapısı (CAL_MIN_CONTRAST=30), debugMode çıkarılmış (SRAM 291/2048 B). Derleniyor: flash 6518 B. **Kalibrasyon butonu: SW2 = siyah çizgi (bizim pist), SW1 = beyaz çizgi modu.**
- `ATLAS_Debug/` (23 Eylül 2026 güncellendi): hafif donanım doğrulama sketch'i (String yok, F()-tabanlı; SRAM 204 B). İKİ arayüz: seri menü (115200) VE bilgisayarsız buton+LED menü (SW2=seç [LED1=1,LED2=2,LED0=4], SW1=çalıştır, canlı test çıkışı=SW1+SW2 2 sn, turbinde SW2). Menü: [1] buton/LED/MEBSTART izleme (START→LED2), [2] 16 sensör ham değer (kontrast olursa LED2), [3] tekerlek testi (SW1 2 sn basili = onay), [4] türbin SW1 aç/kapa. `NASIL_YUKLENIR.txt` + `ATLAS_YARIS_PAKETI_23EYL.zip`: okul bilgisayarına ZIP ile tasma protokolu; tek .ino dosyasi gondermek 23 Eylul'de melez-firmware kazasina yol acti (yalniz ATLAS_Takim.ino + uretici sekmeleri + Gemini yamalari -> kendiliginden start, turbin suskun, viraj alamama).
- `ATLAS_Rev1.4_Manual_1.0_17012026(ENG).pdf`: 30 sayfalık İngilizce Rev. 1.4 kılavuzu, 17 Ocak 2026.
- `ATLAS_Rev1.4_Manual_1.0_17012026(ENG)_dual.pdf`: 30 sayfalık yan yana İngilizce/Türkçe belge. Çeviride teknik terim bozulmaları var; teknik değerlendirmede İngilizce asıl esas alındı.
- `ATLAS_MEBSTART_Test/ATLAS_MEBSTART_Test.ino`: ayrı Uno üzerinde iki LED ile aktif-LOW başlatma/ön-vakum/durdurma denemesi. ATLAS motor firmware'i değildir.
- `hizli-cizgi-kronometre/robot_kronometre/`: Uno + MZ80 + Waveshare LCD için bağımsız tur kronometresi.
- `hizli-cizgi-kronometre/tests/`: zamanlama çekirdeği ve GPIO/LCD model testleri; `stubs` yalnızca masaüstü test bağımlılıklarıdır.
- `hizli-cizgi-kronometre/baglanti-ve-kurulum.md`: etkin MZ80 kurulum belgesi.
- `hizli-cizgi-kronometre/baglanti-ve-kurulum-lazer-arsiv.md`: önceki lazer/fotodiyot/LM393 tasarımı; etkin kurulum değildir.
- `hizli-cizgi-kronometre/donanim-listesi.md`: önceki donanım ve kurulum notları. Parçaların güncel elde bulunma durumu tekrar teyit edilmeli.
- `urun-linkleri.txt`: robot, MEBSTART, iki pil, iki şarj cihazı ve adaptör adayları.
- `connector-photos/`: üç parça grubunun ön/arka altı fotoğrafı.
- `pist1.pdf`, `pist2.pdf`: birer sayfalık vektörel pistler. PDF dış ölçüleri yaklaşık 200 × 300 cm.
- Kök, MEBSTART test klasörü ve kronometre klasöründe ayrı `.git` dizinleri bulunuyor. Üçünde de inceleme anında commit yok; proje dosyaları izlenmiyor. Bu nedenle tarihlendirilmiş bir geliştirme geçmişi çıkarılamadı. Git iç verileri ve araç ayarları uygulama kaynağı olarak incelenmedi.

## Bugüne kadarki gelişim — kanıt düzeyleri

1. **Kullanıcı anlatımı:** takım kuruldu, hızlı çizgi izleyen odağı ve ATLAS seçimi oluştu; pistler bastırıldı.
2. **Dosyalardan görülen durum:** üretici firmware'i ve kılavuzları temin edildi; MEBSTART için ayrı LED testi hazırlandı.
3. **Mevcut belgelerde anlatılan gelişim:** kronometrede lazer + BPW34 + LM393 yaklaşımından MZ80'e geçildi; eski kurulum arşivlendi; LCD'de yalnız değişen karakterleri çizme yaklaşımı ve testler eklendi.
4. **Bu inceleme:** üç sketch hedef kartlar için derlendi; iki mevcut masaüstü testi geçti; kronometrede ek bir kısa-darbe senaryosu yeniden üretildi; teknik ve tedarik riskleri belgelendi.

İlk üç maddenin kesin takvim sırası veya fiziksel başarı sonuçları Git geçmişiyle doğrulanmış değildir. Klasörde tur süreleri, yarışma sonuçları veya osiloskop kayıtları bulunmadı.

## Önce çözülmesi gerekenler

1. `RunControl.ino:205` çizgi kaybı süresini başlatılmamış kendi değişkeninden çıkarıyor. Doğru referans `offlineStartTime` olmalı; bu incelemede kaynak değiştirilmedi.
2. MEBSTART aktif-LOW/tek sinyal, üretici firmware'i aktif-HIGH/READY + GO kullanıyor. LED testini doğrudan robota yüklemek veya modülü rastgele bağlamak doğru değil.
3. Türbin yalnızca üreticinin onayladığı 2S beslemeyi almalı; 3S paketin tamamı türbine verilmemeli. HV pil ve kılavuzdaki gerilim limitleri yazılı teyit gerektiriyor.
4. İlk testler düşük PWM ve üreticinin önerdiği 2S ile yapılmalı. Kılavuz 45 saniyeden uzun açık kalmamayı, 3S'te 8 saniyeyi aşmayan testleri öneriyor.
5. Gerçek pist ölçüsü kullanıcı tarafından 200 × 280 cm olarak netleştirildi; yanlış PDF paylaşımı nedeniyle önceki baskı hatası şüphesi kaldırıldı. Yerel PDF'nin keskin/kesikli geometri bulguları doğru baskı dosyasıyla eşleşmeden gerçek piste uygulanmamalı.
6. Kronometrede 30 ms kesintisiz boşluk garantisinin kısa darbelerde geçerli olmadığı görüldü; ölçüm doğruluğu ayrıca sahada doğrulanmalı.

## Bu oturumda yapılan ve yapılmayan işler

**Yapıldı:** kaynak/metin incelemesi, PDF metin ve vektör analizi, fotoğraf OCR denemeleri, erişilebilen üretici/satıcı sayfalarının okunması, hedef kart derlemeleri, masaüstü testleri ve bu dört Markdown belgesinin oluşturulması.

**Yapılmadı:** üretici/test/kronometre kaynaklarına değişiklik, fiziksel bağlantı, karta yükleme, LiPo şarjı, robot sürüşü, satın alma, commit/push veya yayımlama. Fotoğraflar doğrudan görsel olarak açılamadı; OCR, pin sırası ve parça teşhisinde kesin kanıt sayılmadı. Güncel yarışma şartnameleri incelenmedi; hiçbir yarışma için uygunluk onayı verilmedi.

Üretici kodunun başlığında CC BY-NC-ND 4.0 bildirimi var. Satış sayfasındaki “customizable code” ifadesiyle birlikte değerlendirilmeli; değiştirilmiş sürümü veya üretici dosyalarını kamuya açmadan önce üreticiden izin/lisans kapsamı açıklaması alınmalı. Bu bir hukuki uygunluk kararı değildir.
