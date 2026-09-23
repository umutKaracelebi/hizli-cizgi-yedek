/*
  ATLAS_MEBSTART_Test.ino
  ========================
  ATLAS 1.4.3 MEBSTART başlatma/durdurma mantığının
  Arduino Uno üzerinde LED'lerle test edilmesi.

  Gerçek koddaki durum makinesi birebir korunmuştur:
    1. Sinyal bekle
    2. 1. tık → 1sn türbin (LED_TURBIN yanar)
    3. 1sn dolunca → sürüş başlar (LED_SURUS de yanar)
    4. 2. tık → her şey durur → KİLİT (bir daha başlamaz)
    * 1sn vakum sırasında 2. tık → iptal & kilit

  BAĞLANTI ŞEMASI:
  ┌─────────────────────────────────────────────┐
  │  Arduino Uno                                │
  │                                             │
  │  D2 ←──── MEBSTART Sinyal (S) pini          │
  │  D3 ────→ LED 1 (Türbin) ────→ 220Ω → GND   │
  │  D4 ────→ LED 2 (Sürüş)  ────→ 220Ω → GND   │
  │  5V ────→ MEBSTART (+) pini                 │
  │  GND ───→ MEBSTART (-) pini                 │
  │  GND ───→ LED'lerin GND bacakları           │
  └─────────────────────────────────────────────┘
*/

// =====================
//  PIN TANIMLARI
// =====================
#define PIN_MEBSTART  2   // MEBSTART sinyal girişi (dijital, pull-up)
#define PIN_LED_TURBIN 3  // LED 1: Türbin motoru simülasyonu
#define PIN_LED_SURUS  4  // LED 2: Sürüş motorları simülasyonu

// =====================
//  ZAMANLAMALAR (Gerçek ATLAS ile aynı)
// =====================
#define VACUUM_RAMP_TIME_MS  1000  // 1 sn ön-vakum süresi
#define DEBOUNCE_MS          100   // Sinyal debounce süresi

// =====================
//  MEBSTART OKUMA
//  Beklemede 5V (HIGH), Tetikte 0V (LOW)
//  readGo() → true: START aktif (0V)
// =====================
bool readGo() {
  return !digitalRead(PIN_MEBSTART);  // Active LOW → ters çevir
}

// =====================
//  SONSUZ KİLİTLENME
//  Robotun bir daha başlamamasını garanti eder.
//  Yeniden başlatmak için RESET tuşuna bas.
// =====================
void lockout() {
  Serial.println(F("[KILITLENDI] Robot kilitli. Yeniden baslatmak icin RESET'e basin."));
  Serial.println(F("Kumandaya basilsa da robot BASLAMAYACAK."));

  while (1) {
    // Kalp atışı animasyonu (LED13 built-in)
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);

    // Kumanda durumunu göster (gerçekten kilitli olduğunu kanıtla)
    if (readGo()) {
      Serial.println(F("  [!] Kumandaya basildi ama KILITLI - tepki yok."));
    }
  }
}

// =====================
//  SETUP
// =====================
void setup() {
  Serial.begin(9600);
  Serial.println(F("================================"));
  Serial.println(F(" ATLAS MEBSTART TEST MODU"));
  Serial.println(F(" LED1=Turbin  LED2=Surus"));
  Serial.println(F("================================"));

  // Pin konfigürasyonu
  pinMode(PIN_MEBSTART, INPUT_PULLUP);  // Dahili pull-up (fail-safe)
  pinMode(PIN_LED_TURBIN, OUTPUT);
  pinMode(PIN_LED_SURUS, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Başlangıçta her şey kapalı
  digitalWrite(PIN_LED_TURBIN, LOW);
  digitalWrite(PIN_LED_SURUS, LOW);

  // ======================================================
  //  1. START SİNYALİ BEKLEME
  // ======================================================
  Serial.println(F("[BEKLEME] Kumandadan START bekleniyor..."));

  while (!readGo()) {
    // Sinyal gelene kadar bekle
    // (Gerçek kodda burada SW1/SW2 ile hız ayarı yapılır)
  }

  Serial.println(F("[START ALINDI] 1sn vakum basliyor..."));

  // ======================================================
  //  2. 1 SANİYE ÖN-VAKUM & ACİL İPTAL
  // ======================================================
  unsigned long vacuumStart = millis();

  while (millis() - vacuumStart < VACUUM_RAMP_TIME_MS) {
    unsigned long elapsed = millis() - vacuumStart;

    // Türbin LED'ini yak (vakum simülasyonu)
    digitalWrite(PIN_LED_TURBIN, HIGH);

    // Acil iptal: 100ms debounce sonrası 2. tık gelirse derhal dur
    if (elapsed > DEBOUNCE_MS && !readGo()) {
      Serial.println(F("[IPTAL] Vakum sirasinda 2. tik! Durduruluyor..."));
      digitalWrite(PIN_LED_TURBIN, LOW);
      digitalWrite(PIN_LED_SURUS, LOW);
      lockout();  // Sonsuz kilitleme
      return;     // Buraya asla ulaşılmaz
    }

    // İlerleme göstergesi (her 200ms'de bir)
    if (elapsed % 200 < 10) {
      Serial.print(F("  Vakum: "));
      Serial.print(elapsed);
      Serial.println(F(" ms"));
    }
  }

  Serial.println(F("[VAKUM TAMAM] Turbin tam devirde. Surus basliyor!"));

  // ======================================================
  //  3. KOŞU BAŞLADI
  // ======================================================
  digitalWrite(PIN_LED_TURBIN, HIGH);  // Türbin açık kalıyor
  digitalWrite(PIN_LED_SURUS, HIGH);   // Sürüş motorları devrede

  Serial.println(F("[KOSU] Robot pistte! Durdurmak icin kumandaya basin."));

  // Koşu döngüsü: 2. tık gelene kadar devam et
  while (true) {
    if (!readGo()) {
      Serial.println(F("[STOP] 2. tik alindi! Fren yapiliyor..."));
      break;
    }
  }

  // ======================================================
  //  4. DURDURMA
  // ======================================================
  digitalWrite(PIN_LED_SURUS, LOW);    // Sürüş motorları kapalı
  digitalWrite(PIN_LED_TURBIN, LOW);   // Türbin kapalı

  Serial.println(F("[FREN TAMAM] Robot durdu."));

  // ======================================================
  //  5. SONSUZ KİLİTLENME (TEK KOŞU GÜVENLİĞİ)
  // ======================================================
  lockout();
}

// =====================
//  LOOP (Boş — tüm mantık setup'ta)
// =====================
void loop() {
  // Gerçek ATLAS kodunda da loop() boştur.
  // Tüm iş setup() → run() → disableRobot() → while(1) akışında biter.
}
