/*
  ATLAS_Debug.ino - Team Turquoise hardware verification sketch
  -------------------------------------------------------------

  Amac: Yaris firmware'i YUKLENMEDEN ONCE robotun tum donanimini
  ayri ayri dogrulamak. String'siz, F()-tabanli; RAM kullanimi dusuk.

  IKI KULLANIM BICIMI:
    A) Bilgisayarla: USB-TTL bagli, seri monitor 115200 baud,
       menuden 1-4 arasi numara gonder.
    B) Bilgisayarsiz (sahada): yalniz SW1/SW2 + LED'ler.
       - SW2 kisa bas: test secimini degistir (1..4 dongu)
       - Secim LED'lerde ikilik gorunur:  LED1=1, LED2=2, LED0=4
         (orn. 3 -> LED2+LED1 yanar, 4 -> yalniz LED0)
       - SW1 kisa bas: secili testi calistir
       - Canli testlerden cikis: SW1+SW2'yi 2 sn birlikte basili tut
       (Turbin testinde cikis tek tus: SW2)

  Guvenlik: Motor testi ancak 2 sn SW1 basili tutulursa baslar.
*/

// ---------- Pinler (uretici kartiyla ayni) ----------
#define PIN_INH   12  // motor surucu bekleme (1 = aktif)
#define PIN_INL1   6  // sol motor yon
#define PIN_INL2   9  // sol motor PWM (Timer1 OC1A)
#define PIN_INR1   5  // sag motor yon
#define PIN_INR2  10  // sag motor PWM (Timer1 OC1B)
#define PIN_PWMC  11  // turbin PWM (Timer2 OC2A)

#define PIN_LED0   2  // PD2
#define PIN_LED1   8  // PB0
#define PIN_LED2  13  // PB5

// SW1 = A5(PC5), SW2 = D7(PD7), ikisi de dahili pull-up, aktif-LOW
#define PIN_SW1   A5
#define PIN_SW2    7

#define PIN_GO     4  // PD4 - MEBSTART sinyal (aktif-LOW: start = 0V)
#define PIN_RDY    3  // PD3 - kullanilmiyor, izleniyor

#define TOTAL_SENSORS 16

// ---------- Durum ----------
byte sensorRaw[TOTAL_SENSORS];

// ---------- Buton yardimcilari ----------
bool sw1() { return !digitalRead(PIN_SW1); }  // basili = true
bool sw2() { return !digitalRead(PIN_SW2); }

void waitRelease() {
  while (sw1() || sw2()) delay(5);
  delay(30);  // debounce
}

// Kisa bas kenari: basili degilken basildi -> birakana kadar bekle -> true
bool pressEdge(bool (*btn)()) {
  if (!btn()) return false;
  waitRelease();
  return true;
}

// Iki butonun birden ms kadar basili kalinca true (canli testlerden CIKIS)
bool comboExit(uint16_t ms) {
  if (!(sw1() && sw2())) return false;
  unsigned long t0 = millis();
  while (sw1() && sw2()) {
    if (millis() - t0 >= ms) {
      waitRelease();
      return true;
    }
    delay(10);
  }
  return false;
}

// ---------- LED yardimcilari ----------
void ledWrite(uint8_t pin, bool on) { digitalWrite(pin, on ? HIGH : LOW); }

// Ureticinin displayNumber eslemesi: LED0=4, LED2=2, LED1=1
void showSel(byte s) {
  ledWrite(PIN_LED0, s & 4);
  ledWrite(PIN_LED2, s & 2);
  ledWrite(PIN_LED1, s & 1);
}

void ledsOff() {
  ledWrite(PIN_LED0, LOW);
  ledWrite(PIN_LED1, LOW);
  ledWrite(PIN_LED2, LOW);
}

void blinkAll(byte times, uint16_t onMs, uint16_t offMs) {
  for (byte i = 0; i < times; i++) {
    ledWrite(PIN_LED0, HIGH); ledWrite(PIN_LED1, HIGH); ledWrite(PIN_LED2, HIGH);
    delay(onMs);
    ledsOff();
    delay(offMs);
  }
}

