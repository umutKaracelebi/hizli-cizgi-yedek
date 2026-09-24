/*
  RunControl.ino - module code for ATLAS series / Antigravity Version
*/

#define LEFT 0
#define RIGHT 1

// run-state
bool isRunning;

// run time
unsigned long previousLooptime_us;
unsigned long startTime;
unsigned long elapsedTime;

// line control
int previousError;
bool lastDetectedSide;
bool isOnLine;
unsigned long offlineStartTime;

// intersection control
bool inIntersection = false;
unsigned long intersectionStartTime = 0;

// PWM
int velocityPWM = BASE_VELOCITY_PWM;
int velPWMDecrement;
int impPWMDecrement;
int outputPWML;
int outputPWMR;
int outputPWMImp = IMPELLER_PWM;

// stop phase
bool stopBrake_flag = false;
unsigned long stopBrakeStartTime;

void runInit() {
  enableMotorDrivers();

  previousError = 0;
  inIntersection = false;
  intersectionStartTime = 0;

  velPWMDecrement = velocityPWM;
  impPWMDecrement = IMPELLER_PWM;  // Tribün 250 ms rampayla tam devre çıkar

  // start timing
  elapsedTime = 0;
  startTime = millis();
  currentTime = startTime;
  offlineStartTime = startTime;

  previousLooptime_us = micros();
}

void updatePWMDecrementRamps() {
  // stop velocity PWM brake phase
  if (stopBrake_flag) {
    if (currentTime - stopBrakeStartTime >= STOP_BRAKE_TIME_MS) {
      isRunning = false;
    } else {
      velPWMDecrement = velocityPWM * (currentTime - stopBrakeStartTime) / STOP_BRAKE_TIME_MS;
    }
  }

  // velocity PWM rise phase (150 ms yumuşak kalkış)
  if (!stopBrake_flag && velPWMDecrement) {
    if (elapsedTime >= RUN_VELOCITY_RAMP_TIME_MS) {
      velPWMDecrement = 0;
    } else {
      velPWMDecrement = velocityPWM - (velocityPWM * elapsedTime / RUN_VELOCITY_RAMP_TIME_MS);
    }
  }

  // impeller PWM rise phase (250 ms kalkış rampası)
  if (impPWMDecrement) {
    if (elapsedTime >= RUN_IMPELLER_RAMP_TIME_MS) {
      impPWMDecrement = 0;
    } else {
      impPWMDecrement = IMPELLER_PWM - (IMPELLER_PWM * elapsedTime / RUN_IMPELLER_RAMP_TIME_MS);
    }
  }
}

