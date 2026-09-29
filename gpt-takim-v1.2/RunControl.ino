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

// TEAM (Antigravity): cizgi kaybi siniflandirmasi ve yeniden yakalama
bool offlineEpisodeActive = false;    // su an bir kayip/kopru olayi icinde miyiz
bool offlineIsDash = false;           // kayip "duz kesikli cizgi boslugu" mu, yoksa donus mu
// 28 Eyl saha: pivot (yerinde donus) YALNIZCA bar hicbir cizgi gormezken ve
// kayip konumu bar ucundayken yapilir. Cizgi bar ucunda gorunuyorsa (viraj
// cikisi / egik temas) pivot yok -> ileri takip.
bool offlineIsEdgeLoss = false;       // kayip bar ucunda + bar tamamen bos (keskin kose)
bool contactSinceEpisodeStart = false;// kayip olayi boyunca cizgiye en az bir kez degildi mi
bool reacquireFromSearch = false;     // son yeniden yakalama pivot/kenar kaybindan mi geldi
bool offlineSearchSide = false;       // arama manevrasinin ilk yonu (LEFT/RIGHT)
unsigned int lastOnLinePosition = 0;  // kayiptan onceki son gecerli konum (0 = hic yok)
unsigned int posSmooth = 7500;        // konum EMA (alpha=1/8); dash siniflandirmasi icin
int steerSmooth = 0;                  // sonulmus (EMA, giris +-400 kırpik) direksiyon; siniflandirma icin
byte reacquireCount = 0;              // yeniden yakalama teyit sayaci (ardisik dongu)
unsigned long sideSinceMs = 0;        // kesikte bant-disI cizginin ilk gorunme ani (0 = yok)
bool sideSignRight = false;           // bant-disi temasin tarafi (true = sag)
unsigned long dampUntil = 0;          // yeniden yakalama sonrasi yumusatma penceresi sonu

// intersection control
bool inIntersection = false;
unsigned long intersectionEnterMs = 0;  // kesisime ilk giris ani
bool intersectionExitCandidateActive = false;
unsigned long intersectionExitCandidateSinceMs = 0;
bool intersectionUseExitConfirm = false;
unsigned long intersectionLastRawMs = 0;
bool intersectionApproachActive = false;
unsigned long intersectionApproachSinceMs = 0;

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

// H1 (27 Eyl inceleme): fren, tetik anindaki GECERLI tekerlek cikislarina
// mandallanir ve oradan sifira iner. Eski surum freni velocityPWM'den
// baslatiyordu: pivot/arama (120,-120) ya da hiz rampasi sirasinda STOP
// gelirse robot fren yerine tam gaz ileri sicrardi.
int brakeFromPwmL = 0;
int brakeFromPwmR = 0;

void startBrake() {
  if (stopBrake_flag) return;
  brakeFromPwmL = outputPWML;
  brakeFromPwmR = outputPWMR;
  stopBrakeStartTime = currentTime;
  stopBrake_flag = true;
  setLEDS(0);
}

void runInit() {
  enableMotorDrivers();

  previousError = 0;
  inIntersection = false;
  intersectionEnterMs = 0;
  intersectionExitCandidateActive = false;
  intersectionExitCandidateSinceMs = 0;
  intersectionUseExitConfirm = false;
  intersectionLastRawMs = 0;
  intersectionApproachActive = false;
  intersectionApproachSinceMs = 0;
  stopBrake_flag = false;  // kosu baslarken fren her zaman ACIK baslar
  stopBrakeStartTime = 0;
  brakeFromPwmL = 0;
  brakeFromPwmR = 0;

  // TEAM: kayip / yeniden yakalama durumunu sifirla
  offlineEpisodeActive = false;
  offlineIsDash = false;
  offlineIsEdgeLoss = false;           // 28 Eyl: ucuncu kayip sinifi
  contactSinceEpisodeStart = false;    // 28 Eyl: ters faz kilidi icin temas izi
  reacquireFromSearch = false;         // 28 Eyl: kademeli baz hiz icin
  offlineSearchSide = LEFT;
  lastOnLinePosition = 0;  // 0 = "henuz gecerli cizgi konumu yok"
  posSmooth = LINE_CENTER_POSITION;
  steerSmooth = 0;
  reacquireCount = 0;
  sideSinceMs = 0;
  sideSignRight = false;
  dampUntil = 0;

  velPWMDecrement = velocityPWM;
  impPWMDecrement = IMPELLER_PWM;  // On-vakum yoksa RUN basinda rampa kurar; on-vakum varsa runBegin() iptal eder

  // start timing
  elapsedTime = 0;
  startTime = millis();
  currentTime = startTime;
  offlineStartTime = startTime;

  previousLooptime_us = micros();
}

