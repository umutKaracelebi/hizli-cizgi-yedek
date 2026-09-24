/*
  ATLAS_Tekerlek_Debug.ino
  ========================
  ATLAS Rev 1.4 Hızlı Çizgi İzleyen Robot - TRİBÜN (VAKUM) MOTOR TEST KODU
  
  ÇALIŞMA MANTIĞI:
  1. Karta enerji verildiğinde (veya Reset atıldığında) tam 8 saniye geri sayar.
  2. 8 saniye boyunca LED'ler her saniye çakar (robotu düz ve güvenli bir zemine koymanız için süre tanır).
  3. 8 saniye bitince sürücüler uyandırılır (INH = 1).
  4. Tekerlekler KESİNLİKLE DÖNMEZ (0 PWM).
  5. Sadece TRİBÜN (vakum) motoru fabrika standardı güçte (PWM 200 / ~%78 güç) devreye girer.
  6. Acil durdurma: Robot üzerindeki SW1 veya SW2 butonuna basıldığında tribün anında durur ve kilitlenir.

  ÖNEMLİ DONANIM NOTU (ATLAS Rev 1.4 Kılavuz s.19):
  Kart üzerindeki SB1 lehim köprüsü kapalı değilse, tribünün 2 pinli güç kablosunun
  pilin balans soketine (2S - 7.4V uçlarına) takılı olduğundan emin olun.
*/

// ==========================================
//  P I N   T A N I M L A R I  (Ana Koddan)
// ==========================================
#define PIN_INH   12   // Motor sürücüleri uyandırma / inhibit (Aktif HIGH)
#define PIN_INL1   6   // Sol motor yön pini
#define PIN_INL2   9   // Sol motor PWM (Timer1 OC1A)
#define PIN_INR1   5   // Sağ motor yön pini
#define PIN_INR2  10   // Sağ motor PWM (Timer1 OC1B)
#define PIN_PWMC  11   // Tribün (impeller) MOSFET pini (Timer2 OC2A)

// LED ve Buton Pinleri
#define PIN_LED0   2   // PD2
#define PIN_LED1   8   // PB0
#define PIN_LED2  13   // PB5 (Dahili LED)
#define PIN_SW1   A5   // PC5 (Buton 1)
#define PIN_SW2    7   // PD7 (Buton 2)

// ==========================================
//  A Y A R L A R
// ==========================================
#define BEKLEME_SURESI_SN   8    // Enerji verildikten sonraki bekleme süresi (saniye)
#define TURBIN_TEST_PWM   200    // Tribün test gücü (0-255 arası, 200 = ATLAS fabrika varsayılanı)

#define MOTORS_PWM_PULSE_MAX (400 - 1)  // Timer1 20 kHz Fast PWM tavan değeri
#define MOTORS_PWM_RESOLUTION 255

// ==========================================
//  Y A R D I M C I   F O N K S İ Y O N L A R
// ==========================================

void enableMotorDrivers() {
  digitalWrite(PIN_INH, HIGH);
  delayMicroseconds(10);
}

void disableMotorDrivers() {
  digitalWrite(PIN_INH, LOW);
  digitalWrite(PIN_INL1, LOW);
  digitalWrite(PIN_INR1, LOW);
  OCR1A = 0;
  OCR1B = 0;
  OCR2A = 0;
}

// Tribün Motorunu Sür (Timer2 OC2A / Pin 11)
void setPWM_Impeller(int val) {
  if (val <= 0) {
    OCR2A = 0;
  } else if (val >= 255) {
    OCR2A = 255;
  } else {
    OCR2A = val;
  }
}

bool readStopButton() {
  // SW1 veya SW2 basıldı mı? (Aktif LOW)
  return (digitalRead(PIN_SW1) == LOW) || (digitalRead(PIN_SW2) == LOW);
}

