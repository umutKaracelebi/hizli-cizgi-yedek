/*
  TraceMode.ino - module code for ATLAS series

    Created on: Mar, 2026
    Edited by Mauricio Tovar

    ATLAS 1.4.3 © 2026 by EXOTIC TEAM MX is licensed under Creative Commons
    Attribution-NonCommercial-NoDerivatives 4.0 International. To view a copy of
    this license, visit https://creativecommons.org/licenses/by-nc-nd/4.0

  This code is part of the ATLAS_1.4.3.ino original firmware.
*/

/*
  [ WARNING ] - debugModeNoSerial()

  USB-TTL adapter olmadan da kullanilabilen test/debug modu. Serial cikisi
  yerine olcumler ATmega328P'nin kendi EEPROM'unda (1024 byte) saklanir.
  Kayit, firmware'i yuklediginiz ayni programlayici ile (USBasp / USBtinyISP /
  USB-serial bootloader icin avrdude) geri okunur:

    avrdude -c usbasp -p m328p -U eeprom:r:atlas_log.bin:r
    powershell -File tools\dump_eeprom.ps1 -DecodeOnly atlas_log.bin

  Kullanim: karta guc verilirken SW1 veya SW2 basili tutulur (veya
  DEBUG_MODE_ALWAYS_ON = 1 yapilir), LED'ler ile secilen test calistirilir ve
  test bitiminde 3 LED 3 kez yanip soner. Sonra programlayici takilip log
  okunur.

  ================= EEPROM LOG LAYOUT (1024 byte) =================
    offset   0..3   magic  'A' 'T' 'L' 'S'
    offset   4      log version (EEPROM_LOG_VERSION)
    offset   5      mode     (EEPROM_LOG_MODE_*)
    offset   6      record count
    offset   7      record length (byte)
    offset   8      flags    (bit0: invertSensorReads)
    offset   9..10  sample period (ms, little endian)
    offset  11      reserved
    offset  12..    records  (record index = (offset - 12) / record length)
  =================================================================
*/

#include <avr/eeprom.h>

//  log header
#define EEPROM_LOG_SIZE_BYTES 1024
#define EEPROM_LOG_MAGIC_0 'A'
#define EEPROM_LOG_MAGIC_1 'T'
#define EEPROM_LOG_MAGIC_2 'L'
#define EEPROM_LOG_MAGIC_3 'S'
#define EEPROM_LOG_VERSION 1

#define EEPROM_LOG_OFF_MAGIC 0
#define EEPROM_LOG_OFF_VERSION 4
#define EEPROM_LOG_OFF_MODE 5
#define EEPROM_LOG_OFF_COUNT 6
#define EEPROM_LOG_OFF_RECORD_LENGTH 7
#define EEPROM_LOG_OFF_FLAGS 8
#define EEPROM_LOG_OFF_PERIOD_L 9
#define EEPROM_LOG_OFF_PERIOD_H 10
#define EEPROM_LOG_OFF_RESERVED 11
#define EEPROM_LOG_OFF_DATA 12

//  log modes
#define EEPROM_LOG_MODE_RAW_SENSORS 0
#define EEPROM_LOG_MODE_CALIBRATED_SENSORS 1
#define EEPROM_LOG_MODE_CALIBRATION 2
#define EEPROM_LOG_MODE_LINE_POSITION 3
#define EEPROM_LOG_MODE_IMPELLER 4

//  line position trace record: position low, position high, flags
#define EEPROM_LOG_POSITION_RECORD_LENGTH 3

//  impeller / power trace record: impeller PWM, ADC6, ADC7
#define EEPROM_LOG_POWER_RECORD_LENGTH 3