// =====================================================================
//  ANTIGRAVITY: MEBSTART Ana Durum Makinesi
// =====================================================================
//
//   WAIT_IDLE  ──(Sinyal stabil 5V)──> ARMED
//   ARMED      ──(Kumanda 1. basış: 5V -> 0V, 60 ms teyit)──> PREVACUUM
//   PREVACUUM  ──(INH=1, Tekerlek=0, Tribün 1 sn tam devir)──> RUN
//   RUN        ──(Kumanda 2. basış: 0V -> 5V, 50 ms teyit)──> 500 ms fren ──> KALICI KİLİT
//
void run() {
  bool started = false;

  while (!started) {
    //  =========================
    //  W A I T _ I D L E
    //  =========================
    // Sinyalin bekleme seviyesinde (5V / HIGH) ve stabil olduğunu doğrula
    setLEDS(0);
    {
      unsigned long highCount = 0;
      while (highCount < 50) {
        if (readStartIdle()) {
          highCount++;
          delay(1);
        } else {
          highCount = 0;
          delay(1);
        }
      }
    }

    //  =========================
    //  A R M E D
    //  =========================
    // LED0 yavaşça yanıp söner = "Hazır, kumanda sinyali bekliyor".
    // Bu aşamada SW1/SW2 ile hız (PWM) ayarlanabilir.
    {
      unsigned long lowSince = 0;
      bool armedBlink = false;
      unsigned long lastBlink = 0;

      while (1) {
        const bool startHeld = startSignalLow();  // D4 LOW = 0V = START isteği

        if (startHeld) {
          if (lowSince == 0) lowSince = millis();
          if (millis() - lowSince >= START_SIGNAL_CONFIRM_MS) {
            break;  // Gerçek kumanda startı onaylandı!
          }
        } else {
          lowSince = 0;
        }

        // ARMED göstergesi (LED0 yanıp söner)
        if (millis() - lastBlink >= 500) {
          lastBlink = millis();
          armedBlink = !armedBlink;
          setLED_0(armedBlink);
        }

        // Hız (PWM) düşürme / artırma seçimi
        if (readButton_1() && velocityPWM >= MIN_VELOCITY_PWM + VELOCITY_PWM_STEP) {
          setLED_1(1);
          velocityPWM -= VELOCITY_PWM_STEP;
          // Buton bırakılana kadar bekle (ark ve sahte startı önler)
          while (readButton_1()) delay(10);
          delay(100);
          setLED_1(0);
          lowSince = 0;
        } else if (readButton_2() && velocityPWM <= MAX_VELOCITY_PWM - VELOCITY_PWM_STEP) {
          setLED_1(1);
          velocityPWM += VELOCITY_PWM_STEP;
          // Buton bırakılana kadar bekle
          while (readButton_2()) delay(10);
          delay(100);
          setLED_1(0);
          lowSince = 0;
        }
      }
    }

    started = true;  // Kumandadan START onaylandı -> Anında RUN'a geç!
  }

  //  =========================
  //  R U N   (Yarışma Koşusu)
  //  =========================
  setLEDS(1);

  isRunning = true;
  runInit();

  unsigned long stopHighSince = 0;

  while (isRunning) {
    if (micros() - previousLooptime_us >= CONTROL_LOOP_PERIOD_US) {
      previousLooptime_us = micros();

      currentTime = millis();
      elapsedTime = currentTime - startTime;

      updatePeriod();
    }

    // STOP tetikleyicisi (Kumandaya 2. basış veya acil durum SW1+SW2)
    if (!stopBrake_flag) {
      bool stopRequested = false;

      // MEBSTART STOP: İlk 300 ms (0.3 saniye) kalkış parazit koruması
      // Kumandaya 2. basış: Sinyal 5V'a döndü (150 ms teyit)
      if (elapsedTime >= 300 && readStartIdle()) {
        if (stopHighSince == 0) stopHighSince = millis();
        if (millis() - stopHighSince >= STOP_SIGNAL_CONFIRM_MS) {
          stopRequested = true;
        }
      } else {
        stopHighSince = 0;
      }

      // Güvenlik: SW1 ve SW2 butonlarına aynı anda basılırsa anında dur
      if (readButton_1() && readButton_2()) {
        stopRequested = true;
      }

      if (stopRequested) {
        setLEDS(0);
        stopBrakeStartTime = currentTime;
        stopBrake_flag = true;
      }
    }

      // Güvenlik: SW1 ve SW2 butonlarına aynı anda basılırsa anında dur
      if (readButton_1() && readButton_2()) {
        stopRequested = true;
      }

      if (stopRequested) {
        setLEDS(0);
        stopBrakeStartTime = currentTime;
        stopBrake_flag = true;
      }
    }
  }

  // Koşu sonu: 500 ms aktif frenleme bitti -> KALICI GÜVENLİK KİLİDİ
  disableRobot();
}

