# Çizgi izleyen robot tur kronometresi

## Amaç
Robot ölçüm çizgisinden ilk geçtiğinde süreyi başlatmak, ikinci geçtiğinde durdurmak ve tur süresini göstermek/kaydetmek.

## Mevcut donanım
- Arduino Uno klon (ürün bağlantısı henüz paylaşılmadı).
- [Lazer modülü: 5 V, 5 mW, 650 nm](https://www.robotistan.com/lazer-sensor-650nm-5v-5mw).
- [RFT BPW34 fotodiyot](https://www.robotistan.com/bpw34-fotodiyot).
- [MZ80 (E18-D80NK) kızılötesi sensör: belirtilen menzil 80 cm](https://www.robotistan.com/80cm-menzilli-kizilotesi-sensor-mz80).
- [LM393 DIP-8 karşılaştırıcı entegresi](https://www.robotistan.com/lm393-dip8).
- [10 kΩ çok turlu trimpot: 3296W-1-103](https://www.robotistan.com/10k-cok-turlu-trimpot-dik-3296w-1-103).
- [KY-022 kızılötesi alıcı modülü](https://www.robotistan.com/ky022-kizilotesi-alici-modulu).

## Kullanılan ekran
- [Waveshare 2.4 inç, 8 pinli LCD modülü](https://www.robotistan.com/24-inc-lcd-ekran-modulu).
- Satıcı sayfasından doğrulanan özellikler: ILI9341 sürücü, SPI arayüzü, 240 × 320 piksel, 65K renk, 3.3 V / 5 V çalışma gerilimi; Arduino desteği belirtiliyor.
- [Üretici belgesi](https://www.waveshare.com/wiki/2.4inch_LCD_Module), 5 V beslemede 5 V lojik ve doğrudan Uno bağlantısı belirtiyor. Bu bilgi belirtilen Waveshare modülü için geçerlidir; başka ILI9341 kartlarına genellenmemelidir.

## Seçilen düzen — MZ80 sürümü
- Pist dış ölçüsü 200 × 280 cm. MZ80 geçiş koridorunun yanına, robotun gövdesine bakacak biçimde sabitlenir; pistin tamamını taraması gerekmez.
- Algılama: **5 V MZ80 (E18-D80NK), yansımalı kızılötesi, engelde LOW dijital çıkış → Uno D2**. Kod `INPUT_PULLUP` kullanır. Karşıda lazer veya ayrı alıcı bulunmaz.
- Lazer, BPW34, LM393, harici 3296W trimpot ve KY-022 bu sürümde kullanılmıyor. Eski LM393 çıkışı D2'den ayrılmalıdır.
- Ekranda anlık, son ve oturumun en iyi süresi gösterilir. Kayıtlar RAM'de; güç kesilince silinir. Bilgisayar gerekmez.
- İlk engel algılaması başlatır; alan boşaldıktan sonraki ikinci algılama durdurur. Buton yeni ölçümü hazırlar; son ve en iyi süreyi silmez.
- Yeniden tetikleme kilidi 500 ms; en kısa gerçek turdan küçük kalmalıdır. Yeniden kurmak için en az 30 ms engel algılanmaması da gerekir.
- LCD'nin yalnızca değişen karakterleri güncellenir; büyük alanları sürekli silip yeniden çizme kaldırılmıştır.

## MZ80 teknik bilgisi ve kablo doğrulaması
- Robotistan sayfası 5 V besleme, yaklaşık 25–100 mA tüketim ve yüzeye bağlı 3–80 cm aralık belirtir. 80 cm her yüzey/ortam için garanti değildir; sensör analog mesafe değil engel var/yok bilgisi verir.
- [DFRobot SEN0019 belgesinde](https://wiki.dfrobot.com/Adjustable_Infrared_Sensor_Switch__SKU_SEN0019_) benzer sensör için kahverengi = 5 V, mavi = GND, siyah = sinyal; engel varken LOW, yokken HIGH açıklanır. **Robotistan sayfasının metni kablo görevlerini belirtmez; bu renkleri kendi sensör etiketi veya satıcı şemasıyla doğrulamadan uygulamayın.**
- Besleme/çıkış türü veya kablo renkleri farklıysa bağlantı öncesi doğrulama gerekir. Rastgele kutup denemeyin. Sensör GPIO'dan değil 5V hattından beslenir.
- Yüzey rengi, açı, mesafe ve ortam ışığı algılamayı etkiler. MZ80'e geçmek parazitsiz çalışma veya milisaniye doğruluğu garantisi değildir.

## Mevcut pasif parçalar ve eksikler
- 10 kΩ sabit dirençler mevcut. MZ80 temel bağlantısında dahili pull-up kullanılır; doğrulanmış açık kolektör çıkışta gerekirse D2–5V arasına bir adet 10 kΩ harici pull-up eklenebilir.
- 220 Ω dirençler bu devrede kullanılmıyor.
- 1 MΩ geri besleme direnci **MZ80 için gerekmiyor**. Eski lazer devresine ait bir öneriydi.
- Kondansatör sipariş edildi, henüz gelmedi. Besleme parazitini azaltmak için MZ80 VCC–GND arasına, sensöre yakın 100 nF seramik kondansatör önerilir; yerine kablo veya direnç bağlanmaz.
- 10 µF kondansatör isteğe bağlıdır: + uç 5V, − uç GND. 100 nF yerel bypass kondansatörünün yerine düşünülmemelidir.

## Diğer kurulum ihtiyaçları
- 1 adet normalde açık anlık buton: D4–GND, ek direnç gerekmiyor.
- Breadboard veya delikli pertinaks, jumper kablolar ve MZ80'i titreşmeyecek biçimde tutacak sabitleme parçaları.
- LCD ve sensörün toplam yüküne uygun regüle 5 V USB besleme/powerbank; düşük yükte otomatik kapanmayan model tercih edilmeli.
- Multimetre ilk işlev testinde zorunlu değil; kablo belirsizliği ve elektriksel arıza teşhisinde ölçüm veya satıcı doğrulaması gerekir.

Güncel bağlantı, multimetresiz ayar ve deneme sırası: **`baglanti-ve-kurulum.md`**. Güncel kod: **`robot_kronometre/robot_kronometre.ino`**. Eski lazer bağlantısı yalnızca arşiv olarak **`baglanti-ve-kurulum-lazer-arsiv.md`** dosyasındadır; iki devre aynı D2 çıkış hattını paylaşmamalıdır.

## Teknik notlar
- KY-022 modüle edilmiş kumanda sinyalleri içindir; MZ80 yerine doğrudan kullanılmaz.
- BPW34 doğrudan dijital sensör değildir; artık kullanılmayan LM393 eşik devresine ihtiyaç duyar.
- Uno'nun 5 V lojik çıkışları, yalnız 3.3 V lojik kabul eden LCD'ye doğrudan bağlanmamalıdır.
- Robot motorlarının beslemesi kronometreden ayrı olmalıdır. Yazılım kilidi bütün donanımsal parazitleri süzmez.
- Tek MZ80 yön veya robot kimliği ayırt etmez. Daha tekrarlanabilir ölçüm çizgisi için karşılıklı verici–alıcılı hazır ışık bariyeri değerlendirilebilir.
- Eski lazer artık bağlı olmamalıdır. Tekrar kullanılırsa göze doğrultmayın; ışın yolunu koruyun ve mat, yansıtıcı olmayan bir yüzeyde sonlandırın.
