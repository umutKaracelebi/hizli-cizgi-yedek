/*
  RunControl.ino - module code for ATLAS series

    Created on: Mar, 2026
    Edited by Mauricio Tovar

    ATLAS 1.4.3 © 2026 by EXOTIC TEAM MX is licensed under Creative Commons
    Attribution-NonCommercial-NoDerivatives 4.0 International. To view a copy of
    this license, visit https://creativecommons.org/licenses/by-nc-nd/4.0/

  This code is part of the ATLAS_1.4.3.ino original firmware.

  ---------------------------------------------------------------
  TEAM TURQUOISE surumu (2026-09-22) - kok surume gore farklar:
   1) updatePeriod(): offlineElapsedTime tanimisiz-okuma hatasi
      duzeltildi (offlineStartTime kullaniliyor).
   2) run(): MEBSTART durum makinesi (D4, aktif-LOW tek sinyal):
         WAIT_IDLE  : sinyal bosta (5V) beklenir
         ARMED      : hiz secimi + START bekleme (20 ms teyit)
         PREVACUUM  : 1 sn turbin rampasi; STOP gelirse iptal
         RUN        : kosu
         STOP       : sinyal 5V'a dondu (30 ms teyit) veya SW1+SW2
                      -> 500 ms kontrollu fren -> reset kilidi
      Sinyal kaldirilirsa/kablo koparsa pull-up HIGH = STOP yonunde
      guvenli davranir (UI.ino'da INPUT_PULLUP acildi).
   3) Kesikli cizgi koprusu + kalici-kayip guvenli durusu
      (OFFLINE_GAP_BRIDGE_MS / OFFLINE_FAILSAFE_MS, ana sekmede).
   4) Offline dalina da fren rampasi uygulanir (kok kodda yoktu).
  Lisans: CC BY-NC-ND 4.0 - yalniz takim ici kullanim, yayinlanamaz.
  ---------------------------------------------------------------
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

// PWM
int velocityPWM = BASE_VELOCITY_PWM;
int velPWMDecrement;
int impPWMDecrement;
int outputPWML;
int outputPWMR;
int outputPWMImp;

// stop phase
bool stopBrake_flag = false;
unsigned long stopBrakeStartTime;

// TEAM: MEBSTART sinyal teyit sayani (run() icinde gomulu)

void runInit() {
  enableMotorDrivers();

  previousError = 0;

  velPWMDecrement = velocityPWM;
  impPWMDecrement = IMPELLER_PWM;

  // start timming
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

  // velocity PWM rise phase
  if (!stopBrake_flag && velPWMDecrement) {
    if (elapsedTime >= RUN_VELOCITY_RAMP_TIME_MS) {
      velPWMDecrement = 0;
    } else velPWMDecrement = velocityPWM - (velocityPWM * elapsedTime / RUN_VELOCITY_RAMP_TIME_MS);
  }

  // impeller PWM rise phase
  if (impPWMDecrement) {
    if (elapsedTime >= RUN_IMPELLER_RAMP_TIME_MS) {
      impPWMDecrement = 0;
    } else impPWMDecrement = IMPELLER_PWM - (IMPELLER_PWM * elapsedTime / RUN_IMPELLER_RAMP_TIME_MS);
  }
}

// =====================================================================
//  TEAM: MEBSTART ana durum makinesi
// =====================================================================
//
//   WAIT_IDLE ──(sinyal 5V, sabit)──> ARMED
//   ARMED ──(START: sinyal 0V, 20 ms teyit)──> PREVACUUM ──(1 sn doldu)──> RUN
//   PREVACUUM ──(STOP: sinyal 5V)──> iptal, ARMED'a don
//   RUN ──(STOP: sinyal 5V, 30 ms teyit  |  SW1+SW2)──> 500 ms fren ──> KILIT (reset gerekir)
//
void run() {
  bool started = false;

  while (!started) {
    //  =========================
    //  W A I T _ I D L E
    //  =========================
    // Sinyal bekleme konumuna (HIGH) gelene kadar bekle. Sinyal zaten
    // HIGH ise bu dongu aninda gecer. Kisa sacaklanmalar onemsizdir.
    // LED'ler kapali: "hazir degilim" gostergesi.
    setLEDS(0);
    while (!readStartIdle())
      ;

    //  =========================
    //  A R M E D
    //  =========================
    // LED0 yavas yanip soner = "silahli, START bekliyor".
    // Bu asamada SW1/SW2 ile hiz secimi yapilabilir (uretici davranisi).
    {
      unsigned long lowSince = 0;
      bool armedBlink = false;
      unsigned long lastBlink = 0;

      while (1) {
        const bool startHeld = startSignalLow();  // D4 LOW = START istegi

        if (startHeld) {
          if (lowSince == 0) lowSince = millis();
          if (millis() - lowSince >= START_SIGNAL_CONFIRM_MS) break;  // START gecerli
        } else {
          lowSince = 0;
        }

        // armed gostergesi
        if (millis() - lastBlink >= 500) {
          lastBlink = millis();
          armedBlink = !armedBlink;
          setLED_0(armedBlink);
        }

        // velocity PWM selector (uretici davranisi ile ayni; 500 ms bloklama
        // sadece ARMED'da olur, RUN'da degil)
        if (readButton_1() && velocityPWM >= MIN_VELOCITY_PWM + VELOCITY_PWM_STEP) {
          setLED_1(1);
          delay(250);
          setLED_1(0);
          delay(250);
          velocityPWM -= VELOCITY_PWM_STEP;
          lowSince = 0;  // tus basimi sirasinda sinyal gürültüsü start sayilmasin
        } else if (readButton_2() && velocityPWM <= MAX_VELOCITY_PWM - VELOCITY_PWM_STEP) {
          setLED_1(1);
          delay(250);
          setLED_1(0);
          delay(250);
          velocityPWM += VELOCITY_PWM_STEP;
          lowSince = 0;
        }
      }
    }

    //  =========================
    //  P R E V A C U U M   (1 sn turbin rampasi)
    //  =========================
    // Bu asama boyunca STOP gelirse (sinyal 5V'a donerse) turbin kapanir
    // ve WAIT_IDLE'dan basa donulur.
    {
      const unsigned long vacStart = millis();
      unsigned long highSince = 0;
      bool cancelled = false;

      while (millis() - vacStart < START_IMPELLER_RAMP_TIME_MS) {
        const unsigned long vm = millis() - vacStart;
        setPWM_Impeller((int)((uint32_t)IMPELLER_PWM * vm / START_IMPELLER_RAMP_TIME_MS));

        // ilerleme: LED2 rampayi takip eden hizli yanip sonme
        setLED_2((vm / 100) % 2);

        if (readStartIdle()) {  // sinyal 5V'a dondu = erken STOP
          if (highSince == 0) highSince = millis();
          if (millis() - highSince >= STOP_SIGNAL_CONFIRM_MS) {
            cancelled = true;
            break;
          }
        } else {
          highSince = 0;
        }
      }

      if (cancelled) {
        setPWM_Impeller(0);
        setLEDS(0);
        delay(300);
        continue;  // WAIT_IDLE'a geri don
      }
    }

    started = true;  // on-vakum tamam -> RUN asamasina gec
  }

  //  =========================
  //  R U N
  //  =========================
  setLEDS(1);

  isRunning = true;
  runInit();
  impPWMDecrement = 0;      // turbin zaten tam devirde (on-vakumdan)
  setPWM_Impeller(IMPELLER_PWM);

  unsigned long stopHighSince = 0;

  while (isRunning) {
    if (micros() - previousLooptime_us >= CONTROL_LOOP_PERIOD_US) {
      previousLooptime_us = micros();

      // update time measurements
      currentTime = millis();
      elapsedTime = currentTime - startTime;

      updatePeriod();
    }

    // stop trigger
    if (!stopBrake_flag) {
      bool stopRequested = false;

      // MEBSTART STOP: sinyal 5V'a dondu, surekli 30 ms kalsin
      if (readStartIdle()) {
        if (stopHighSince == 0) stopHighSince = millis();
        if (millis() - stopHighSince >= STOP_SIGNAL_CONFIRM_MS) stopRequested = true;
      } else {
        stopHighSince = 0;
      }

      // SW1+SW2 cift basim = aninda STOP (emniyet supabi)
      if (readButton_1() && readButton_2()) stopRequested = true;

      if (stopRequested) {
        setLEDS(0);
        stopBrakeStartTime = currentTime;
        stopBrake_flag = true;
      }
    }
  }

  // end of run -> KILIT (uretici davranisi: reset gerekir)
  disableRobot();
}

void updatePeriod() {

  updatePWMDecrementRamps();

  // get line position
  const unsigned int position = getLinePosition();

  if (isOnLine && position && position != (TOTAL_SENSORS - 1) * 1000) {

    // check and update last detected side
    if (sensorValues[0] || sensorValues[1]) lastDetectedSide = LEFT;
    if (sensorValues[TOTAL_SENSORS - 2] || sensorValues[TOTAL_SENSORS - 1]) lastDetectedSide = RIGHT;

    // get line error
    const int error = position - (TOTAL_SENSORS - 1) * 1000 / 2;

    // get line delta error
    const int deltaError = error - previousError;
    previousError = error;  //  update previous error

    // compute and clamp line control term PWM value
    long linePWM = ((float)(error)*KP) + ((float)(deltaError)*KD);

    if (linePWM > 1000) linePWM = 1000;
    else if (linePWM < -1000) linePWM = -1000;

    // get motor PWM values
    outputPWML = velocityPWM - velPWMDecrement + linePWM;
    outputPWMR = velocityPWM - velPWMDecrement - linePWM;
    outputPWMImp = IMPELLER_PWM - impPWMDecrement;

    // update last time online
    offlineStartTime = currentTime;

  } else {
    //  ===============================
    //    O F F L I N E   M E T H O D
    //  ===============================

    // TEAM FIX: kok kodda `currentTime - offlineElapsedTime` yaziliydi;
    // bu, degiskeni kendisinden cikarir (tanimisiz davranis).
    const unsigned long offlineElapsedTime = currentTime - offlineStartTime;

    // TEAM: kalici kayipta guvenli durus kapisi.
    // Not: bu kosul STOP_BRAKE rampasini devreye sokar; isRunning donguden
    // once kontrol edilir, fren 500 ms surer ve kilitlenir.
    if (offlineElapsedTime >= OFFLINE_FAILSAFE_MS && !stopBrake_flag) {
      stopBrakeStartTime = currentTime;
      stopBrake_flag = true;
      setLEDS(0);
    }

    if (offlineElapsedTime <= OFFLINE_GAP_BRIDGE_MS && !stopBrake_flag) {
      // TEAM: kesik/catlak koprusu — kisa boslukta son hizla duz devam.
      // Fren rampasi basladiysa buraya girilmez (yukaridaki kosul).
      outputPWML = velocityPWM - velPWMDecrement;
      outputPWMR = velocityPWM - velPWMDecrement;
    } else if (stopBrake_flag) {
      // TEAM: offline dalinda da fren rampasi uygula (kok kodda rampa
      // sadece online dalina etkiyordu; stop sonrasi ~500 ms boyunca
      // kurtarma PWM'leri devam edebiliyordu).
      const int base = velocityPWM - velPWMDecrement;
      outputPWML = base;
      outputPWMR = base;
    } else {
      // uretici kurtarma manevrasi (kopru penceresi dolduktan sonra)
      if (lastDetectedSide == LEFT) {
        outputPWML = (offlineElapsedTime - OFFLINE_GAP_BRIDGE_MS) <= OFFLINE_INNER_MOTOR_BRAKE_TIME_MS ? OFFLINE_INNER_MOTOR_PWM : 0;
        outputPWMR = OFFLINE_OUTTER_MOTOR_PWM;
      } else {
        outputPWML = OFFLINE_OUTTER_MOTOR_PWM;
        outputPWMR = (offlineElapsedTime - OFFLINE_GAP_BRIDGE_MS) <= OFFLINE_INNER_MOTOR_BRAKE_TIME_MS ? OFFLINE_INNER_MOTOR_PWM : 0;
      }
    }
  }

  // clamp motors PWM values
  if (outputPWML > CONTROL_MAX_PWM_FORWARD) outputPWML = CONTROL_MAX_PWM_FORWARD;
  else if (outputPWML < CONTROL_MAX_PWM_BACKWARD) outputPWML = CONTROL_MAX_PWM_BACKWARD;

  if (outputPWMR > CONTROL_MAX_PWM_FORWARD) outputPWMR = CONTROL_MAX_PWM_FORWARD;
  else if (outputPWMR < CONTROL_MAX_PWM_BACKWARD) outputPWMR = CONTROL_MAX_PWM_BACKWARD;

  //  update motors PWM
  setPWM_MotorL(outputPWML);
  setPWM_MotorR(outputPWMR);
  setPWM_Impeller(outputPWMImp);
}

void disableRobot() {
  // motors off
  setPWM_MotorL(0);
  setPWM_MotorR(0);
  setPWM_Impeller(0);
  disableMotorDrivers();

  delay(2000);

  // infinite standby loop (kilit): reset gerekir
  while (1) {
    setLED_0(1);
    setLED_1(1);
    delay(50);
    setLED_0(0);
    setLED_1(0);
    delay(1000);
  }
}

void impellerRamp() {
  int rampPWM = 0;
  startTime = millis();  // impeller PWM ramp start time

  while (rampPWM != IMPELLER_PWM) {
    currentTime = millis();

    if (currentTime - startTime >= START_IMPELLER_RAMP_TIME_MS) {
      rampPWM = IMPELLER_PWM;
    } else {
      rampPWM = IMPELLER_PWM * (currentTime - startTime) / START_IMPELLER_RAMP_TIME_MS;
    }

    setPWM_Impeller(rampPWM);
  }
}
