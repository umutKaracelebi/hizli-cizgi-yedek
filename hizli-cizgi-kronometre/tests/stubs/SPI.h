#ifndef TEST_SPI_H
#define TEST_SPI_H

// Masaustu testleri icin Arduino giris/cikis modeli. Uno derlemesinde kullanilmaz.
#include <stddef.h>
#include <stdint.h>

#define F(text) text
constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;
constexpr uint8_t INPUT_PULLUP = 2;
constexpr uint8_t OUTPUT = 1;
constexpr int FALLING = 2;

namespace TestHardware {
static uint32_t nowUs = 0;
static int sensorLevel = HIGH;
static int buttonLevel = HIGH;
static int sensorMode = -1;
static int buttonMode = -1;
static int interruptPin = -1;
static int interruptMode = -1;
static void (*handler)() = nullptr;
}

inline uint32_t micros() { return TestHardware::nowUs; }
inline uint32_t millis() { return TestHardware::nowUs / 1000UL; }
inline void noInterrupts() {}
inline void interrupts() {}
inline int digitalRead(uint8_t pin) {
  return pin == 2 ? TestHardware::sensorLevel : TestHardware::buttonLevel;
}
inline void pinMode(uint8_t pin, int mode) {
  if (pin == 2) TestHardware::sensorMode = mode;
  if (pin == 4) TestHardware::buttonMode = mode;
}
inline void digitalWrite(uint8_t, int) {}
inline int digitalPinToInterrupt(uint8_t pin) { return pin; }
inline void attachInterrupt(int pin, void (*handler)(), int mode) {
  TestHardware::interruptPin = pin;
  TestHardware::handler = handler;
  TestHardware::interruptMode = mode;
}

#endif
