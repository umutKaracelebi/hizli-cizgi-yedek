#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include "../robot_kronometre/robot_kronometre.ino"

void sensorAt(uint32_t timestamp, int level) {
  TestHardware::nowUs = timestamp;
  const int previous = TestHardware::sensorLevel;
  TestHardware::sensorLevel = level;
  if (previous == HIGH && level == LOW && TestHardware::handler) {
    TestHardware::handler();
  }
  serviceSensor();
}

std::string fieldAt(int x, int y, int size, unsigned columns) {
  std::string text;
  for (unsigned i = 0; i < columns; ++i) {
    text += tft.cells.at({x + static_cast<int>(i) * 6 * size, y}).character;
  }
  return text;
}

int main() {
  char text[10];
  formatTime(0, text, sizeof(text));
  assert(std::strcmp(text, "00:00.000") == 0);
  formatTime(61234567, text, sizeof(text));
  assert(std::strcmp(text, "01:01.234") == 0);
  formatTime(TimingCore::MAX_RUN_US, text, sizeof(text));
  assert(std::strcmp(text, "60:00.000") == 0);

  setup();
  assert(TestHardware::sensorMode == INPUT_PULLUP);
  assert(TestHardware::buttonMode == INPUT_PULLUP);
  assert(TestHardware::interruptPin == SENSOR_PIN);
  assert(TestHardware::interruptMode == FALLING);
  assert(tft.clears == 1);

  // Baslangicta engel varken kurulmaz, sensor metni aktif-LOW'dur.
  sensorAt(0, LOW);
  sensorAt(500000, LOW);
  assert(!captureEnabled);
  assert(timer.state() == TimingCore::READY);
  drawDisplay();
  assert(fieldAt(10, 178, 2, 15) == "MZ80: ENGEL VAR");
  unsigned writes = tft.writes;
  drawDisplay();
  assert(tft.writes == writes); // Degismeyen ekran yeniden cizilmez.

  sensorAt(510000, HIGH);
  sensorAt(539999, HIGH);
  assert(!captureEnabled);
  sensorAt(540000, HIGH);
  assert(captureEnabled);
  drawDisplay();
  assert(fieldAt(10, 38, 2, 24) == "HAZIR - ilk gecisi bekle");
  assert(fieldAt(10, 178, 2, 15) == "MZ80: ENGEL YOK");

  sensorAt(600000, LOW);
  assert(timer.state() == TimingCore::RUNNING);
  drawDisplay();
  assert(fieldAt(10, 38, 2, 13) == "OLCUM SURUYOR");
  assert(fieldAt(10 + 13 * 12, 38, 2, 12) == std::string(12, ' '));
  assert(tft.cells.at({10, 38}).foreground == ILI9341_YELLOW);
  assert(tft.cells.at({10, 38}).background == ILI9341_BLACK);

  // Milisaniyenin tek rakami degisince yalniz o karakter cizilir.
  writes = tft.writes;
  TestHardware::nowUs = 601000;
  drawDisplay();
  assert(tft.writes == writes + 1);
  assert(fieldAt(10, 68, 4, 9) == "00:00.001");

  sensorAt(1200000, LOW); // Surekli engel ikinci gecis sayilmaz.
  assert(timer.state() == TimingCore::RUNNING);
  assert(!captureEnabled);
  sensorAt(1210000, HIGH);
  sensorAt(1239999, HIGH);
  assert(!captureEnabled);
  sensorAt(1240000, HIGH);
  assert(captureEnabled);
  sensorAt(1250000, LOW);
  assert(timer.state() == TimingCore::FINISHED);
  assert(timer.last() == 650000);
  drawDisplay();
  assert(fieldAt(106, 118, 2, 9) == "00:00.650");
  assert(fieldAt(106, 142, 2, 9) == "00:00.650");
  writes = tft.writes;
  TestHardware::nowUs = 2000000;
  drawDisplay();
  assert(tft.writes == writes); // Sonuc ekrani sabit.
  sensorAt(2100000, HIGH);
  sensorAt(2500000, LOW);
  assert(timer.last() == 650000); // Bitince yeni gecis yok sayilir.

  TestHardware::nowUs = 2600000;
  TestHardware::buttonLevel = LOW;
  serviceButton(millis());
  TestHardware::nowUs += 25000;
  serviceButton(millis());
  serviceSensor();
  assert(timer.state() == TimingCore::READY);
  assert(timer.hasLast() && timer.best() == 650000);
  drawDisplay();
  assert(fieldAt(10, 38, 2, 12) == "BUTONU BIRAK");
  assert(!captureEnabled);
  assert(tft.clears == 1); // Dongude buyuk alan silme yok.

  std::cout << "MZ80 sketch tests passed (simulated GPIO/LCD)\n";
}