//  compile time check: secilen ornek sayilari EEPROM'a sigmali
#if (EEPROM_LOG_OFF_DATA + EEPROM_TRACE_SENSOR_SAMPLES * TOTAL_SENSORS) > EEPROM_LOG_SIZE_BYTES
#error "EEPROM_TRACE_SENSOR_SAMPLES cok buyuk: 12 + EEPROM_TRACE_SENSOR_SAMPLES * 16 <= 1024 olmali"
#endif
#if (EEPROM_LOG_OFF_DATA + EEPROM_TRACE_POSITION_SAMPLES * EEPROM_LOG_POSITION_RECORD_LENGTH) > EEPROM_LOG_SIZE_BYTES
#error "EEPROM_TRACE_POSITION_SAMPLES cok buyuk: 12 + EEPROM_TRACE_POSITION_SAMPLES * 3 <= 1024 olmali"
#endif
#if (EEPROM_LOG_OFF_DATA + EEPROM_TRACE_POWER_SAMPLES * EEPROM_LOG_POWER_RECORD_LENGTH) > EEPROM_LOG_SIZE_BYTES
#error "EEPROM_TRACE_POWER_SAMPLES cok buyuk: 12 + EEPROM_TRACE_POWER_SAMPLES * 3 <= 1024 olmali"
#endif

//  ========================
//  L O G   P R I M I T I V E S
//  ========================

void eepromLogWriteByte(unsigned int address, byte value) {
  eeprom_write_byte((uint8_t*)address, value);
}

void eepromLogWriteBytes(unsigned int address, const byte* data, unsigned int length) {
  for (unsigned int i = 0; i < length; i++) {
    eeprom_write_byte((uint8_t*)(address + i), data[i]);
  }
}

byte eepromLogReadByte(unsigned int address) {
  return eeprom_read_byte((const uint8_t*)address);
}

//  stored log gecerli mi (magic + dolu kayit sayisi)
bool eepromLogIsValid() {
  return eepromLogReadByte(EEPROM_LOG_OFF_MAGIC) == EEPROM_LOG_MAGIC_0 &&
         eepromLogReadByte(EEPROM_LOG_OFF_COUNT) > 0;
}

//  invalidates the stored log (magic byte cleared)
void eepromLogClear() {
  eepromLogWriteByte(EEPROM_LOG_OFF_MAGIC, 0);
  eepromLogWriteByte(EEPROM_LOG_OFF_COUNT, 0);
}

void eepromLogBegin(byte mode, byte recordLength, unsigned int samplePeriodMs) {
  eepromLogWriteByte(EEPROM_LOG_OFF_MAGIC, EEPROM_LOG_MAGIC_0);
  eepromLogWriteByte(EEPROM_LOG_OFF_MAGIC + 1, EEPROM_LOG_MAGIC_1);
  eepromLogWriteByte(EEPROM_LOG_OFF_MAGIC + 2, EEPROM_LOG_MAGIC_2);
  eepromLogWriteByte(EEPROM_LOG_OFF_MAGIC + 3, EEPROM_LOG_MAGIC_3);
  eepromLogWriteByte(EEPROM_LOG_OFF_VERSION, EEPROM_LOG_VERSION);
  eepromLogWriteByte(EEPROM_LOG_OFF_MODE, mode);
  eepromLogWriteByte(EEPROM_LOG_OFF_RECORD_LENGTH, recordLength);
  eepromLogWriteByte(EEPROM_LOG_OFF_FLAGS, invertSensorReads ? 1 : 0);
  eepromLogWriteByte(EEPROM_LOG_OFF_PERIOD_L, (byte)(samplePeriodMs & 0xFF));
  eepromLogWriteByte(EEPROM_LOG_OFF_PERIOD_H, (byte)(samplePeriodMs >> 8));
  eepromLogWriteByte(EEPROM_LOG_OFF_RESERVED, 0);
  eepromLogWriteByte(EEPROM_LOG_OFF_COUNT, 0);
}

void eepromLogStoreRecord(unsigned int index, const byte* record, byte recordLength) {
  eepromLogWriteBytes(EEPROM_LOG_OFF_DATA + index * recordLength, record, recordLength);
}