// =============================================================
//  Kurulum
// =============================================================
void setup() {
  pinMode(PIN_INH, OUTPUT);
  pinMode(PIN_INL1, OUTPUT);
  pinMode(PIN_INL2, OUTPUT);
  pinMode(PIN_INR1, OUTPUT);
  pinMode(PIN_INR2, OUTPUT);
  pinMode(PIN_PWMC, OUTPUT);

  pinMode(PIN_LED0, OUTPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);

  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_GO, INPUT_PULLUP);
  pinMode(PIN_RDY, INPUT_PULLUP);

  digitalWrite(PIN_INH, LOW);  // suruculer beklemede baslar
  allPwmZero();

  // Timer1 (sürüş) ~20 kHz fast-PWM, Timer2 (türbin) ~31 kHz — üreticiyle ayni
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(CS10);
  ICR1 = 399;  // MOTORS_PWM_PULSE_MAX
  OCR1A = 0;
  OCR1B = 0;

  TCCR2A = _BV(COM2A1) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A = 0;

  // Sensor MUX (PC0..PC3 cikis), ADC kanal 4, AVcc, sola hizali 8 bit
  DDRC |= 0x0F;
  ADMUX = _BV(REFS0) | _BV(ADLAR) | 4;
  ADCSRA = (ADCSRA & ~0x07) | _BV(ADPS2);

  Serial.begin(115200);

  blinkAll(3, 120, 120);  // "debug yazilimi acik" isareti (yaristan ayirt edici)
  printBanner();
  printMenu();
}

// =============================================================
//  Ana dongu: seri KOMUT veya BUTON ile menu
// =============================================================
void loop() {
  static byte sel = 1;
  showSel(sel);

  // --- seri yolu ---
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c >= '1' && c <= '4') {
      runTest(c - '0');
      printMenu();
      sel = 1;
      showSel(sel);
    } else if (c == '0' || c == 'h' || c == '?') {
      printMenu();
    }
    return;
  }

  // --- buton yolu ---
  if (pressEdge(sw2)) {           // secim degistir
    sel = sel >= 4 ? 1 : sel + 1;
    showSel(sel);
    Serial.print(F("Secim: ")); Serial.println(sel);
  } else if (pressEdge(sw1)) {    // secileni calistir
    runTest(sel);
    printMenu();
  }
}

void runTest(byte which) {
  switch (which) {
    case 1: testButtonsAndStartPins(); break;
    case 2: testSensors();             break;
    case 3: testMotors();              break;
    case 4: testImpeller();            break;
  }
}

// =============================================================
//  Yardimcilar
// =============================================================
void printBanner() {
  Serial.println(F("=========================================="));
  Serial.println(F(" ATLAS 1.4.3 - TEAM TURQUOISE DONANIM TEST"));
  Serial.println(F("=========================================="));
}

void printMenu() {
  Serial.println(F(""));
  Serial.println(F("[1] Buton + LED + MEBSTART pinleri (canli)"));
  Serial.println(F("[2] 16 cizgi sensoru ham degerler (canli)"));
  Serial.println(F("[3] Tekerlek motorlari testi (TEKERLER HAVADA!)"));
  Serial.println(F("[4] Turbin testi (duz zemin, el uzakta)"));
  Serial.println(F("Seri: numara gonder. Buton: SW2=sec, SW1=baslat."));
  Serial.println(F("Canli test cikis: SW1+SW2 2sn (turbinde SW2)."));
}

void allPwmZero() {
  OCR1A = 0;
  OCR1B = 0;
  OCR2A = 0;
  digitalWrite(PIN_INL1, LOW);
  digitalWrite(PIN_INR1, LOW);
}

void setMotorL(int val) {
  uint16_t pulse = (uint32_t)(val < 0 ? -val : val) * 399 / 255;
  if (val > 0) {
    digitalWrite(PIN_INL1, LOW);
    OCR1A = pulse;
  } else if (val < 0) {
    digitalWrite(PIN_INL1, HIGH);
    OCR1A = 399 - pulse;
  } else {
    digitalWrite(PIN_INL1, LOW);
    OCR1A = 0;
  }
}

