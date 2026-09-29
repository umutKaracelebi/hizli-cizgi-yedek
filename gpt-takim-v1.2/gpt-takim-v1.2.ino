/*
  gpt-takim-v1.2.ino - Hızlı Çizgi İzleyen Robot (Team Antigravity Sürümü)
  Taban: ATLAS 1.4.3 (c) 2026 EXOTIC TEAM MX, CC BY-NC-ND 4.0
  
  ANTIGRAVITY DÜZELTMELERİ (2026-09-24, gün. 2026-09-28 saha + inceleme):
   1) MEBSTART Entegrasyonu ve Kararlı Kenar Algılama (Edge-Detection):
      - START_SIGNAL_CONFIRM_MS = 60 ms ile elektriksel gürültü ve buton arkı koruması.
      - Sinyalin önce stabil 5V olduğu doğrulanır; 5V -> 0V düşen kenar şart koşulur.
   2) Ön-Vakum (1 sn) + Yumuşak Kalkış Rampaları (28 Eyl saha düzeltmesi):
      - Kumanda START'ından sonra ÖNCE tribün devreye alınır: 250 ms ramp +
        tam devirde bekleme = toplam PRE_VACUUM_TIME_MS (1000 ms).
      - Bu süre boyunca tekerlek sürücüleri UYKUDA (INH = 0) olduğu için
        tekerlekler KESİNLİKLE dönmez; robot yere emiş tutunarak başlar.
      - Ön-vakum bitince tekerlekler 150 ms rampayla kalkar; tribün koşu
        boyunca TAM DEVİRDE kalır (yeniden rampa / sıfırlama YOK).
      - Ön-vakum sırasında STOP (sinyal 5V) gelirse iptal: tribün kapanır,
        WAIT_IDLE'a dönülür.
   3) Koşu Boyunca Kesintisiz Vakum Güvencesi:
      - Çizgi dışına (offline / köprü) çıkılsa dahi tribün PWM'i asla sıfırlanmaz.
   4) Kumandaya 2. Basışta / Failsafe'te Kontrollü Fren + KALICI KİLİT:
      - Fren tetik anindaki GECERLI tekerlek PWM'lerine mandallanir (H1); o
        degerlerden 500 ms'de sıfıra inilir (pivot sirasinda ileri firlamaz).
      - Tekerlekler ve tribün tamamen kapatılır; sürücüler uyutulur (INH = 0).
      - Robot sonsuz döngüde kilitlenir; 3., 4. vb. sonraki basışlar kesinlikle çalışmaz.
   5) Buton Bırakma (Debounce) ve Kalibrasyon Akış Koruması:
      - Butonlara basıldıktan sonra parmak çekilene kadar beklenir; bir önceki basış
        sonraki menüye veya starta sıçramaz.
   6) Son saha/inceleme düzeltmeleri (27 Eyl):
      - Kesikte tam düz kör devam (OFFLINE_DASH_COAST_MS), EMA siniflandirma,
        yeniden yakalama yumusatma penceresi, sinirli arama butcesi ve gecici takip.
      - Kesisim/tam-siyah ust siniri (INTERSECTION_MAX_MS) ile yanlis failsafe
        ertelemesi engellenir.
      - MAX secilebilir hiz, CONTROL_MAX_PWM_FORWARD - 40 olarak sinirlandi
        (diferansiyel payi kalmasi icin).
   7) Viraj cikisi / kenar temasi duzeltmesi (28 Eyl saha):
      - Kayip siniflandirmasi uc oldu: DUZ KESIK (merkezde, direksiyon
        kucuk), KENAR TEMASI / VIRAJ CIKISI (cizgi bar ucunda gorunuyor)
        ve KESKIN KOSE (bar hicbir cizgi gormuyor + kayip bar ucunda).
      - Yerinde donus (pivot) YALNIZCA keskin kosede yapilir. Viraj
        cikisinda cizgi bar ucunda gorunuyorsa kisitli P ile ILERI takip
        edilir; hic gorunmuyorsa SONEN sinirli direksiyonla ileri devam
        edilir. Saha arizasi ("viraj sonunda geri donme / disa atma") biter.
      - Arama sirasinda bir kez bile temas olduysa TERS FAZ ACILMAZ.
      - Pivot/kenar kaybindan gelen yeniden yakalamada baz hiz kademeli
        acilir (REACQUIRE_BASE_PWM_PCT) -> egik halde tam gazla savrulma yok.
*/

