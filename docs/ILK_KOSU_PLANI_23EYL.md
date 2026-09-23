# Team Turquoise — İlk koşu günü planı (23 Eylül 2026)

Hedef: 200 × 280 cm Fibonacci tarzı pistte ATLAS'ın ilk kontrollü turu. Kullanılacak yazılım: önce **`ATLAS_Debug`** (donanım doğrulama), sonra **`ATLAS_Takim`** (yarış sürümü, MEBSTART'lı).

Bu plan sabah sırasıyla uygulanır; bir adım geçilemeden sonrakine geçilmez. Yanında yazıcıdan çıkar.

---

## 0. Çantada olacaklar

- Robot (MEBSTART takılı olacak — aşağıda bağlantı), 2S pil (şarjlı, sabah hücre başına ~4.2 V), yedek pil varsa
- USBasp + 10 pin şerit + 10→6 adaptör (connector1), USB-C CH340 (connector2), panel adaptör (connector3)
- Laptop (Arduino IDE + Zadig), kısa USB kabloları
- Multimetre, küçük yalıtım bandı (kesikleri kapatmak İÇİN YEDEK PLAN), koli bandı
- Defter/telefon: kayıt şablonu §8

## 0.5 Kodu okul bilgisayarına taşıma — TEK KURAL

**Bir Arduino sketch'i tek dosya değildir; klasördeki tüm sekmelerdir.** 23 Eylül'deki sorunun kökü buydu: yalnız `ATLAS_Takim.ino` gönderildiği için robota melez kod yüklendi.

- Arkadaşına **tek bir dosya gönderme.** `ATLAS_YARIS_PAKETI_23EYL.zip`'i (klasörde hazır) olduğu gibi gönder; içinde `ATLAS_Takim/` (5 sekme) + `ATLAS_Debug/` + `NASIL_YUKLENIR.txt` var.
- IDE'de `ATLAS_Takim.ino` açıldığında sekme çubuğunda **5 sekme** görünmeli: `ATLAS_Takim | Motors | RunControl | Sensors | UI`. Görünmüyorsa yükleme; zip'i yeniden aç.
- Üreticinin orijinal kök `.ino` dosyalarını bu klasörlere karıştırma. Derleme hatası çıkarsa AI'a yamalatma; hatayı olduğu gibi takıma ilet.
- Beklenen derleme ölçüleri: ATLAS_Takim ≈ flash 6518 / SRAM 291; ATLAS_Debug ≈ flash 6034 / SRAM 204.

## 1. MEBSTART montajı (ilk iş)

Konum: robot üst yüzeyi; IR alıcı gözü dışa dönük ve gölgelenmemiş olsun (hakem kumandası her açıdan görmeli). Gövde 3 cm yükseklikte IR'ı kesmeli (kronometre + yarışma kuralı).

Bağlantı (robot start portu `+  −  GO  RDY`):
- MEBSTART **Sinyal → GO**
- MEBSTART **− → GND**
- MEBSTART **+ → 5V**
- **RDY boşta kalır** (tek sinyallik modül; ön-vakum yazılımda).

Kontrol: bağladıktan sonra, güç vermeden, multimetreyle Sinyal–GND ve +5V–GND arasında kısa devre olmadığını doğrula.

## 2. Aşama A — ATLAS_Debug ile donanım doğrulaması (~20 dk)