// ==========================================
//  S E T U P
// ==========================================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" ATLAS REV 1.4 - 8 SN TRIBÜN TEST KODU  "));
  Serial.println(F("========================================"));

  // Pin Yönlendirmeleri
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

  // Başlangıçta tüm çıkışlar kesinlikle kapalı
  digitalWrite(PIN_INH, LOW);
  digitalWrite(PIN_INL1, LOW);
  digitalWrite(PIN_INR1, LOW);
  digitalWrite(PIN_PWMC, LOW);

  digitalWrite(PIN_LED0, LOW);
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);

  // Timer 1 Konfigürasyonu (Tekerlekler sıfırda kilitli kalacak)
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(CS10);
  ICR1 = MOTORS_PWM_PULSE_MAX;
  OCR1A = 0;
  OCR1B = 0;

  // Timer 2 Konfigürasyonu (Orijinal ATLAS 31.4 kHz Tribün PWM Modu)
  TCCR2A = _BV(COM2A1) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A = 0;

  //  ----------------------------------------------------
  //  8 SANİYE GERİ SAYIM (Görsel LED ve Seri Port Bildirimi)
  //  ----------------------------------------------------
  Serial.println(F("[BILGI] Enerji verildi. 8 saniye sonra TRIBÜN baslayacak..."));
  Serial.println(F("[BILGI] Tekerlekler KESINLIKLE donmeyecek, sadece vakum calisacak."));

  for (int saniye = BEKLEME_SURESI_SN; saniye > 0; saniye--) {
    Serial.print(F("Tribunun baslamasina son: "));
    Serial.print(saniye);
    Serial.println(F(" saniye"));

    digitalWrite(PIN_LED2, HIGH);
    if (saniye % 2 == 0) {
      digitalWrite(PIN_LED0, HIGH);
      digitalWrite(PIN_LED1, LOW);
    } else {
      digitalWrite(PIN_LED0, LOW);
      digitalWrite(PIN_LED1, HIGH);
    }
    delay(500);

    digitalWrite(PIN_LED2, LOW);
    digitalWrite(PIN_LED0, LOW);
    digitalWrite(PIN_LED1, LOW);
    delay(500);
  }

  //  ----------------------------------------------------
  //  8 SANİYE DOLDU -> TRIBÜNÜ BAŞLAT
  //  ----------------------------------------------------
  Serial.println(F("----------------------------------------"));
  Serial.println(F(">> 8 SANİYE DOLDU! SURUCULER ACILIYOR (INH = 1)..."));
  enableMotorDrivers();  // Sürücüleri uyandır (D12 = HIGH)

  Serial.print(F(">> TRIBUN MOTORU CALISIYOR! PWM: "));
  Serial.println(TURBIN_TEST_PWM);
  Serial.println(F(">> Durdurmak icin robot uzerindeki SW1 veya SW2 tusuna basin."));
  Serial.println(F("----------------------------------------"));

  // LED2 sürekli açık (Tribün devrede göstergesi)
  digitalWrite(PIN_LED2, HIGH);

  // Tekerlekler kesinlikle 0
  OCR1A = 0;
  OCR1B = 0;

  // Tribün motorunu çalıştır
  setPWM_Impeller(TURBIN_TEST_PWM);
}

// ==========================================
//  L O O P
// ==========================================
void loop() {
  // Acil durdurma: Kullanıcı SW1 veya SW2'ye basarsa tribünü anında kapat
  if (readStopButton()) {
    Serial.println(F("\n[DURDURULDU] Butona basildi. Tribun durduruldu ve kilitlendi."));
    setPWM_Impeller(0);
    disableMotorDrivers();

    // Kilitlendiğini belirtmek için LED'leri sırayla çaktır
    while (1) {
      digitalWrite(PIN_LED0, HIGH);
      digitalWrite(PIN_LED1, LOW);
      digitalWrite(PIN_LED2, LOW);
      delay(150);
      digitalWrite(PIN_LED0, LOW);
      digitalWrite(PIN_LED1, HIGH);
      digitalWrite(PIN_LED2, LOW);
      delay(150);
    }
  }

  // Tribün çalışırken LED2 ve LED0 açık kalır
  digitalWrite(PIN_LED0, HIGH);
  delay(100);
}