//  =============================
//  U S E R   P A R A M E T E R S
//  =============================

#define MOTOR_L_SIGN NOT_INVERTED  // left motor direction
#define MOTOR_R_SIGN NOT_INVERTED  // right motor direction

#define KP 0.05f  // line control term proportional component coefficient
#define KD 0.35f  // line control term derivative component coefficient

#define BASE_VELOCITY_PWM 100  // velocity selection base PWM
#define MIN_VELOCITY_PWM 60    // velocity selection min PWM
#define MAX_VELOCITY_PWM 160   // H3 (27 Eyl inceleme): CONTROL_MAX_PWM_FORWARD(200) - direksiyon payi(40).
                               // 200 ustu secimde kucuk hatalarda diferansiyel kirpta oluyordu.
#define VELOCITY_PWM_STEP 10   // velocity selection PWM increment/decrement step

#define IMPELLER_PWM 200  // suction impeller fan PWM value

#define RUN_VELOCITY_RAMP_TIME_MS 150  // velocity term PWM rise time in milliseconds
#define RUN_IMPELLER_RAMP_TIME_MS 250  // impeller PWM rise time in milliseconds

// 28 Eyl saha: START alindiktan sonra tekerlekler kalkmadan ONCE tribünün
// tam devirde calistigi (yere emis / vakum) sure. Bu sure boyunca suruculer
// uykuda (INH = 0) kalir; tekerlekler bu pencere bitince rampayla kalkar.
#define PRE_VACUUM_TIME_MS 1000  // on-vakum suresi (ms)

//  =====================================
//  A D V A N C E D   P A R A M E T E R S
//  =====================================

const bool areMotorsEnabled = true;

// TIME
#define CONTROL_LOOP_PERIOD_US 750
#define STOP_BRAKE_TIME_MS 500
#define OFFLINE_INNER_MOTOR_BRAKE_TIME_MS 35

// PWM
#define CONTROL_MAX_PWM_FORWARD 200
#define CONTROL_MAX_PWM_BACKWARD (-120)

#define OFFLINE_OUTTER_MOTOR_PWM CONTROL_MAX_PWM_FORWARD
#define OFFLINE_INNER_MOTOR_PWM CONTROL_MAX_PWM_BACKWARD
// NOT (TEAM, 25 Eylul 2026): yukaridaki iki PWM ve asagidaki fren suresi
// URETICININ eski kurtarma manevrasi icindir ve artik KULLANILMIYOR.
// Yerine "T E A M   P A R A M E T R E L E R I" bolumundeki siniflandirmali
// kopru + SINIRLI yerinde donuslu arama + gecici takip mantigi gecti
// (OFFLINE_SEARCH_PWM, OFFLINE_SEARCH_FIRST_MS, OFFLINE_SEARCH_MAX_MS).
// Referans icin birakildi.

// OTHERS
#define SENSORS_THRESHOLD_PCT 50

//  =================================
//  T E A M   P A R A M E T E R L E R I
//  =================================

// MEBSTART sinyal teyit süreleri (gürültü / buton arkı filtresi)
#define START_SIGNAL_CONFIRM_MS 60   // START: 0V en az bu süre stabil kalırsa geçerli
#define STOP_SIGNAL_CONFIRM_MS 150   // STOP: 5V en az bu süre stabil kalırsa geçerli (motor titreşim koruması)