1. **Yükle:** USBasp tak (pil takılı, hedef-besleme jumper'ı açık). IDE: kart "Arduino Nano", işlemci "ATmega328P", programlayıcı "USBasp". `ATLAS_Debug.ino` aç → **Taslak → Programlayıcı ile Yükle** (Ctrl+Shift+U). "Done uploading."
2. **Seri bağla:** USBasp'yi çıkar. Panel adaptör üzerinden CH340 → UART; **önce robota, sonra PC'ye.** Terminal 115200 baud.
3. **Robota güç ver** (pil tak, tekerlekler havada tut). Ekranda menü görülmeli.
   *USB-TTL yoksa bile gider:* yeni `ATLAS_Debug` buton menülü — SW2 kısa basım = test seç (LED1=1, LED2=2, LED0=4 ikilik toplamı), SW1 kısa basım = çalıştır. [1] testinde MEBSTART START → LED2 yanar (bilgisayarsız da görülür). [3] motor testi için SW1'i 2 sn basili tut = onay. Canlı testlerden çıkış: SW1+SW2 2 sn (türbinde SW2).
4. **[1] MEBSTART testi:** beklemede `GO(D4)=1 (bekleme)` görmelisin. Kumandadan START bas → `GO(D4)=0 (START)` ve LED2 yanar. STOP bas → tekrar bekleme. **Bu adım geçmeden piste çıkma yok.**
5. **[2] Sensör testi:** eli/parmagı sensör barının altında soldan sağa gezdir; 16 sütunun hepsi değişmeli. Değişmeyen sütun varsa FFC şeridi/oturuş kontrol.
6. **[3] Motor testi:** tekerlekler havada, `y` onayı ver. Beklenen: LED0 yanarken SOL teker önce ileri sonra geri; LED1'de SAĞ aynı. Yön tersse kaydet — yazılımda sign bayrağı ile düzeltilir.
7. **[4] Türbin testi:** robot temiz düz zeminde, `t` ile aç (PWM 200), emişi elle hisset, `t` ile kapa, `x` ile çık.
8. Sorun yoksa pili çıkar, seri bağlantıyı sökmek gerekmiyor ama UART yalnız test için.

## 3. Aşama B — ATLAS_Takim yarış sürümü (~5 dk)

1. USBasp ile `ATLAS_Takim.ino`'yu aynı yolla yükle.
2. Bu sürümde **debug modu yok** (RAM 291/2048 B'ye indi; üreticinin debugMode'u çıkarıldı). Açılışta buton basılı tutmak artık debug'a girmez — yanlışlıkla basılı tutmayın, doğrudan kalibrasyona geçer.

## 4. Aşama C — Pistte ilk koşu (ana akış)

Yeni akış (MEBSTART durum makinesi):

```
Güç aç → açılış animasyonu → KALİBRASYON (**SW2** seç = siyah çizgi) → kapı kontrolü
→ ARMED (LED0 yanıp söner; SW1=hız azalt, SW2=hız artır)
→ Kumanda START → 1 sn ön-vakum (LED2 hızlı çakar)
→ KOŞU (tüm LED'ler yanık)
→ STOP: kumandadan STOP veya SW1+SW2 → 500 ms fren → KİLİT (reset gerekir)
```

Adım adım:
1. Robotu çizgiyi kesecek biçimde pist başlangıcına koy, tekerlekleri bekletirken **pili tak.** Açılış animasyonu (LED2) bitsin.
2. **SW2'ye bas** — bizim pist **beyaz zemin + siyah çizgi**, yani "siyah çizgi modu" gerekir. Kılavuz ve senin 22 Eylül gözlemin aynı şeyi gösteriyor: **SW1 = beyaz çizgi modu, SW2 = siyah çizgi modu.** (Önceki taslakta yanlışlıkla SW1 yazıyordu, düzeltildi.) SW2'ye basınca kalibrasyon **başlar**: LED'ler yanıp sönerken robotu elinde çizgi üstünde 3–4 kez soldan sağa yavaşça salla (16 sensör de hem beyazı hem siyahı görsün) → herhangi bir butonla **kalibrasyonu bitir.**
3. **Kapı kontrolü:** kontrast düşükse LED0/LED1 hızlı çakıp kalır = kalibrasyon başarısız → resetle, 2. adımı tekrarla (daha geniş salla). Ondan emin olunca: ARMED, LED0 yavaş çakar.
4. **Hız ayarı:** ilk koşu SW1 ile **PWM 60'a** düşür varsayılan 100 bile hızlı gelebilir; tekerleği havada tutup test ettiğin PWM 60 güvenli taban).
5. **START:** operatör çekilir → kumandayla START. Türbin 1 sn şişer (ön-vakum); tekerlekler sonra hareket eder. İlk 2 metrede robotu takip et ama **dokunma.**
6. **STOP:** tur sonunda veya kaçışta: kumandadan STOP; yedek: SW1+SW2 çift basım. 500 ms fren sonrası LED'ler 1 sn arayla çakıyorsa robot kilitte — **reset ile yeni koşu.**
7. Her koşudan sonra pil sökmeye gerek yok ama **toplam açık kalma 45 sn'yi geçmesin** (regülatör ısınması); koşular arası 1–2 dk mola.

## 5. Kesikler (dashed) için A planı / B planı

- **A planı (yazılım):** `OFFLINE_GAP_BRIDGE_MS = 60` aktif. Robot çizgiyi kaybedince ilk 60 ms aynı hızda düz devam eder, sonra üreticinin kurtarma manevrasına döner; 400 ms boyunca çizgi bulamazsa **kendi kendine fren yapar** (failsafe). Bu ayar 5–9 cm'lik kesikleri geçmeli. Zigzag/90° köşeler kurtarma manevrasıyla dönülür.
- **B planı (bant):** kesik bölgede robot dönüyorsa/kayboluyorsa ve zaman daralıyorsa → kesikleri bantla kapat, kesintisiz çizgiyle turu tamamlamaya odaklan. (Bugünün hedefi ilk tam tur; kesik geçişi haftaya da iyileştirilebilir.) B bandını kullandıysan kayıt şablonuna yaz.
- GAP ayarı gerekiyorsa: `ATLAS_Takim.ino` içindeki `OFFLINE_GAP_BRIDGE_MS` (denenecek dizi: 60 → 100 → 150; büyüttükçe kör uçuş süresi artar), sonra yeniden yükle. Her değişiklikte derleyip USBasp ile yükle.

## 6. Gözlem ve kayıt (her koşu)

| Alan | Örnek |
|---|---|
| Koşu no / saat | #1 / 10:32 |
| PWM / KP / KD / türbin | 60 / 0.05 / 0.35 / 200 |
| Pil hücre V (önce/sonra) | 4.18 → 4.11 |
| Sonuç | bitti / kaçış / kilit / failsafe |
| Kayıp noktası (pistte işaretle) | sağ üst zigzag #2 köşe |
| Video dosyası | IMG_4021.MOV |

Kurallar: tek seferde tek parametre değişir; başarısız koşular da not edilir; "10 denemede 10 bitiriş" takım içi geçiş hedefi.

## 7. Hızlı sorun tablosu

| Belirti | Muhtemel neden | İlk bakılacak |
|---|---|---|
| Yüklemede "programmer is not responding" | Zadig sürücüsü eksik / şerit ters | Zadig WinUSB; pin-1 |
| Seri monitörde anlamsız karakter | Baud | 115200 seç |
| GO hep 1 görünüyor | Sinyal GO pininde değil / modül beslemesi yok | start portu etiketleri; +/− voltajı |
| Kalibrasyon kapısı hiç geçmiyor | Kalibrasyon sallantısı az / sensör bar yüksek | daha geniş salla; bar yüksekliğini ölç |
| Start'tan hemen sonra stop'a düşüyor | STOP teyidi 30 ms gürültü yakalıyor | `STOP_SIGNAL_CONFIRM_MS` 50 yap |
| Kesikte kaybolup dönüyor | GAP_BRIDGE az | 60→100→150 dene, yetmezse B planı |
| Düzde salınım (zigzag sallantı) | KP yüksek ya da hız yüksek | önce PWM düşür; sonra KP 0.04 |
| Virajda taşıyor | hız yüksek / KD az | PWM düşür; KD 0.35→0.45 |
| Açık kalma sonrası garip davranış | regülatör ısınması | kapat, soğut, süreyi kısalt |

## 8. Günün başarı ölçütü (gerçekçi)

1. Debug tüm testler geçti.
2. MEBSTART: START→ön-vakum→koşu, STOP→fren→kilit; hepsi masada ve pistte doğrulandı.
3. PWM 60–100 bandında **en az 1 tam tur** (kesiksiz veya bantlı bölümle).
4. En az 5 kayıtlı koşu + 1 video.

Bunlar tutarsa ikinci oturum: hız artırma, kesiklerde bantsız geçiş, kronometre devreye alma.
