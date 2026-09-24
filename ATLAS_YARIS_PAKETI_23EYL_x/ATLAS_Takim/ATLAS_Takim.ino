/*
  ATLAS_Takim.ino - Team Turquoise yaris surumu
  Taban: ATLAS 1.4.3 (c) 2026 EXOTIC TEAM MX, CC BY-NC-ND 4.0
  Lisans: yalniz takim ici kullanim; yayinlanamaz.

  TEAM TURQUOISE degisiklikleri (2026-09-22):
   1) RunControl.ino: offlineElapsedTime kendini-okuma hatasi duzeltildi
      (offlineStartTime referansi) - DERLEYICI UYARISI GIDERILDI.
   2) MEBSTART entegrasyonu: GO (D4) aktif-LOW (bekleme 5V, START'ta 0V).
      START kenari -> 1 sn on-vakum -> kosu.
      STOP = sinyalin 5V'a donmesi (30 ms teyit) veya SW1+SW2 cift basim.
      500 ms kontrollu fren sonrasi reset'e kadar kilit.
      On-vakum sirasinda STOP gelirse turbin kapanir, ARMED'a donulur.
   3) Kesikli cizgi: cizgi kaybinda ilk OFFLINE_GAP_BRIDGE_MS boyunca
      ayni hizda duz devam (kopru); sonra ureticinin kurtarma manevrasi.
   4) OFFLINE_FAILSAFE_MS: cizgi bu sureden uzun kaybolursa robot
      kendini guvenli durusa alir (bos arazide sonsuz tur atmaz).
   5) Kalibrasyon kapisi: kontrasti dusuk sensor varsa run'a girilmez
      (CAL_MIN_CONTRAST). LED0/LED1 hizli cakim = kalibrasyon hatasi.
   6) debugMode() bu surumden CIKARILDI; donanim testleri icin ayri
      ATLAS_Debug sketch'i kullanilir. Boylece SRAM boslugu artar
      (uretici surumunde 1557/2048 B idi; String/String-birlestirme yok).
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
// (Yalniz takim surumunde var; piste gore ayarlanir.)

// MEBSTART sinyal teyit sureleri (gurultu/sacaklanma filtresi)
#define START_SIGNAL_CONFIRM_MS 20  // START: LOW bu sure surekli gorulurse gecerli
#define STOP_SIGNAL_CONFIRM_MS 30   // STOP: HIGH bu sure surekli gorulurse gecerli

// Kesik/catlak gecme: cizgi kaybolunca ilk bu kadar ms ayni hizda duz devam
#define OFFLINE_GAP_BRIDGE_MS 60
// Cizgi bu sureden uzun kaybolursa guvenli durus (failsafe)
#define OFFLINE_FAILSAFE_MS 400

// Kalibrasyon kapisi: her sensorde (max-min) en az bu kadar olmali (0..255 olcek)
#define CAL_MIN_CONTRAST 30

//  =====================================================
//  G L O B A L   V A R I A B L E S  /  C O N S T A N T S
//  =====================================================

// sensors
#define TOTAL_SENSORS 16
byte sensorValues[TOTAL_SENSORS];
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

  // TEAM NOTU: debugMode bu surumde yok; donanim testi icin ATLAS_Debug
  // sketch'i yuklenir. Burada dogrudan normal akisa gecilir.

  //  ===================
  //  I D L E   S T A T E
  //  ===================

  while (!readButton_1() && !readButton_2())
    ;

  invertSensorReads = readButton_2();  // SW2 = SİYAH cizgi (bizim pist); SW1 = beyaz cizgi modu

  calibrateSensors();

  // TEAM: kalibrasyon kapisi — yetersiz kontrastta run'a girme
  if (!calibrationValid()) {
    // Hizli LED0/LED1 cakimi = kalibrasyon hatasi; reset gerekir.
    while (1) {
      setLED_0(1);
      setLED_1(0);
      delay(80);
      setLED_0(0);
      setLED_1(1);
      delay(80);
    }
  }

  delay(250);

  run();
}

void loop() {
}