// ---------------------------------------------------------------------
// CIZGI KAYBI YONETIMI (kesikli cizgi / keskin donus ayrimi)
// ---------------------------------------------------------------------
// SAHA GERI BILDIRIMI (27 Eyl 2026, pist 1 beyaz kesik cizgileri):
// Onceki surumde (120 ms kopru + azalan direksiyon tutma + sinirsiz
// iki yonlu arama) robot kesikte bazen duz geciyor, bazen zikzak
// ciziyor, bazen geldigi cizgiye GERI DONUYOR, bazen de kesikten
// saga sapip pist disina cikiyordu. Koki nedenler:
//   1) 120 ms kopru gercek bosluk suresinden kisa kalabiliyordu
//      (orta hizda 56 mm bosluk ~160 ms); kopru BOSLUGUN ORTASINDA
//      dolunca yerinde donuslu arama beyaz alanda basliyor, robot
//      180 derece donup arkadaki kesik parcasina kilitleniyordu.
//   2) Kopru boyunca kayip anindaki son PID degeri (turev darbesi
//      dahil) +-80'e kadar tutuluyordu -> robota yanal sapma.
//   3) Yeniden yakalamada tek donguluk turev sifirmasi yeterli
//      degildi; sonraki dongude konum sicalamasi buyuk D darbesi
//      uretiyor -> kesikte zikzak.
//   4) Siniflandirma, darbeli ani linePWM'e bakiyordu.
//
// GECERLI TASARIM:
//   - Kayip ani, SONULMIS direksiyon (EMA) ve son konumla siniflanir.
//   - DUZ KESIK: direksiyon tamamen sifirlanarak DUZ kor devam; bu
//     pencerede asla donus/arama yapilmaz. Devam cizgisi merkez
//     bandinda gorunurse kabul; bant disinda KALICI cizgi gorunurse
//     (gercek egri / 90 derece zikzak sonrasi parca) kisa bir kalicilik
//     suresi sonunda kabul edilir. Bosluk pencereyi asarsa guvenli durus.
//   - KESKIN KOSE (28 Eyl güncellemesi): kisa kopru -> sinirli sureli yerinde
//     arama (ilk faz kaybedilen yone kisa, sonra TEK ters yon; toplam butce
//     asilirsa guvenli durus). Boylece 180+ derece donup geri kilitlenme
//     engellenir. Pivot YALNIZCA bar hicbir cizgi gormezken ve kayip bar
//     ucundayken yapilir; arama sirasinda bir kez bile temas olduysa ters
//     faz ACILMAZ.
//   - VIRAJ CIKISI / KENAR TEMASI (28 Eyl): cizgi bar ucunda gorunuyorsa
//     PIVOT YAPILMAZ; kisitli P ile ileri takip edilir, temas TRACK süresi
//     kesintisiz surerse kabul edilir. Hic gorunmuyorsa SONEN sinirli
//     direksiyonla ileri devam edilir (kor viraj takibi), butce dolunca
//     guvenli durus. Saha arizasi: bu iki durumda yerinde donus yapiliyordu
//     -> robot viraj sonunda GERIYE DONUYOR / kendini DISA ATIYORDU.
//   - Her yeniden yakalamadan sonra kisa bir "yumusatma" penceresi:
//     direksiyon duzeltmesi sinirlanir (zikzak/darbe onlemi); pivot/kenar
//     kaybindan geldiyse baz hiz da kademeli acilir (savrulma onlemi).

// Kayip ani siniflandirmasi
// Ayrut esasi KONUM: duz kesikte cizgi kaybolmadan once merkezdedir;
// keskin donus/kose cikisinda bar kenara kaymistir. Direksiyon olcutu
// yalnizca "susturulmus sert pivot"u yakalamak icindir; tek donguluk
// turev darbeleri (sahadaki zikzak hali) +-400'e kirpilip 1/4 EMA'dan
// gectigi icin birkac dongude ~200'u asamaz -> 300 esigi guvenli ayirir.
#define LINE_CENTER_POSITION 7500        // 0..15000 olceginde merkez degeri
#define OFFLINE_DASH_CENTER_BAND 3500    // |sonulmus konum - merkez| bu bant icindeyse "merkezden kayip"
#define OFFLINE_DASH_STEER_MAX 300       // |sonulmus direksiyon| bunun altindaysa "pivot yapmiyordu"

// Kopru (kor devam) sureleri
#define OFFLINE_GAP_BRIDGE_MS 15         // Donus/keskin kose: kisa kor devam
#define OFFLINE_DASH_COAST_MS 250        // Duz kesikli cizgi: DUZ kor devam penceresi