void updatePeriod() {
  updatePWMDecrementRamps();

  // Çizgi konumunu hesapla
  const unsigned int position = getLinePosition();

  // Tribün PWM'i koşu boyunca DAİMA tam güçte kalır (kesinlikle sıfırlanmaz)
  outputPWMImp = IMPELLER_PWM;

  //  =============================================================
  //   K E S İ Ş İ M   T E S P İ T İ  (Intersection / Loop Detection)
  //  =============================================================
  int minActiveIdx = -1;
  int maxActiveIdx = -1;
  for (byte i = 0; i < TOTAL_SENSORS; i++) {
    if (sensorValues[i] > 0) {
      if (minActiveIdx == -1) minActiveIdx = i;
      maxActiveIdx = i;
    }
  }

  const bool leftWing = (sensorValues[0] > 0 || sensorValues[1] > 0 || sensorValues[2] > 0);
  const bool rightWing = (sensorValues[TOTAL_SENSORS - 3] > 0 || sensorValues[TOTAL_SENSORS - 2] > 0 || sensorValues[TOTAL_SENSORS - 1] > 0);
  const bool centerOnLine = (sensorValues[6] > 0 || sensorValues[7] > 0 || sensorValues[8] > 0 || sensorValues[9] > 0);

  // 1) Tam kesişim: Her iki kanat birden aktif ve en az 4 sensör siyah
  const bool bothWings = (leftWing && rightWing && activeSensorsCount >= 4);

  // 2) Geniş yayılım: Sensör barının büyük bölümüne yayılan siyah çizgi
  const bool wideSpan = (minActiveIdx != -1 && minActiveIdx <= 3 && maxActiveIdx >= (TOTAL_SENSORS - 4) && activeSensorsCount >= 5);

  // 3) Açılı kesişim girişi: Merkezdeyken tek kanat kesişim çizgisine dokundu (arada beyaz boşluk var)
  const bool leftCrossTouch = (leftWing && centerOnLine && !sensorValues[3] && !sensorValues[4] && activeSensorsCount >= 4);
  const bool rightCrossTouch = (rightWing && centerOnLine && !sensorValues[11] && !sensorValues[12] && activeSensorsCount >= 4);

  const bool isIntersectionRaw = bothWings || wideSpan || leftCrossTouch || rightCrossTouch;

  if (isIntersectionRaw) {
    inIntersection = true;
    intersectionStartTime = currentTime;
  } else if (inIntersection) {
    // Kesişim sensörlerden çıktıktan sonra INTERSECTION_HOLD_TIME_MS kadar düz gitmeye devam et
    // Bu hold süresi, arka kenar parazitlerini ve asimetrik sensör çıkışını filtreler
    if (currentTime - intersectionStartTime >= INTERSECTION_HOLD_TIME_MS) {
      inIntersection = false;
    }
  }

  //  =============================================================
  //   D U R U M   Y Ö N E T İ M İ
  //  =============================================================
  if (stopBrake_flag) {
    // 1) FRENLEME AŞAMASI
    const int base = velocityPWM - velPWMDecrement;
    outputPWML = base;
    outputPWMR = base;

  } else if (inIntersection) {
    // 2) KESİŞİM / LOOP GEÇİŞİ (Intersection Pass-Through)
    // Kesişimde (loop dönüşü veya artı/T kesişiminde) yatay çizgi sensörleri yanıltır.
    // Robot 90 derece sapıp ters hatta girmemeli; var olan hızıyla DÜMDÜZ karşıya geçmelidir.
    outputPWML = velocityPWM - velPWMDecrement;
    outputPWMR = velocityPWM - velPWMDecrement;

    // Hata türevini sıfırla ki kesişim çıkışında ani savrulma (derivative kick) olmasın
    previousError = 0;

    // Çizgi üzerindeyiz (kesişim), offline failsafe tetiklenmesin
    offlineStartTime = currentTime;

  } else if (isOnLine && position && position != (TOTAL_SENSORS - 1) * 1000) {
    // 3) NORMAL ÇİZGİ TAKİBİ (PID)
    // Son algılanan yönü sadece tek taraflı belirgin sapmada güncelle (kesişim yönü bozmasın)
    const bool leftActive = (sensorValues[0] || sensorValues[1]);
    const bool rightActive = (sensorValues[TOTAL_SENSORS - 2] || sensorValues[TOTAL_SENSORS - 1]);
    if (leftActive && !rightActive) lastDetectedSide = LEFT;
    else if (rightActive && !leftActive) lastDetectedSide = RIGHT;

    // Hata hesapla
    const int error = position - (TOTAL_SENSORS - 1) * 1000 / 2;
    const int deltaError = error - previousError;
    previousError = error;

    // PID kontrol terimi
    long linePWM = ((float)(error) * KP) + ((float)(deltaError) * KD);

    if (linePWM > 1000) linePWM = 1000;
    else if (linePWM < -1000) linePWM = -1000;

    // Motor PWM değerleri
    outputPWML = velocityPWM - velPWMDecrement + linePWM;
    outputPWMR = velocityPWM - velPWMDecrement - linePWM;

    offlineStartTime = currentTime;

  } else {
    // 4) OFFLINE METHOD (Kesik çizgi köprüsü / Keskin köşe toparlama)
    const unsigned long offlineElapsedTime = currentTime - offlineStartTime;

    // Failsafe: Çizgi çok uzun süre kayıpsa güvenli fren başlat
    if (offlineElapsedTime >= OFFLINE_FAILSAFE_MS && !stopBrake_flag) {
      stopBrakeStartTime = currentTime;
      stopBrake_flag = true;
      setLEDS(0);
    }

    if (offlineElapsedTime <= OFFLINE_GAP_BRIDGE_MS && !stopBrake_flag) {
      // Kesikli çizgi köprüsü: Kısa boşlukta son hızla düz devam
      outputPWML = velocityPWM - velPWMDecrement;
      outputPWMR = velocityPWM - velPWMDecrement;
    } else if (stopBrake_flag) {
      // Fren rampası
      const int base = velocityPWM - velPWMDecrement;
      outputPWML = base;
      outputPWMR = base;
    } else {
      // Köprü süresi bittiğinde agresif köşe arama kurtarma manevrası
      if (lastDetectedSide == LEFT) {
        outputPWML = (offlineElapsedTime - OFFLINE_GAP_BRIDGE_MS) <= OFFLINE_INNER_MOTOR_BRAKE_TIME_MS ? OFFLINE_INNER_MOTOR_PWM : 0;
        outputPWMR = OFFLINE_OUTTER_MOTOR_PWM;
      } else {
        outputPWML = OFFLINE_OUTTER_MOTOR_PWM;
        outputPWMR = (offlineElapsedTime - OFFLINE_GAP_BRIDGE_MS) <= OFFLINE_INNER_MOTOR_BRAKE_TIME_MS ? OFFLINE_INNER_MOTOR_PWM : 0;
      }
    }
  }

  // Motor sınırlandırmaları
  if (outputPWML > CONTROL_MAX_PWM_FORWARD) outputPWML = CONTROL_MAX_PWM_FORWARD;
  else if (outputPWML < CONTROL_MAX_PWM_BACKWARD) outputPWML = CONTROL_MAX_PWM_BACKWARD;

  if (outputPWMR > CONTROL_MAX_PWM_FORWARD) outputPWMR = CONTROL_MAX_PWM_FORWARD;
  else if (outputPWMR < CONTROL_MAX_PWM_BACKWARD) outputPWMR = CONTROL_MAX_PWM_BACKWARD;

  // Motorlara ve tribüne PWM gönder
  setPWM_MotorL(outputPWML);
  setPWM_MotorR(outputPWMR);
  setPWM_Impeller(outputPWMImp);
}

// =====================================================================
//  KALICI GÜVENLİK KİLİDİ (Permanent Lockout)
// =====================================================================
// 2. basışta robot durduktan sonra bu fonksiyona girer ve bir daha
// asla çıkmaz. Kumandaya veya butonlara basılsa da robot tepki vermez.
// Tekrar çalıştırmak için kartın güç anahtarı veya reset butonu gerekir.
void disableRobot() {
  // Motorları ve tribünü tamamen kapat
  setPWM_MotorL(0);
  setPWM_MotorR(0);
  setPWM_Impeller(0);

  // Sürücüleri donanımsal olarak uyut (INH = 0)
  disableMotorDrivers();

  delay(500);

  // Kilit döngüsü: LED0 ve LED1 saniyede bir kısa çakar
  while (1) {
    setLED_0(1);
    setLED_1(1);
    delay(50);
    setLED_0(0);
    setLED_1(0);
    delay(950);
  }
}