void setMotorR(int val) {
  uint16_t pulse = (uint32_t)(val < 0 ? -val : val) * 399 / 255;
  if (val > 0) {
    digitalWrite(PIN_INR1, HIGH);      // sagda polarite ters (ureticiyle ayni)
    OCR1B = 399 - pulse;
  } else if (val < 0) {
    digitalWrite(PIN_INR1, LOW);
    OCR1B = pulse;
  } else {
    digitalWrite(PIN_INR1, LOW);
    OCR1B = 0;
  }
}

void setImpeller(uint8_t val255) {  // 0..255
  OCR2A = val255;
}

void readSensorsRaw() {
  for (byte i = 0; i < TOTAL_SENSORS; i++) {
    PORTC = (PORTC & 0xF0) | i;  // MUX sec

    ADCSRA |= _BV(ADEN) | _BV(ADSC);
    while (bit_is_set(ADCSRA, ADSC))
      ;
    ADCSRA &= ~_BV(ADEN);
    sensorRaw[i] = ADCH;
  }
}

void flushSerialIn() {
  while (Serial.available()) (void)Serial.read();
}

// =============================================================
//  [1] Buton + LED + MEBSTART pinleri
//  Bilgisayarsiz anlam: SW1->LED0, SW2->LED1, MEBSTART START->LED2
// =============================================================
void testButtonsAndStartPins() {
  Serial.println(F(""));
  Serial.println(F("--- [1] BUTON / LED / MEBSTART IZLEME ---"));
  Serial.println(F("Beklenen: beklemede GO(D4)=1; kumanda START'ta 0."));
  Serial.println(F("Cikis: SW1+SW2 2 sn."));

  while (1) {
    if (comboExit(2000)) break;
    if (Serial.available() && (char)Serial.read() == 'x') break;

    const bool b1 = sw1();
    const bool b2 = sw2();
    const bool goLow = (digitalRead(PIN_GO) == LOW);

    ledWrite(PIN_LED0, b1);
    ledWrite(PIN_LED1, b2);
    ledWrite(PIN_LED2, goLow);  // START isteginde LED2 yanar

    Serial.print(F("SW1="));    Serial.print(b1);
    Serial.print(F(" SW2="));   Serial.print(b2);
    Serial.print(F(" GO(D4)=")); Serial.print(digitalRead(PIN_GO));
    Serial.print(goLow ? F("(START)") : F("(bekleme)"));
    Serial.print(F(" RDY(D3)=")); Serial.println(digitalRead(PIN_RDY));
    delay(150);
  }
  flushSerialIn();
  ledsOff();
  Serial.println(F("[1] bitti."));
}

// =============================================================
//  [2] Sensor ham degerler
//  Bilgisayarsiz anlam: en az bir sensor >128 ise LED2 yanar
//  (robotu cizgi/zemin uzerinde gezdir; LED2 degismeliyse dizi canli)
// =============================================================
void testSensors() {
  Serial.println(F(""));
  Serial.println(F("--- [2] SENSOR HAM DEGERLER (0..255) ---"));
  Serial.println(F("Tum sutunlar eli sensore yaklastirinca degismeli."));
  Serial.println(F("Degismeyen sutun = supheli sensor/baglanti."));
  Serial.println(F("Cikis: SW1+SW2 2 sn veya seri 'x'."));

  while (1) {
    if (comboExit(2000)) break;
    if (Serial.available() && (char)Serial.read() == 'x') break;

    readSensorsRaw();

    bool anyHigh = false;
    for (byte i = 0; i < TOTAL_SENSORS; i++) {
      Serial.print(sensorRaw[i]);
      Serial.print('\t');
      if (sensorRaw[i] > 128) anyHigh = true;
    }
    Serial.println();
    ledWrite(PIN_LED2, anyHigh);
    delay(120);
  }
  flushSerialIn();
  ledsOff();
  Serial.println(F("[2] bitti."));
}