void updatePWMDecrementRamps() {
  // stop velocity PWM brake phase — H1: baz deger tetik aninda mandallandi
  // (brakeFromPwm*), asagida fren dallarinda kullanilir. Burada yalnizca
  // bitis suresi yonetilir.
  if (stopBrake_flag) {
    if (currentTime - stopBrakeStartTime >= STOP_BRAKE_TIME_MS) {
      isRunning = false;
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
//  ANTIGRAVITY: On-Vakum (1 sn) + Kosu Girisi
// =====================================================================
// Saha geri bildirimi (28 Eyl 2026): tribün tekerleklerle AYNI ANDA
// kalkiyordu; robot yere tutunmadan harekete gectigi icin kalkista
// savruluyordu. Gecerli akis:
//
//    START ──> PREVACUUM (PRE_VACUUM_TIME_MS = 1 sn) ──> RUN (tekerlekler)
//
// On-vakum boyunca tekerlek sürücüleri henuz UYKUDA (INH = 0; uyandirma
// runBegin() icindedir) oldugu icin tekerlekler KESINLIKLE donmez; yalnizca
// tribün 250 ms ramp + tam devirde bekleme ile yere emis yapar.
//
// Donus: true  = on-vakum tamamlandi (RUN'a gecilebilir)
//         false = on-vakum sirasinda STOP geldi (tribün kapatildi)
bool preVacuum() {
  const unsigned long vacStart = millis();
  bool stopPending = false;       // sinyal 5V'a dondu (STOP adayi) mi
  unsigned long stopSince = 0;

  while (millis() - vacStart < PRE_VACUUM_TIME_MS) {
    const unsigned long vacElapsed = millis() - vacStart;

    // Tribün rampası: RUN_IMPELLER_RAMP_TIME_MS icinde tam devire cikar ve
    // kalan sure boyunca TAM DEVIRDE kalir (yere tam emis).
    unsigned long rampPWM = vacElapsed * IMPELLER_PWM / RUN_IMPELLER_RAMP_TIME_MS;
    if (rampPWM > IMPELLER_PWM) rampPWM = IMPELLER_PWM;
    setPWM_Impeller((int)rampPWM);

    setLED_2((vacElapsed / 100) % 2);  // LED2: on-vakum ilerleme gostergesi

    // Erken STOP: sinyal bekleme seviyesine (5V) donerse iptal (gurultu icin
    // STOP_SIGNAL_CONFIRM_MS teyidi aynen kosu fazindaki gibidir).
    if (readStartIdle()) {
      if (!stopPending) {
        stopPending = true;
        stopSince = millis();
      }
      if (millis() - stopSince >= STOP_SIGNAL_CONFIRM_MS) {
        setPWM_Impeller(0);
        setLEDS(0);
        return false;
      }
    } else {
      stopPending = false;
    }

    delay(2);  // zaman tabanini ilerlet (rampa/LED hesabi millis() uzerinden)
  }

  // On-vakum bitti: tribün tam devirde, hiz tutuluyor.
  impPWMDecrement = 0;
  outputPWMImp = IMPELLER_PWM;
  setPWM_Impeller(IMPELLER_PWM);
  return true;
}

// RUN fazina giris: durum sifirlama + tribünü on-vakumdan DEVRALMA.
// runInit() tribün rampasini yeniden kurar; on-vakum tamamlandigi icin
// burada iptal edilir -> kosu boyunca tribün sabit tam devirde kalir
// (updatePWMDecrementRamps() impPWMDecrement = 0 gorunce hicbir sey yapmaz).
void runBegin() {
  isRunning = true;
  runInit();
  impPWMDecrement = 0;
  outputPWMImp = IMPELLER_PWM;
  setPWM_Impeller(IMPELLER_PWM);
}

// Kayip epizodu icindeki GECICI TAKIP direksiyonu:
//  - turev terimi YOK (kucuk hamleler), hamle REACQUIRE_STEER_CLAMP ile
//    sinirlidir;
//  - 28 Eyl saha: hamle baz hizla da sinirlanir -> ic teker ASLA geri
//    donmez. Boylece cizgi bar ucundayken (viraj cikisi) yapilan duzeltme
//    yerinde savrulma / DISA ATMA yaratmaz.
int limitProvisionalSteer(int steer, int base) {
  int limit = REACQUIRE_STEER_CLAMP;
  if (base < limit) limit = base;
  if (limit < 0) limit = 0;

  if (steer > limit) steer = limit;
  else if (steer < -limit) steer = -limit;
  return steer;
}

// =====================================================================
//  ANTIGRAVITY: MEBSTART Ana Durum Makinesi
// =====================================================================
//
//   WAIT_IDLE  ──(Sinyal stabil 5V)──> ARMED
//   ARMED      ──(Kumanda 1. basış: 5V -> 0V, 60 ms teyit)──> PREVACUUM
//   PREVACUUM  ──(PRE_VACUUM_TIME_MS boyunca tribün tam devirde; tekerlek
//                 sürücüleri UYKUDA)──> STOP gelirse WAIT_IDLE'a dön
//   RUN        ──tekerlekler 150 ms rampayla kalkar (tribün zaten tam devir)──
//              ──(Kumanda 2. basış: 0V -> 5V, 150 ms teyit)──> 500 ms fren ──> KALICI KİLİT
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

    //  =========================
    //  P R E V A C U U M   (1 sn)
    //  =========================
    // Tribün tam devire cikmadan tekerlekler KALKMAZ: bu fazda tekerlek
    // sürücüleri hala uykuda (INH = 0) oldugu icin robot yerinden oynamaz,
    // yalnizca yere emis (vakum) yapilir.
    if (!preVacuum()) {
      // Erken STOP: tribün kapatildi ve LED'ler sondu -> WAIT_IDLE'a don.
      delay(300);
      continue;
    }

    started = true;  // on-vakum tamam -> RUN'a geç!
  }

  //  =========================
  //  R U N   (Yarışma Koşusu)
  //  =========================
  setLEDS(1);

  runBegin();  // durum sifirlama + tribünü on-vakumdan devralma

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
        startBrake();  // H1: mevcut tekerlek PWM'lerine mandallanir
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

  // Tribün: START'tan sonraki 1 sn'lik on-vakum (preVacuum) sirasinda tam
  // devire cikmistir; runBegin() bu hizi devralir (impPWMDecrement = 0).
  // Sonrasinda kosu boyunca DAIMA tam guclte kalir (kesinlikle sifirlanmaz).
  outputPWMImp = IMPELLER_PWM - impPWMDecrement;

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

  // position == 0 veya 15000 yalnizca barin en ucundaki tek sensoru
  // gosterir; bunlar kesisten sonra ileri cikis olamaz.
  const bool lineFound = (isOnLine && position && position != (TOTAL_SENSORS - 1) * 1000);
  const bool isIntersectionRaw = bothWings || wideSpan || leftCrossTouch || rightCrossTouch;

  const bool isStableIntersectionApproach =
      !offlineEpisodeActive &&
      lineFound &&
      !leftWing &&
      !rightWing &&
      activeSensorsCount <= INTERSECTION_EXIT_MAX_ACTIVE_SENSORS &&
      abs((int)position - LINE_CENTER_POSITION) <= INTERSECTION_ENTRY_CENTER_BAND &&
      abs(steerSmooth) <= INTERSECTION_ENTRY_STEER_MAX;
  const bool intersectionApproachConfirmed =
      intersectionApproachActive &&
      currentTime - intersectionApproachSinceMs >= INTERSECTION_APPROACH_CONFIRM_MS;

  if (!inIntersection && !isIntersectionRaw) {
    if (isStableIntersectionApproach) {
      if (!intersectionApproachActive) {
        intersectionApproachActive = true;
        intersectionApproachSinceMs = currentTime;
      }
    } else {
      intersectionApproachActive = false;
    }
  }

  const bool isForwardIntersectionExit =
      lineFound &&
      !leftWing &&
      !rightWing &&
      activeSensorsCount <= INTERSECTION_EXIT_MAX_ACTIVE_SENSORS &&
      abs((int)position - LINE_CENTER_POSITION) <= INTERSECTION_EXIT_CENTER_BAND;

  if (isIntersectionRaw) {
    if (!inIntersection) {
      intersectionEnterMs = currentTime;
      intersectionUseExitConfirm = intersectionApproachConfirmed;
    }
    inIntersection = true;
    intersectionLastRawMs = currentTime;
    intersectionExitCandidateActive = false;
    intersectionApproachActive = false;
  } else if (inIntersection && !intersectionUseExitConfirm) {
    // Virajda oluşan geniş sensör deseni kesin bir loop kesişimi değildir.
    // Bu yol önceki kısa düz hold'u korur; uzun PID kilidi oluşturmaz.
    if (currentTime - intersectionLastRawMs >= INTERSECTION_UNQUALIFIED_HOLD_MS) {
      inIntersection = false;
    }
  } else if (inIntersection) {
    // Yanal kolun son sensorde gorunmesi PID'yi erken acmamali. Yalnizca
    // dar, merkezde ve kararlı ileri çizgi kesişimin bittiğini kanıtlar.
    if (!isForwardIntersectionExit) {
      intersectionExitCandidateActive = false;
    } else if (!intersectionExitCandidateActive) {
      intersectionExitCandidateActive = true;
      intersectionExitCandidateSinceMs = currentTime;
    } else if (currentTime - intersectionEnterMs >= INTERSECTION_MIN_STRAIGHT_MS &&
               currentTime - intersectionExitCandidateSinceMs >= INTERSECTION_EXIT_CONFIRM_MS) {
      inIntersection = false;
      intersectionExitCandidateActive = false;

      // PID ilk kez calisirken D terimi yalnizca gercek ileri hattin
      // hatasini gorur; onceki "0" atamasi buyuk bir donus darbesi uretiyordu.
      previousError = (int)position - LINE_CENTER_POSITION;
      dampUntil = currentTime + REACQUIRE_DAMP_MS;
    }
  }

  // H2 (27 Eyl inceleme): kesisim MAX suresini asarsa bu artik gercek
  // kesisim degil (tam siyah zemin / pist disi / robot havada). Bu durumda
  // kesisim dali offlineStartTime'i surekli tazeleyip failsafe'i
  // erteliyordu -> robot sonsuza dek duz giderdi. Guvenli durusa gec.
  if (inIntersection && (currentTime - intersectionEnterMs) >= INTERSECTION_MAX_MS) {
    inIntersection = false;
    intersectionExitCandidateActive = false;
    if (!stopBrake_flag) startBrake();
  }

  //  =============================================================
  //   D U R U M   Y Ö N E T İ M İ
  //  =============================================================

  // TEAM: YENIDEN YAKALAMA KAPILARI (27 Eyl + 28 Eyl saha duzeltmesi)
  //  - Kayip yoksa: cizgi her zaman kabul (normal PID takibi).
  //  - DUZ KESIK kaybinda:
  //      * Merkez bantta cizgi -> MIN_CONFIRM ardisik dongude kabul
  //        (beklenen devam cizgisi; tek karelik parazit elenir).
  //      * Bant disinda ama KALICI cizgi (>= SIDE_CONFIRM_MS, ayni
  //        tarafta) -> gercek egri / zikzak sonrasi parca kabulu.
  //        Kisa sureli kenar temaslari (kesik kosesi paraziti) elenir
  //        ve teyit sayacini ZEHIRLEMEZ.
  //  - KESKIN KOSE / VIRAJ CIKISI (dash disi) kaybinda: barin UCUNDAKI
  //    temas da sayilir; kabul icin TRACK_MS boyunca KESINTISIZ temas
  //    sarttir (tek karelik egik temaslar robotun 180+ donup geldigi
  //    cizgiye kilitlenmesini engeller).
  bool lineAccepted = false;
  if (offlineEpisodeActive && !offlineIsDash) {
    // Pivot / viraj / kenar temasi: herhangi bir sensorun gordugu cizgi
    // adaydir; kesintisiz temas kapisi karar verir.
    if (isOnLine) {
      if (sideSinceMs == 0) sideSinceMs = currentTime;
      else if (currentTime - sideSinceMs >= OFFLINE_SEARCH_TRACK_MS) lineAccepted = true;
    } else {
      reacquireCount = 0;
      sideSinceMs = 0;
    }
  } else if (lineFound) {
    if (!offlineEpisodeActive) {
      lineAccepted = true;
    } else {
      // DUZ KESIK (dash) kapisi (27 Eyl): merkez bandi + kalici kenar temasi.
      const int posErrAbs = abs((int)position - LINE_CENTER_POSITION);
      if (posErrAbs <= OFFLINE_DASH_REACQUIRE_BAND) {
        sideSinceMs = 0;
        if (reacquireCount < 255) reacquireCount++;
        if (reacquireCount >= OFFLINE_MIN_CONFIRM_LOOPS) lineAccepted = true;
      } else {
        reacquireCount = 0;
        const bool sideRight = ((int)position > LINE_CENTER_POSITION);
        if (sideSinceMs == 0 || sideRight != sideSignRight) {
          sideSinceMs = currentTime;  // yeni taraf/yeni temas: sayaci bastan
          sideSignRight = sideRight;
        } else if (currentTime - sideSinceMs >= OFFLINE_DASH_SIDE_CONFIRM_MS) {
          lineAccepted = true;  // bant disi ama kalici: gercek geometri
        }
      }
    }
  } else {
    reacquireCount = 0;
    sideSinceMs = 0;
  }

  if (stopBrake_flag) {
    // 1) FRENLEME AŞAMASI — H1: tetik aninda mandallanmis cikislardan
    // dogrusal olarak sifira inilir (velocityPWM'den baslamaz!)
    const unsigned long brakeElapsed = currentTime - stopBrakeStartTime;
    const long remain = (long)STOP_BRAKE_TIME_MS - (long)brakeElapsed;
    if (remain > 0) {
      outputPWML = (int)((long)brakeFromPwmL * remain / (long)STOP_BRAKE_TIME_MS);
      outputPWMR = (int)((long)brakeFromPwmR * remain / (long)STOP_BRAKE_TIME_MS);
    } else {
      outputPWML = 0;
      outputPWMR = 0;
    }

  } else if (inIntersection) {
    // 2) KESİŞİM / LOOP GEÇİŞİ (Intersection Pass-Through)
    // Kesişimde (loop dönüşü veya artı/T kesişiminde) yatay çizgi sensörleri yanıltır.
    // Robot 90 derece sapıp ters hatta girmemeli; var olan hızıyla DÜMDÜZ karşıya geçmelidir.
    outputPWML = velocityPWM - velPWMDecrement;
    outputPWMR = velocityPWM - velPWMDecrement;
    // Çizgi üzerindeyiz (kesişim), offline failsafe tetiklenmesin
    offlineStartTime = currentTime;

    // TEAM: kesiste duz gidiyoruz; kayip baglamini da guncelle
    offlineEpisodeActive = false;
    reacquireCount = 0;
    sideSinceMs = 0;
    steerSmooth = 0;
    posSmooth = position;
    lastOnLinePosition = position;

  } else if (lineAccepted) {
    // 3) NORMAL ÇİZGİ TAKİBİ (PID)
    // Son algılanan yönü sadece tek taraflı belirgin sapmada güncelle (kesişim yönü bozmasın)
    const bool leftActive = (sensorValues[0] || sensorValues[1]);
    const bool rightActive = (sensorValues[TOTAL_SENSORS - 2] || sensorValues[TOTAL_SENSORS - 1]);
    if (leftActive && !rightActive) lastDetectedSide = LEFT;
    else if (rightActive && !leftActive) lastDetectedSide = RIGHT;

    // Hata hesapla
    const int error = position - LINE_CENTER_POSITION;

    // TEAM: Kayip/kopru sonrasi ilk kabul dongusunde deltaError = 0
    // (turev darbesi onlemi). Ayrica kisa bir yumusatma penceresi
    // baslar: bu pencerede direksiyon duzeltmesi REACQUIRE_STEER_CLAMP
    // ile sinirlanir; kesik girisindeki konum sicalamasi robota zikzak
    // olarak yansimaz.
    int deltaError = 0;
    if (offlineEpisodeActive) {
      offlineEpisodeActive = false;  // kayip olayi kapandi
      // 28 Eyl: pivot/kenar kaybindan geldiyse baz hiz kademeli acilir
      // (asagida); duz kesik/kopruden gelindiyse tam hizla devam edilir.
      reacquireFromSearch = !offlineIsDash;
      reacquireCount = 0;
      sideSinceMs = 0;
      dampUntil = currentTime + REACQUIRE_DAMP_MS;
    } else {
      deltaError = error - previousError;
    }
    previousError = error;

    // PD kontrol terimi
    long linePWM = ((float)(error) * KP) + ((float)(deltaError) * KD);

    if (linePWM > 1000) linePWM = 1000;
    else if (linePWM < -1000) linePWM = -1000;

    // Yeniden yakalama yumusatma penceresi
    if (currentTime < dampUntil) {
      if (linePWM > REACQUIRE_STEER_CLAMP) linePWM = REACQUIRE_STEER_CLAMP;
      else if (linePWM < -REACQUIRE_STEER_CLAMP) linePWM = -REACQUIRE_STEER_CLAMP;
    }

    // Motor PWM değerleri
    // 28 Eyl saha: pivot/kenar kaybindan gelen yeniden yakalamada robot
    // cizgiye gore EGIK olabilir. Yumusatma penceresinde baz hiz
    // REACQUIRE_BASE_PWM_PCT'ten %100'e dogrusal acilir; boylece robot
    // egik halde tam gazla DISA ATILMAZ (pist disina savrulma onlemi).
    int basePWM = velocityPWM - velPWMDecrement;
    if (reacquireFromSearch && currentTime < dampUntil) {
      const unsigned long dampElapsed = currentTime - (dampUntil - REACQUIRE_DAMP_MS);
      const int basePct = REACQUIRE_BASE_PWM_PCT +
                          (100 - REACQUIRE_BASE_PWM_PCT) * (int)dampElapsed / (int)REACQUIRE_DAMP_MS;
      basePWM = basePWM * basePct / 100;
    }
    outputPWML = basePWM + linePWM;
    outputPWMR = basePWM - linePWM;

    // TEAM: kayip aninda siniflandirma icin baglam bilgisi saklanir.
    // steerSmooth: ani PD darbesinden arindirilmis EMA (alpha=1/4, giris
    // +-400'e kırpilir); posSmooth: konum EMA (alpha=1/8). Siniflandirma
    // bu sonulmus degerlerle yapilir; tek donguluk turev/konum sicalamasi
    // duz kesigi "donus" gibi gosteremez (sahadaki saga-sapma arizasi).
    int steerIn = (int)linePWM;
    if (steerIn > 400) steerIn = 400;
    else if (steerIn < -400) steerIn = -400;
    steerSmooth += (steerIn - steerSmooth) / 4;
    posSmooth += ((int)position - (int)posSmooth) / 8;
    lastOnLinePosition = position;

    offlineStartTime = currentTime;

  } else {
    // 4) OFFLINE METHOD (kesik cizgi koprusu / sinirli arama / guvenli durus)
    const unsigned long offlineElapsedTime = currentTime - offlineStartTime;

    // TEAM: Kaybin ilk dongusunde kayip baglami siniflandirilir.
    // Siniflandirma SONULMUS direksiyon (steerSmooth) ile yapilir; boylece
    // bosluk girisindeki ani turev darbesi duz kesigi "donus" gibi
    // gostererek boslugun ortasinda arama baslatamaz (saha arizasiydi:
    // robot saga sapip pist disina cikiyor / geri donuyordu).
    //  - Konum merkez bandinda VE sonulmus direksiyon kucukse:
    //    "duz kesikli cizgi boslugu" -> DUZ kor devam, asla donme.
    //  - Kayip bar UCUNDA VE bar hicbir cizgi gormuyorken: "KESKIN KOSE"
    //    -> kisa kopru + SINIRLI yerinde arama (tek ters faz hakki).
    //  - Digerleri (viraj cikisi / egik kenar temasi): PIVOT YOK. Cizgi
    //    bar ucunda gorunuyorsa kisitli P ile ileri takip; hic gorunmuyorsa
    //    SONEN sinirli direksiyonla ileri devam. (28 Eyl saha arizasi:
    //    burada yerinde donus yapiliyordu -> robot viraj sonunda GERIYE
    //    DONUYOR / kendini DISA ATIYORDU.)
    if (!offlineEpisodeActive) {
      offlineEpisodeActive = true;
      reacquireCount = 0;
      sideSinceMs = 0;
      contactSinceEpisodeStart = false;  // 28 Eyl: ters faz kilidi
      reacquireFromSearch = false;       // 28 Eyl: kademeli baz hiz
      offlineSearchSide = lastDetectedSide;  // arama manevrasinin ilk yonu

      const int posErr = (int)posSmooth - LINE_CENTER_POSITION;
      offlineIsDash = (lastOnLinePosition != 0) &&
                      (abs(posErr) <= OFFLINE_DASH_CENTER_BAND) &&
                      (abs(steerSmooth) <= OFFLINE_DASH_STEER_MAX);
      offlineIsEdgeLoss = !offlineIsDash &&
                          (abs(posErr) >= OFFLINE_EDGE_LOSS_BAND) &&
                          !isOnLine;
    }

    // TEAM (28 Eyl saha): kayip olayi boyunca cizgiye DEGEN dongu var mi?
    // Bir kez bile temas olduysa pivot TERS FAZA (geri donuse) GECILMEZ.
    if (isOnLine) contactSinceEpisodeStart = true;

    // Ust guvenlik agi: cizgi bu surede kabul edilemediyse fren + kilit
    if (offlineElapsedTime >= OFFLINE_FAILSAFE_MS && !stopBrake_flag) {
      startBrake();
    }

    if (stopBrake_flag) {
      // Fren rampasi — H1: mandalli cikislardan sifira
      const unsigned long brakeElapsed = currentTime - stopBrakeStartTime;
      const long remain = (long)STOP_BRAKE_TIME_MS - (long)brakeElapsed;
      if (remain > 0) {
        outputPWML = (int)((long)brakeFromPwmL * remain / (long)STOP_BRAKE_TIME_MS);
        outputPWMR = (int)((long)brakeFromPwmR * remain / (long)STOP_BRAKE_TIME_MS);
      } else {
        outputPWML = 0;
        outputPWMR = 0;
      }

    } else if (offlineIsDash) {
      // DUZ KESIKLI CIZGI — KOR DUZ DEVAM:
      // Direksiyon TAMAMEN sifirdir (eski surum kayip anindaki son PID
      // degerini tutuyordu; bu, bosluk girisindeki turev darbesini
      // buzluga tasiyip robotu saga/sola yaylardiyordu). Bu pencerede
      // asla donus/arama yapilmaz: devam cizgisi ileride, merkezdedir ve
      // kabul kapisi onu karsilar. Bosluk pencereyi asarsa guvenli durus.
      const int base = velocityPWM - velPWMDecrement;
      outputPWML = base;
      outputPWMR = base;

      if (offlineElapsedTime >= OFFLINE_DASH_COAST_MS) {
        startBrake();
      }

    } else if (offlineIsEdgeLoss) {
      // KESKIN KOSE (bar tamamen cizgi disinda): SINIRLI yerinde arama.
      // Butce: FIRST sure kaybedilen yone, sonra (MAX'a kadar) TEK ters faz;
      // asilirsa fren + kilit. Arama sirasinda cizgi gorunurse hemen
      // kilitlenilmez: turevsiz ve kisitli duzeltmeyle GECICI TAKIP yapilir;
      // kabul kapisi ancak temas TRACK_MS kesintisiz surerse kabul eder.
      // 28 Eyl: TERS FAZ yalnizca kayip olayi boyunca HIC temas olmadiysa
      // acilir (temas gorulmusse robot cizgiyi zaten bulmustur, geri donmez).
      const int base = velocityPWM - velPWMDecrement;

      if (offlineElapsedTime <= OFFLINE_GAP_BRIDGE_MS) {
        // Kisa kopru: direksiyon birakilir, robot savrulmadan son yonunde
        // bir an daha ilerler.
        outputPWML = base;
        outputPWMR = base;

      } else if (isOnLine) {
        // Gecici takip: kisitli P duzeltmesi (turev yok, buyuk hamle yok;
        // ic teker geri donmez)
        const int steer = limitProvisionalSteer(
            (int)(((float)((int)position - LINE_CENTER_POSITION)) * KP), base);
        outputPWML = base + steer;
        outputPWMR = base - steer;

      } else {
        const unsigned long searchElapsed = offlineElapsedTime - OFFLINE_GAP_BRIDGE_MS;
        if (searchElapsed >= OFFLINE_SEARCH_MAX_MS) {
          // Butce doldu, cizgi yok: guvenli durus
          startBrake();
          outputPWML = brakeFromPwmL;
          outputPWMR = brakeFromPwmR;
        } else {
          // Yerinde donus: once kaybedilen yone kisa faz; ters yon fazi
          // yalnizca HIC temas gorulmediyse acilir.
          const bool searchDir = (searchElapsed < OFFLINE_SEARCH_FIRST_MS ||
                                  contactSinceEpisodeStart)
                                 ? offlineSearchSide : !offlineSearchSide;
          if (searchDir == LEFT) {
            outputPWML = -OFFLINE_SEARCH_PWM;
            outputPWMR = OFFLINE_SEARCH_PWM;
          } else {
            outputPWML = OFFLINE_SEARCH_PWM;
            outputPWMR = -OFFLINE_SEARCH_PWM;
          }
        }
      }

    } else if (isOnLine) {
      // VIRAJ CIKISI / EGIK KENAR TEMASI: bar cizgiyi (kenarda da olsa)
      // GORUYOR -> PIVOT YOK. Kisitli P duzeltmesiyle ILERI takip; temas
      // TRACK_MS kesintisiz surerse yukaridaki kapi kabul eder ve normal
      // PID'e (yumusatma penceresiyle) donulur. Robot kacmaz, geri donmez
      // (ic teker geri donmez: hamle baz hizla sinirlidir).
      const int base = velocityPWM - velPWMDecrement;
      const int steer = limitProvisionalSteer(
          (int)(((float)((int)position - LINE_CENTER_POSITION)) * KP), base);
      outputPWML = base + steer;
      outputPWMR = base - steer;

    } else {
      // VIRAJ CIKISI (cizgi tamamen gorunmuyor) — KOR VIRAJ TAKIBI:
      // Pivot / geri donus YOK. Kisa kopru + SONEN sinirli direksiyonla
      // ileri devam; pencere dolarsa guvenli durus (pist disina cikmaz,
      // yerinde donerek geriye gitmez).
      const int base = velocityPWM - velPWMDecrement;
      int guidedSteer = 0;

      if (offlineElapsedTime > OFFLINE_GAP_BRIDGE_MS &&
          offlineElapsedTime < OFFLINE_GUIDED_COAST_MS) {
        guidedSteer = steerSmooth;  // son gecerli (sonulmus) direksiyon yonu
        if (guidedSteer > OFFLINE_GUIDED_STEER_MAX) guidedSteer = OFFLINE_GUIDED_STEER_MAX;
        else if (guidedSteer < -OFFLINE_GUIDED_STEER_MAX) guidedSteer = -OFFLINE_GUIDED_STEER_MAX;
        guidedSteer = guidedSteer * (int)(OFFLINE_GUIDED_COAST_MS - offlineElapsedTime) /
                      (int)(OFFLINE_GUIDED_COAST_MS - OFFLINE_GAP_BRIDGE_MS);
      }

      outputPWML = base + guidedSteer;
      outputPWMR = base - guidedSteer;

      if (offlineElapsedTime >= OFFLINE_GUIDED_BUDGET_MS) {
        startBrake();
        outputPWML = brakeFromPwmL;
        outputPWMR = brakeFromPwmR;
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