void eepromLogEnd(unsigned int recordCount) {
  // count is stored as a single byte, every mode fits inside the EEPROM anyway
  eepromLogWriteByte(EEPROM_LOG_OFF_COUNT, (byte)recordCount);
}

//  ==================
//  T R A C E S
//  ==================

//  raw or calibrated sensor values, TOTAL_SENSORS bytes per record
void traceSensorsToEEPROM(bool _calibrated) {
  eepromLogBegin(_calibrated ? EEPROM_LOG_MODE_CALIBRATED_SENSORS : EEPROM_LOG_MODE_RAW_SENSORS,
                 (byte)TOTAL_SENSORS, EEPROM_TRACE_SENSOR_PERIOD_MS);

  for (unsigned int i = 0; i < EEPROM_TRACE_SENSOR_SAMPLES; i++) {
    if (_calibrated) {
      readCalibratedSensors();
    } else {
      readRawSensors();
    }

    eepromLogStoreRecord(i, sensorValues, (byte)TOTAL_SENSORS);
    delay(EEPROM_TRACE_SENSOR_PERIOD_MS);
  }

  eepromLogEnd(EEPROM_TRACE_SENSOR_SAMPLES);
}

//  calibration snapshot: MAX + MIN + TH, 3 x TOTAL_SENSORS bytes (single record)
void traceCalibrationToEEPROM() {
  eepromLogBegin(EEPROM_LOG_MODE_CALIBRATION, (byte)(TOTAL_SENSORS * 3), 0);

  unsigned int address = EEPROM_LOG_OFF_DATA;

  eepromLogWriteBytes(address, maxSensorValues, TOTAL_SENSORS);
  address += TOTAL_SENSORS;

  eepromLogWriteBytes(address, minSensorValues, TOTAL_SENSORS);
  address += TOTAL_SENSORS;

  eepromLogWriteBytes(address, sensorThreshold, TOTAL_SENSORS);

  eepromLogEnd(1);
}

//  line position trace: position (2 byte) + flags (1 byte) per record
void traceLinePositionToEEPROM() {
  eepromLogBegin(EEPROM_LOG_MODE_LINE_POSITION, EEPROM_LOG_POSITION_RECORD_LENGTH, EEPROM_TRACE_POSITION_PERIOD_MS);

  byte record[EEPROM_LOG_POSITION_RECORD_LENGTH];

  for (unsigned int i = 0; i < EEPROM_TRACE_POSITION_SAMPLES; i++) {
    const unsigned int position = getLinePosition();  // also updates isOnLine

    record[0] = (byte)(position & 0xFF);
    record[1] = (byte)(position >> 8);
    record[2] = isOnLine ? 1 : 0;

    eepromLogStoreRecord(i, record, EEPROM_LOG_POSITION_RECORD_LENGTH);
    delay(EEPROM_TRACE_POSITION_PERIOD_MS);
  }

  eepromLogEnd(EEPROM_TRACE_POSITION_SAMPLES);
}

//  guc izi icin tek ornek yaz (tribun PWM, ADC6, ADC7)
//  donus: guncellenmis kayit indeksi (tampon dolduysa degismez)
unsigned int tracePowerSample(unsigned int index, int duty) {
  if (index < EEPROM_TRACE_POWER_SAMPLES) {
    readOtherADCs();  // ADC6_value ve ADC7_value guncellenir

    byte record[EEPROM_LOG_POWER_RECORD_LENGTH];
    record[0] = (byte)(duty < 0 ? 0 : (duty > 255 ? 255 : duty));
    record[1] = ADC6_value;
    record[2] = ADC7_value;

    eepromLogStoreRecord(index, record, EEPROM_LOG_POWER_RECORD_LENGTH);
    index++;

    // her 5 ornekte bir sayaci guncelle: guc kesilse bile kismi veri okunabilsin
    if ((index % 5) == 0) eepromLogEnd(index);
  }

  return index;
}

