/*
  antigravity-takim.ino - Hızlı Çizgi İzleyen Robot (Team Antigravity Sürümü)
  Taban: ATLAS 1.4.3 (c) 2026 EXOTIC TEAM MX, CC BY-NC-ND 4.0
  
  ANTIGRAVITY DÜZELTMELERİ (2026-09-24):
   1) MEBSTART Entegrasyonu ve Kararlı Kenar Algılama (Edge-Detection):
      - START_SIGNAL_CONFIRM_MS = 60 ms ile elektriksel gürültü ve buton arkı koruması.
      - Sinyalin önce stabil 5V olduğu doğrulanır; 5V -> 0V düşen kenar şart koşulur.
   2) 1 Saniye Kesintisiz Ön Vakum (Pre-Vacuum):
      - Kumandaya 1. basışta önce motor sürücüsü donanımsal olarak uyandırılır (INH = 1).
      - Tekerlekler 1 saniye boyunca kesinlikle 0 PWM'de beklerken tribün tam devrine ulaşır.
      - 1 saniye dolunca tribün hiç kesilmeden tekerleklerle koşu (RUN) başlar.
   3) Koşu Boyunca Kesintisiz Vakum Güvencesi:
      - Çizgi dışına (offline / köprü) çıkılsa dahi tribün PWM'i asla sıfırlanmaz.
   4) Kumandaya 2. Basışta Kontrollü Frenleme ve KALICI KİLİT (Permanent Lockout):
      - 2. basışta 500 ms kontrollü aktif fren uygulanır.
      - Tekerlekler ve tribün tamamen kapatılır; sürücüler uyutulur (INH = 0).
      - Robot sonsuz döngüde kilitlenir; 3., 4. vb. sonraki basışlar kesinlikle çalışmaz.
   5) Buton Bırakma (Debounce) ve Kalibrasyon Akış Koruması:
      - Butonlara basıldıktan sonra parmak çekilene kadar beklenir; bir önceki basış
        sonraki menüye veya starta sıçramaz.
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
#define MAX_VELOCITY_PWM 250   // velocity selection max PWM
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
#define START_IMPELLER_RAMP_TIME_MS 1000
#define STOP_BRAKE_TIME_MS 500
#define OFFLINE_INNER_MOTOR_BRAKE_TIME_MS 35

// PWM
#define CONTROL_MAX_PWM_FORWARD 200
#define CONTROL_MAX_PWM_BACKWARD (-120)

#define OFFLINE_OUTTER_MOTOR_PWM CONTROL_MAX_PWM_FORWARD
#define OFFLINE_INNER_MOTOR_PWM CONTROL_MAX_PWM_BACKWARD

// OTHERS
#define SENSORS_THRESHOLD_PCT 50

//  =================================
//  T E A M   P A R A M E T E R L E R I
//  =================================

// MEBSTART sinyal teyit süreleri (gürültü / buton arkı filtresi)
#define START_SIGNAL_CONFIRM_MS 60   // START: 0V en az bu süre stabil kalırsa geçerli
#define STOP_SIGNAL_CONFIRM_MS 150   // STOP: 5V en az bu süre stabil kalırsa geçerli (motor titreşim koruması)

// Kesik çizgi köprüleri:
#define OFFLINE_GAP_BRIDGE_MS 15     // Köşe/zikzak dönüşlerinde gecikme yapmaması için 15 ms
#define OFFLINE_DASH_BRIDGE_MS 75    // Düz kesikli çizgi (beyaz boşluk) geçiş köprüsü (75 ms)
// Çizgi kaybolursa toparlama süresi tanınır (1500 ms)
#define OFFLINE_FAILSAFE_MS 1500

// Kesişim / loop geçiş köprüsü: dikey çizgi kesişiminden düz geçiş hold süresi (ms)
#define INTERSECTION_HOLD_TIME_MS 50

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
