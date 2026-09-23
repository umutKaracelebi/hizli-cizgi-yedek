#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <stdio.h>
#include "TimingCore.h"

// Waveshare 2.4inch LCD Module (8 pin, ILI9341), Uno donanimsal SPI.
constexpr uint8_t TFT_CS = 10;
constexpr uint8_t TFT_DC = 7;
constexpr uint8_t TFT_RST = 8;
constexpr uint8_t TFT_BL = 9;
constexpr uint8_t SENSOR_PIN = 2;  // MZ80 OUT: engel varken LOW, yokken HIGH.
constexpr uint8_t BUTTON_PIN = 4;  // Normalde acik buton -> GND.
constexpr uint32_t CLEAR_US = 30000UL;  // Yeniden kurmak icin en az 30 ms engel yok.
constexpr uint32_t BUTTON_DEBOUNCE_MS = 25UL;
constexpr uint32_t DISPLAY_INTERVAL_MS = 100UL;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
TimingCore timer;

volatile bool captureEnabled = false;
volatile bool edgePending = false;
volatile uint32_t edgeAt = 0;

bool clearTracking = false;
uint32_t clearSince = 0;
bool rawButton = HIGH;
bool stableButton = HIGH;
uint32_t buttonChangedAt = 0;
uint32_t lastDisplayAt = 0;
bool displayDirty = true;

// ISR icinde ekran, Serial veya bekleme yok. Engel girisinde tek kenar yakalanir.
void onObjectDetected() {
  if (captureEnabled) {
    edgeAt = micros();
    edgePending = true;
    captureEnabled = false;
  }
}

void prepareMeasurement() {
  noInterrupts();
  captureEnabled = false;
  edgePending = false;
  interrupts();
  timer.reset(micros());
  clearTracking = false;
  displayDirty = true;
}

void serviceButton(uint32_t nowMs) {
  const bool reading = digitalRead(BUTTON_PIN);
  if (reading != rawButton) {
    rawButton = reading;
    buttonChangedAt = nowMs;
  }
  if (rawButton != stableButton &&
      uint32_t(nowMs - buttonChangedAt) >= BUTTON_DEBOUNCE_MS) {
    stableButton = rawButton;
    if (stableButton == LOW) prepareMeasurement();
  }
}

void serviceSensor() {
  uint32_t timestamp = 0;
  noInterrupts();
  const bool pending = edgePending;
  if (pending) {
    timestamp = edgeAt;
    edgePending = false;
  }
  interrupts();

  if (pending) {
    timer.crossing(timestamp);
    clearTracking = false;
    displayDirty = true;
  }

  const uint32_t now = micros();
  const TimingCore::State previousState = timer.state();
  timer.tick(now);
  if (timer.state() != previousState) displayDirty = true;

  if (timer.state() == TimingCore::FINISHED ||
      timer.state() == TimingCore::TIMED_OUT) {
    noInterrupts();
    captureEnabled = false;
    interrupts();
    return;
  }

  if (digitalRead(SENSOR_PIN) == LOW || rawButton == LOW || stableButton == LOW) {
    clearTracking = false;
    return;
  }
  if (!clearTracking) {
    clearSince = now;
    clearTracking = true;
  }
  if (uint32_t(now - clearSince) >= CLEAR_US && timer.canArm(now)) {
    // Son HIGH kontrolu ile kurma arasinda ISR calismasin.
    noInterrupts();
    if (!edgePending && digitalRead(SENSOR_PIN) == HIGH) {
      captureEnabled = true;
    }
    interrupts();
  }
}

void formatTime(uint32_t durationUs, char* text, size_t capacity) {
  const uint32_t totalMs = durationUs / 1000UL;
  snprintf(text, capacity, "%02lu:%02lu.%03lu",
           static_cast<unsigned long>(totalMs / 60000UL),
           static_cast<unsigned long>((totalMs / 1000UL) % 60UL),
           static_cast<unsigned long>(totalMs % 1000UL));
}