//  ==========================================================
//  B U T T O N   /   L E D   Y A R D I M C I L A R I
//  ==========================================================

void waitAnyButton() {
  while (!readButton_1() && !readButton_2())
    ;
}

void waitButtonRelease() {
  while (readButton_1() || readButton_2())
    ;
}

//  iki buton _holdMs boyunca birlikte basili tutulursa true (testten cikis)
bool bothButtonsHeldFor(unsigned long holdMs) {
  if (!(readButton_1() && readButton_2())) return false;

  const unsigned long startTime = millis();

  while (readButton_1() && readButton_2()) {
    if (millis() - startTime >= holdMs) return true;
  }

  return false;
}

//  end of a test: 3 confirmation flashes, then LED_2 stays on until a button is pressed
void traceDoneAnimation() {
  confirmAnimation(150, 3);
  setLED_2(1);
  waitAnyButton();
  waitButtonRelease();
  setLEDS(0);
}

//  ==============================================
//  T R I B U N   ( I M P E L L E R )   T E S T
//  ==============================================
//
//  LED dili:
//    LED_0 : uyari fazi - SW1 ile baslat (yavas blink), SW1+SW2 ile cikis
//    LED_1 : secili PWM seviyesi kadar blink (1..4), fan kapaninca 2 blink
//    LED_2 : impeller cikisi - rampa sirasinda hizli blink, hedef PWM'de sabit
//
//  Butonlar:
//    SW1       : PWM seviyesini degistir (1 -> 2 -> 3 -> 4 -> 1)
//    SW2       : fani ac / kapat
//    SW1 + SW2 : 500 ms basili tutulursa testten cikilir
//
//  Fan her acildiginda (SW2) EEPROM guc izi penceresi bastan baslar; iz
//  tribun PWM + ADC6 + ADC7 kaydeder (pil cokmesi / akim cekisi icin).

const int impellerTestPWMLevels[] = {
  IMPELLER_TEST_PWM_1, IMPELLER_TEST_PWM_2, IMPELLER_TEST_PWM_3, IMPELLER_TEST_PWM_4};
const byte impellerTestLevels = sizeof(impellerTestPWMLevels) / sizeof(impellerTestPWMLevels[0]);

//  secili seviyeyi LED_1 ile blink ederek goster
void impellerLevelLed(byte _level) {
  setLED_1(0);
  delay(200);

  for (byte i = 0; i < _level; i++) {
    setLED_1(1);
    delay(120);
    setLED_1(0);
    delay(180);
  }
}

//  fan kapandi: LED_2 sonuk, LED_1 iki kisa blink
void impellerOffLed() {
  setLED_2(0);
  setLED_1(0);

  for (byte i = 0; i < 2; i++) {
    setLED_1(1);
    delay(120);
    setLED_1(0);
    delay(180);
  }
}