// =============================================================
//  [3] Tekerlek motorlari
//  Bilgisayarsiz: sec, tum LED'ler hizli cakar (tekerleri kaldir!),
//  ardindan SW1'i 2 sn basili tut = onay. LED0=SOL, LED1=SAG sirasiyla doner.
// =============================================================
void testMotors() {
  Serial.println(F(""));
  Serial.println(F("--- [3] TEKERLEK MOTOR TESTI ---"));
  Serial.println(F("UYARI: Tekerlekler havadayken ve el uzaktayken."));

  // uyari penceresi: 8 sn icinde onay bekle
  Serial.println(F("Onay: SW1'i 2 sn basili tut (veya seri 'y')."));
  bool ok = false;
  const unsigned long deadline = millis() + 8000;
  while (millis() < deadline) {
    blinkAll(1, 60, 60);
    if (Serial.available()) {
      char c = (char)Serial.read();
      if (c == 'y' || c == 'Y') { ok = true; break; }
      flushSerialIn();
    }
    // SW1 2 sn basili = onay
    if (sw1()) {
      const unsigned long t0 = millis();
      while (sw1() && millis() - t0 < 2000) delay(10);
      if (millis() - t0 >= 2000) { ok = true; waitRelease(); break; }
    }
  }
  flushSerialIn();
  if (!ok) {
    Serial.println(F("Iptal edildi (onay gelmedi)."));
    return;
  }

  Serial.println(F("Suruculer etkinlesiyor, test basliyor..."));
  digitalWrite(PIN_INH, HIGH);
  delayMicroseconds(5);

  const int P = 60;       // uretecinin test PWM'i
  const int T_ON = 400;   // ms
  const int T_OFF = 900;  // ms

  // SOL ileri / geri
  Serial.println(F("SOL ileri"));
  ledWrite(PIN_LED0, HIGH);
  setMotorL(P); delay(T_ON);
  setMotorL(0); delay(T_OFF);
  Serial.println(F("SOL geri"));
  setMotorL(-P); delay(T_ON);
  setMotorL(0); delay(T_OFF);
  ledWrite(PIN_LED0, LOW);

  // SAG ileri / geri
  Serial.println(F("SAG ileri"));
  ledWrite(PIN_LED1, HIGH);
  setMotorR(P); delay(T_ON);
  setMotorR(0); delay(T_OFF);
  Serial.println(F("SAG geri"));
  setMotorR(-P); delay(T_ON);
  setMotorR(0); delay(T_OFF);
  ledWrite(PIN_LED1, LOW);

  allPwmZero();
  digitalWrite(PIN_INH, LOW);
  Serial.println(F("Test bitti. Beklenen: LED0'da SOL teker, LED1'de SAG teker once ileri sonra geri."));
}

// =============================================================
//  [4] Turbin
//  Bilgisayarsiz: SW1 = ac/kapa (LED2 = turbin durumu), SW2 = cikis.
// =============================================================
void testImpeller() {
  Serial.println(F(""));
  Serial.println(F("--- [4] TURBIN TESTI ---"));
  Serial.println(F("Robot duz, temiz zeminde; fan agzi acik, el uzakta."));
  Serial.println(F("SW1 veya seri 't' = ac/kapa. SW2 veya seri 'x' = cikis."));

  bool on = false;
  setImpeller(0);
  ledWrite(PIN_LED2, LOW);

  while (1) {
    if (pressEdge(sw2)) break;                 // butonla cikis
    if (pressEdge(sw1)) {                      // butonla ac/kapa
      on = !on;
      setImpeller(on ? 200 : 0);
      ledWrite(PIN_LED2, on);
      Serial.println(on ? F("TURBIN: ACIK (PWM 200)") : F("TURBIN: KAPALI"));
    }
    if (Serial.available()) {
      char c = (char)Serial.read();
      if (c == 'x' || c == 'X') break;
      if (c == 't' || c == 'T') {
        on = !on;
        setImpeller(on ? 200 : 0);
        ledWrite(PIN_LED2, on);
        Serial.println(on ? F("TURBIN: ACIK (PWM 200)") : F("TURBIN: KAPALI"));
      }
    }
  }
  setImpeller(0);
  ledWrite(PIN_LED2, LOW);
  flushSerialIn();
  Serial.println(F("[4] bitti. Turbin kapali."));
}