// Varsayilan sabit genislikli fontta yalniz degisen karakterleri ciz.
// Siyah arka plan eski karakteri kapatir; genis alanlari silip cizmek yok.
// Kisa metne geciste kalan karakterleri boslukla temizle.
void drawTextField(const char* text, char* previous, uint8_t columns,
                   int16_t x, int16_t y, uint8_t size, uint16_t color,
                   bool force) {
  tft.setTextSize(size);
  tft.setTextColor(color, ILI9341_BLACK);
  bool ended = false;
  for (uint8_t i = 0; i < columns; ++i) {
    if (!ended && text[i] == '\0') ended = true;
    const char character = ended ? ' ' : text[i];
    if (force || previous[i] != character) {
      tft.setCursor(x + i * 6 * size, y);
      tft.print(character);
      previous[i] = character;
    }
  }
}

void drawDisplay() {
  // Yaklasik 80 bayt karakter onbellegi; tam ekran tamponu yok.
  static char oldStatus[25] = {};
  static char oldTime[9] = {};
  static char oldLast[9] = {};
  static char oldBest[9] = {};
  static char oldSensor[25] = {};
  static uint16_t oldStatusColor = ILI9341_BLACK;
  static uint16_t oldSensorColor = ILI9341_BLACK;
  const bool armed = captureEnabled;
  const char* status = "";
  uint16_t statusColor = ILI9341_YELLOW;
  switch (timer.state()) {
    case TimingCore::READY:
      statusColor = armed ? ILI9341_GREEN : ILI9341_YELLOW;
      if (rawButton == LOW || stableButton == LOW) status = "BUTONU BIRAK";
      else status = armed ? "HAZIR - ilk gecisi bekle" : "ALANI BOSALT / BEKLE";
      break;
    case TimingCore::RUNNING:
      status = "OLCUM SURUYOR";
      break;
    case TimingCore::FINISHED:
      statusColor = ILI9341_GREEN;
      status = "TAMAMLANDI";
      break;
    case TimingCore::TIMED_OUT:
      statusColor = ILI9341_RED;
      status = "60 DK SINIRI ASILDI";
      break;
  }
  drawTextField(status, oldStatus, sizeof(oldStatus), 10, 38, 2,
                statusColor, statusColor != oldStatusColor);
  oldStatusColor = statusColor;

  char timeText[10];  // mm:ss.mmm + sonlandirici; olcum en fazla 60 dakika.
  formatTime(timer.elapsed(micros()), timeText, sizeof(timeText));
  drawTextField(timeText, oldTime, sizeof(oldTime), 10, 68, 4,
                ILI9341_WHITE, false);
  if (timer.hasLast()) formatTime(timer.last(), timeText, sizeof(timeText));
  drawTextField(timer.hasLast() ? timeText : "--:--.---", oldLast,
                sizeof(oldLast), 106, 118, 2, ILI9341_WHITE, false);
  if (timer.hasLast()) formatTime(timer.best(), timeText, sizeof(timeText));
  drawTextField(timer.hasLast() ? timeText : "--:--.---", oldBest,
                sizeof(oldBest), 106, 142, 2, ILI9341_WHITE, false);

  const bool objectDetected = digitalRead(SENSOR_PIN) == LOW;
  const uint16_t sensorColor = objectDetected ? ILI9341_RED : ILI9341_GREEN;
  drawTextField(objectDetected ? "MZ80: ENGEL VAR" : "MZ80: ENGEL YOK",
                oldSensor, sizeof(oldSensor), 10, 178, 2, sensorColor,
                sensorColor != oldSensorColor);
  oldSensorColor = sensorColor;
}

void setup() {
  // 5 V aktif-LOW MZ80 cikisini bosta HIGH tut. Sensoru GPIO'dan besleme.
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);
  tft.begin();
  tft.setRotation(1);
  tft.setTextWrap(false);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print(F("HIZLI CIZGI KRONOMETRE"));
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 118);
  tft.print(F("Son:"));
  tft.setCursor(10, 142);
  tft.print(F("En iyi:"));
  tft.setTextSize(1);
  tft.setCursor(10, 210);
  tft.print(F("Buton: yeni olcum / calisan olcumu iptal"));
  tft.setCursor(10, 225);
  tft.print(F("Kayitlar guc kapaninca silinir."));
  digitalWrite(TFT_BL, HIGH);
  prepareMeasurement();
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), onObjectDetected, FALLING);
}

void loop() {
  const uint32_t nowMs = millis();
  serviceButton(nowMs);
  serviceSensor();
  if (displayDirty || uint32_t(nowMs - lastDisplayAt) >= DISPLAY_INTERVAL_MS) {
    displayDirty = false;
    lastDisplayAt = nowMs;
    drawDisplay();
  }
}