void impellerTestNoSerial() {
  // NOT: debug modunda INH dusuk kaliyordu; tribun surucusu INH'e bagliysa
  // fan hic donmezdi. Tekerlek cikislari 0'da oldugu icin tekerler kipirdamaz.
  enableMotorDrivers();

  byte level = impellerTestLevels - 1;  // varsayilan seviye: tam guc
  bool isOn = false;
  bool blinkState = false;
  unsigned int traceIndex = 0;
  unsigned long rampStartTime = 0;
  unsigned long blinkTime = 0;

  //  uyari fazi: SW1 = basla, SW1 + SW2 = cikis
  setPWM_Impeller(0);
  setLED_2(0);

  while (1) {
    setLED_0(1);
    delay(250);
    setLED_0(0);
    delay(250);

    if (bothButtonsHeldFor(500)) {
      setLEDS(0);
      return;
    }

    if (readButton_1()) break;
  }

  waitButtonRelease();
  setLED_0(0);

  //  guc izi basligi hazirla (henuz kayit yok)
  eepromLogBegin(EEPROM_LOG_MODE_IMPELLER, EEPROM_LOG_POWER_RECORD_LENGTH, EEPROM_TRACE_POWER_PERIOD_MS);
  traceIndex = 0;

  confirmAnimation(150, 1);
  impellerLevelLed(level + 1);

  //  ana test dongusu (EEPROM_TRACE_POWER_PERIOD_MS tick)
  while (1) {
    const unsigned long now = millis();

    //  cikis: iki buton birlikte
    if (bothButtonsHeldFor(500)) {
      setPWM_Impeller(0);
      setLEDS(0);
      eepromLogEnd(traceIndex);
      confirmAnimation(150, 2);
      return;
    }

    //  PWM seviyesi degistir
    if (readButton_1()) {
      level = (level + 1 < impellerTestLevels) ? level + 1 : 0;
      impellerLevelLed(level + 1);
      rampStartTime = millis();  // yeni seviyeye tekrar rampala
      blinkTime = 0;
      waitButtonRelease();
    }

    //  fani ac / kapat
    if (readButton_2()) {
      isOn = !isOn;

      if (isOn) {
        rampStartTime = millis();
        blinkTime = 0;
        traceIndex = 0;  // yeni guc izi penceresi
        eepromLogBegin(EEPROM_LOG_MODE_IMPELLER, EEPROM_LOG_POWER_RECORD_LENGTH, EEPROM_TRACE_POWER_PERIOD_MS);
        impellerLevelLed(level + 1);
      } else {
        setPWM_Impeller(0);
        eepromLogEnd(traceIndex);
        impellerOffLed();
      }

      waitButtonRelease();
      continue;
    }

    //  hedef PWM'e tirmanma + LED_2 gostergesi
    int duty = 0;

    if (isOn) {
      const int targetPWM = impellerTestPWMLevels[level];
      const unsigned long elapsed = now - rampStartTime;

      duty = (elapsed >= IMPELLER_TEST_RAMP_TIME_MS)
               ? targetPWM
               : (int)((long)targetPWM * elapsed / IMPELLER_TEST_RAMP_TIME_MS);

      setPWM_Impeller(duty);

      if (duty < targetPWM) {
        // rampa: hizli blink
        if (now - blinkTime >= IMPELLER_TEST_BLINK_MS) {
          blinkState = !blinkState;
          setLED_2(blinkState);
          blinkTime = now;
        }
      } else {
        // hedef PWM: sabit yanar
        blinkState = true;
        setLED_2(1);
      }
    }

    //  guc izi ornegi
    traceIndex = tracePowerSample(traceIndex, duty);

    delay(EEPROM_TRACE_POWER_PERIOD_MS);
  }
}

//  =======================================
//  S E N S O R   T E S T   ( L E D )
//  =======================================
//
//  Once kalibrasyon yapilir (LED_2/LED_1/LED_0 siralari degisir), sonra:
//    LED_ler  : o anda cizgiyi goren sensor sayisi (0..7, 7+ doygun)
//    SW2 basili: cizginin konumu - 0..7 (0-1. sensor bolgesi ... 14-15. sensor bolgesi)
//    SW1 + SW2 : 500 ms basili tutulursa cikis
//
//  Tek sensor kontrolu: sensorun uzerine beyaz/siyah bir karti yaklastirip
//  LED'lerdeki sayinin artip azalmadigini izleyin.

void sensorTestNoSerial() {
  calibrateSensors();

  while (!bothButtonsHeldFor(500)) {
    readCalibratedSensors();

    byte onSensors = 0;

    for (byte i = 0; i < TOTAL_SENSORS; i++) {
      if (sensorValues[i]) onSensors++;
    }

    if (readButton_2()) {
      // konum modu: 0..7 -> (sensor cifti)
      const unsigned int position = getLinePosition();
      displayNumber((uint8_t)(position / 2000));
    } else {
      displayNumber(onSensors > 7 ? 7 : onSensors);
    }

    delay(50);
  }

  setLEDS(0);
}

