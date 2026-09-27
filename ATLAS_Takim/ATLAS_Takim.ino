/*
  ATLAS_Takim.ino - Hızlı Çizgi İzleyen Robot (Team Antigravity Sürümü)
  Taban: ATLAS 1.4.3 (c) 2026 EXOTIC TEAM MX, CC BY-NC-ND 4.0
  
  ANTIGRAVITY DÜZELTMELERİ (2026-09-24, gün. 2026-09-27 inceleme):
   1) MEBSTART Entegrasyonu ve Kararlı Kenar Algılama (Edge-Detection):
      - START_SIGNAL_CONFIRM_MS = 60 ms ile elektriksel gürültü ve buton arkı koruması.
      - Sinyalin önce stabil 5V olduğu doğrulanır; 5V -> 0V düşen kenar şart koşulur.
   2) Yumuşak Kalkış Rampaları (on-vakum fazi kodda yok; dokumandaki
      eski "1 sn on-vakum" tanimi kaldirildi):
      - Tekerlekler 150 ms, tribün 250 ms rampayla devreye girer (RUN aninda).
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
//   - KESKIN DONUS: kisa kopru -> sinirli sureli yerinde arama
//     (ilk faz kaybedilen yone kisa, sonra bir kez ters yon; toplam
//     butce asilirsa guvenli durus). Boylece 180+ derece donup geri
//     kilitlenme engellenir.
//   - Her yeniden yakalamadan sonra kisa bir "yumusatma" penceresi:
//     direksiyon duzeltmesi sinirlanir (zikzak/darbe onlemi).

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

// Her durumda ust guvenlik agi (fren + kalici kilit)
#define OFFLINE_FAILSAFE_MS 800

// Kesişim / loop geçiş köprüsü: dikey çizgi kesişiminden düz geçiş hold süresi (ms)
#define INTERSECTION_HOLD_TIME_MS 50
// H2 (27 Eyl inceleme): gercek kesisim gecisi ~50-150 ms surer. Bar TAM
// siyah kalirsa (pist disi zemin / robot havada) bu "kesisim" sonsuza dek
// failsafe'i erteler; pencere asilinca guvenli durusa gecilir.
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