// Yeniden yakalama kapilari
#define OFFLINE_DASH_REACQUIRE_BAND 4500 // Kesik sonrasi devam cizgisi bu bantta beklenir
#define OFFLINE_MIN_CONFIRM_LOOPS 4      // Yeniden yakalama icin ardisik dongu teyidi
#define OFFLINE_DASH_SIDE_CONFIRM_MS 40  // Kesikte bant disi ama KALICI cizgi = gercek egri

// Yeniden yakalama sonrasi yumusatma (kesik zikzagi / darbe onlemi)
#define REACQUIRE_DAMP_MS 100            // Yumusatma penceresi suresi
#define REACQUIRE_STEER_CLAMP 150        // Pencerede en fazla |direksiyon duzeltmesi|
// 28 Eyl saha: pivot/kenar kaybindan gelen yeniden yakalamada robot cizgiye
// gore EGIK olabilir. Yumusatma penceresinde baz hiz bu yuzdeden %100'e
// dogrusal acilir; boylece robot egik halde tam gazla DISA ATILMAZ.
#define REACQUIRE_BASE_PWM_PCT 45        // Damp penceresi basindaki baz hiz (%)

// Arama manevrasi (keskin donus; kisa kopru dolduktan sonra, cizgi hala yok)
// Yerinde donus: dis teker +PWM, ic teker -PWM -> robot ileri kacmaz.
// (27 Eyl saha) Iki yonlu fazlar SONSUZA kadar surebiliyordu; robot 180+
// derece donup geldigi cizgiye kilitleniyordu ("oldugu cizgiden geri dondu").
// Gecerli tasarim:
//  - Once kaybedilen yone KISA tek faz, sonra TEK ters faz; toplam butce
//    SEARCH_MAX ile sinirlidir, asilirsa guvenli durus (asla geri donmez).
//  - Arama sirasinda bir cizgi gorunurse hemen kilitlenilmez: kisitli
//    duzeltmeyle "gecici takip" baslar ve ancak temas SEARCH_TRACK_MS
//    boyunca kesintisiz surerse kabul edilir. Arkadan/egik kisa temaslar
//    (eski geri kilitlenme arizasi) bu sayede elenir.
#define OFFLINE_SEARCH_PWM 120           // Arama donus siddeti (simetrik)
#define OFFLINE_SEARCH_FIRST_MS 100      // Ilk yonde (kaybedilen tarafa) arama suresi
#define OFFLINE_SEARCH_MAX_MS 280        // Toplam arama butcesi; sonra fren + kilit
#define OFFLINE_SEARCH_TRACK_MS 60       // Gecici takipte kesintisiz temas teyidi

// ---------------------------------------------------------------------
// VIRAJ CIKISI / KENAR TEMASI (28 Eyl 2026 saha duzeltmesi)
// ---------------------------------------------------------------------
// Saha geri bildirimi: viraj sonunda (duz cizgiye gecerken) robot bazen
// GERIYE DONUYOR, bazen kendini DISA ATIYORDU. Koki neden: viraj cikisinda
// cizgi barin UCUNDA kalir/kaybolur; bu kayip "keskin kose" gibi
// siniflaniyor ve yerinde donuslu arama (ters faz dahil) calisiyordu.
// Gecerli kurallar:
//  - Pivot (yerinde donus) YALNIZCA bar hicbir cizgi gormezken VE kayip
//    konumu bar ucundayken yapilir (gercek keskin kose).
//  - Arama sirasinda bir kez bile temas olduysa TERS FAZ ACILMAZ (robot
//    cizgiyi zaten bulmustur; geri donmez).
//  - Cizgi bar ucunda gorunuyorsa: pivot YOK; kisitli P ile ileri takip;
//    temas TRACK_MS kesintisiz surerse kabul edilir.
//  - Cizgi hic gorunmuyorsa: SONEN sinirli direksiyonla ileri devam (kor
//    viraj takibi); butce dolunca guvenli durus (asla yerinde donmez).
#define OFFLINE_EDGE_LOSS_BAND 5000      // |sonulmus konum - merkez| >= bu deger: kayip bar ucunda
#define OFFLINE_GUIDED_COAST_MS 150      // Kor viraj takibinde direksiyonun (dogrusal) sonme suresi
#define OFFLINE_GUIDED_STEER_MAX 40      // Kor viraj takibinde en fazla |direksiyon duzeltmesi| (yumusak;
                                         // ic teker baz hizin ALTINA inmez -> yerinde savrulma yok)
