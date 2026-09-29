#ifndef ATLAS_ANTIGRAVITY_TEST_PROTOTYPES_H
#define ATLAS_ANTIGRAVITY_TEST_PROTOTYPES_H

// Arduino derleyicisi (arduino-cli/IDE) .ino dosyalarini birlestirirken
// otomatik fonksiyon prototipleri uretir. Masaustu (native) derlemede bu
// uretim olmadigi icin ayni bildirimleri burada elle yaziyoruz.
//
// Onemli: bir .ino fonksiyonunun imzasi degisirse burasi da guncellenmeli.

// --- UI.ino ---
void UIInit();
bool readButton_1();
bool readButton_2();
bool readReady();
bool readGo();
bool readStartIdle();
bool startSignalLow();
void setLED_0(bool _state);
void setLED_1(bool _state);
void setLED_2(bool _state);
void setLEDS(bool _state);
void displayNumber(uint8_t number);
void toggleLED_2();
void confirmAnimation(int _time, byte _cycles);
void bootAnimation();

// --- Motors.ino ---
void motorsInit();
void enableMotorDrivers();
void disableMotorDrivers();
void setPWM_Impeller(int val);
void setPWM_MotorL(int val);
void setPWM_MotorR(int val);
void motorTest();

// --- Sensors.ino ---
void sensorsInit();
void setADCChannel(byte channel);
byte readADC();
void readRawSensors();
void readSampledSensors();
void updateMaxMinSensorValues();
void resetCalibrationValues();
void calibrateSensors();
void getSensorThreshold();
bool calibrationValid();
void readCalibratedSensors();
unsigned int getLinePosition();
void printCalibrationValues();
void readOtherADCs();
void printSensorValues();

// --- RunControl.ino ---
void runInit();
void updatePWMDecrementRamps();
bool preVacuum();
void runBegin();
void run();
void updatePeriod();
void disableRobot();
void impellerRamp();

#endif