//  ==============================
//  U I   T E S T   ( L E D )
//  ==============================
//
//    LED_0 : SW1 basili
//    LED_1 : SW2 basili
//    LED_2 : READY ve GO girisleri - ikisi de aktif: sabit yanar,
//            sadece birisi aktif: yavas blink, hicbiri aktif degil: sonuk
//    cikis : SW1 + SW2 500 ms birlikte basili

void uiTestNoSerial() {
  bool blinkState = false;

  while (!bothButtonsHeldFor(500)) {
    setLED_0(readButton_1());
    setLED_1(readButton_2());

    const bool ready = readReady();
    const bool go = readGo();

    if (ready && go) {
      setLED_2(1);
    } else if (ready || go) {
      setLED_2(blinkState);
      blinkState = !blinkState;
    } else {
      setLED_2(0);
    }

    delay(150);
  }

  setLEDS(0);
}

/*
  Serial-free version of debugMode() - PC'ye hicbir sey baglamadan, sadece
  buton + LED ile kullanilir.

    SW2  ...... islem numarasini artir (LED'ler sayiyi gosterir, displayNumber())
    SW1  ...... islemi onayla
    SW1+SW2 ... ic testlerden cikis (500 ms birlikte)

  Islem listesi:
    0 = WHEELS MOTOR TEST            (LED, motorTest())
    1 = TRIBUN / IMPELLER TEST       (LED + EEPROM guc izi)
    2 = SENSOR TEST                  (canli LED gosterge)
    3 = UI TEST                      (LED'ler SW1/SW2/READY/GO)
    4 = EEPROM: KALIBRASYON ANLIK    (MAX / MIN / TH)
    5 = EEPROM: HAM SENSOR IZI       (16 x 63 kayit)
    6 = EEPROM: KALIBRE SENSOR IZI   (16 x 63 kayit)
    7 = EEPROM: CIZGI KONUM IZI      (300 kayit)
*/
void debugModeNoSerial() {
  // okunmamis bir kayit varsa LED_0 kisa sure yanarak uyarir
  // (yeni bir test calistirilirsa bu kayit uzerine yazilir)
  if (eepromLogIsValid()) {
    setLED_0(1);
    delay(400);
    setLED_0(0);
  }

  eepromLogClear();  // invalidate an old log, so nothing stale can be read back

  while (1) {
    byte selection = DEBUG_MODE_START_OPERATION;
    displayNumber(selection);

    // islem secici: SW2 ilerletir, SW1 onaylar
    while (!readButton_1()) {
      if (readButton_2()) {
        selection = selection < 7 ? selection + 1 : 0;
        displayNumber(selection);
        delay(250);
      }
    }

    waitButtonRelease();
    confirmAnimation(250, 2);

    switch (selection) {
      case 0:  // WHEELS MOTOR TEST
        motorTest();
        break;

      case 1:  // TRIBUN / IMPELLER TEST
        impellerTestNoSerial();
        break;

      case 2:  // SENSOR TEST (canli LED)
        sensorTestNoSerial();
        break;

      case 3:  // UI TEST
        uiTestNoSerial();
        break;

      case 4:  // EEPROM: kalibrasyon anlik goruntusu
        calibrateSensors();
        traceCalibrationToEEPROM();
        traceDoneAnimation();
        break;

      case 5:  // EEPROM: ham sensor izi
        traceSensorsToEEPROM(false);
        traceDoneAnimation();
        break;

      case 6:  // EEPROM: kalibre sensor izi
        calibrateSensors();
        traceSensorsToEEPROM(true);
        traceDoneAnimation();
        break;

      case 7:  // EEPROM: cizgi konum izi
        calibrateSensors();
        traceLinePositionToEEPROM();
        traceDoneAnimation();
        break;

      default:
        break;
    }
  }
}