#define OFFLINE_GUIDED_BUDGET_MS 250     // Kor viraj takibi butcesi; asilirsa guvenli durus

// Her durumda ust guvenlik agi (fren + kalici kilit)
#define OFFLINE_FAILSAFE_MS 800

// Kesisim/loop gecisinde PID, yalnizca ileri hattan cikis teyit edilince yeniden acilir.
// Sabit bir "hold" suresi yanal kol gorunurken bitebildigi icin robotu saga/sola ceviriyordu.
#define INTERSECTION_MIN_STRAIGHT_MS 100       // kesisime girdikten sonra en az bu kadar duz gec
// Sadece normal, merkezde ve kararlı takipten girilen kesişimlerde PID çıkışı
// sensör deseniyle kapatılır. Virajdaki geniş desen eski kısa hold'u korur.
#define INTERSECTION_APPROACH_CONFIRM_MS 30    // giriş öncesi düz takip teyidi
#define INTERSECTION_ENTRY_CENTER_BAND 3000    // kesişim yaklaşım merkezi (0..15000)
#define INTERSECTION_ENTRY_STEER_MAX 100       // yaklaşımda azami sönümlü direksiyon
#define INTERSECTION_UNQUALIFIED_HOLD_MS 50    // viraj/geniş desen için önceki güvenli hold
#define INTERSECTION_EXIT_CONFIRM_MS 25        // dar ve merkezde ileri cizginin kesintisiz teyidi
#define INTERSECTION_EXIT_CENTER_BAND 2500     // ileri cikis kabul merkezi (0..15000)
#define INTERSECTION_EXIT_MAX_ACTIVE_SENSORS 4 // yatay/yan kol degil, dar devam cizgisi
// Bar TAM siyah kalirsa (pist disi zemin / robot havada) kesisim sonsuza dek
// fail-safe'i ertelememeli; pencere asilinca guvenli durusa gecilir.
#define INTERSECTION_MAX_MS 400

// Kalibrasyon kapısı: her sensörde (max - min) en az bu kadar olmalı (0..255 ölçek)
#define CAL_MIN_CONTRAST 30

//  =====================================================
//  G L O B A L   V A R I A B L E S  /  C O N S T A N T S
//  =====================================================

// sensors
#define TOTAL_SENSORS 16
byte sensorValues[TOTAL_SENSORS];
byte activeSensorsCount = 0;
bool invertSensorReads = false;
byte ADC6_value;
byte ADC7_value;

// time
unsigned long currentTime;

void setup() {
  cli();  // disable global interrupts

  UIInit();
  sensorsInit();
  motorsInit();

  sei();  // enable global interrupts

  bootAnimation();  // boot display animation

  //  ===================
  //  I D L E   S T A T E
  //  ===================

  // SW1 veya SW2 basılana kadar bekle
  while (!readButton_1() && !readButton_2())
    ;

  invertSensorReads = readButton_2();  // SW2 = Siyah çizgi / Beyaz zemin; SW1 = Beyaz çizgi modu

  // GÜVENLİK: Basılan buton bırakılana kadar bekle (kalibrasyona sıçramayı önler)
  while (readButton_1() || readButton_2())
    delay(10);
  delay(150);

  calibrateSensors();

  // Kalibrasyon kapısı — yetersiz kontrast varsa kilitlen
  if (!calibrationValid()) {
    while (1) {
      setLED_0(1);
      setLED_1(0);
      delay(80);
      setLED_0(0);
      setLED_1(1);
      delay(80);
    }
  }

  // GÜVENLİK: Kalibrasyonu bitiren butonun bırakılmasını bekle
  while (readButton_1() || readButton_2())
    delay(10);
  delay(200);

  run();
}

void loop() {
}
