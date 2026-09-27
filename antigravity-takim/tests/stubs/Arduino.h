#ifndef ATLAS_ANTIGRAVITY_TEST_ARDUINO_H
#define ATLAS_ANTIGRAVITY_TEST_ARDUINO_H

// Masaustu (native) testler icin Arduino + AVR kayit modeli.
//
// Bu dosya KARTA YUKLENEN derlemede kullanilmaz: antigravity-takim sketch
// klasorunun "tests/" alt klasoru Arduino derlemesine dahil edilmez
// (yalnizca sketch kok klasoru ve "src/" derlenir). Sadece g++/clang++
// ile yapilan masaustu kosularinda .ino dosyalarini sarmak icin vardir.
//
// Modelin amaci: kontrol dongusunu (updatePeriod) ve kopru/arama durum
// makinesini karta yukleme yapmadan adim adim kosturabilmek.

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t byte;
typedef bool boolean;

#ifndef LOW
#define LOW 0
#define HIGH 1
#endif
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define F(text) text

//  ---------------------------------------------------------------
//   A V R   K A Y I T   M O D E L I
//  ---------------------------------------------------------------
// Yalnizca firmware'in dokundugu alanlar. Turler hedefteki (ATmega328P)
// gercek kayitlarla birebir ayni olmak zorunda degil; test icin yeterli.

static volatile uint8_t ADMUX = 0;
static volatile uint8_t ADCSRA = 0;
static volatile uint8_t ADCH = 0;

static volatile uint8_t DDRB = 0;
static volatile uint8_t PORTB = 0;
static volatile uint8_t PINB = 0;

static volatile uint8_t DDRC = 0;
static volatile uint8_t PORTC = 0;
static volatile uint8_t PINC = 0;

static volatile uint8_t DDRD = 0;
static volatile uint8_t PORTD = 0;
static volatile uint8_t PIND = 0;

static volatile uint16_t TCCR1A = 0;
static volatile uint16_t TCCR1B = 0;
static volatile uint16_t ICR1 = 0;
static volatile uint16_t OCR1A = 0;
static volatile uint16_t OCR1B = 0;

static volatile uint16_t TCCR2A = 0;
static volatile uint16_t TCCR2B = 0;
static volatile uint16_t OCR2A = 0;

// bit numaralari (gercek AVR basliklariyla ayni)
#define DDD2 2
#define DDD3 3
#define DDD4 4
#define DDD5 5
#define DDD7 7
#define DDB0 0
#define DDB5 5
#define DDC5 5
#define PORTD2 2
#define PORTD3 3
#define PORTD4 4
#define PORTD7 7
#define PORTB0 0
#define PORTB5 5
#define PORTC5 5
#define PINC5 5
#define PIND3 3
#define PIND4 4
#define PIND7 7

#define REFS0 6
#define ADLAR 5
#define ADPS2 2
#define ADEN 7
#define ADSC 6

#define COM1A1 7
#define COM1B1 5
#define WGM11 1
#define WGM13 4
#define CS10 0
#define COM2A1 7
#define WGM20 0
#define CS20 0

#define _BV(bit) (1 << (bit))
#define cli()
#define sei()

//  ---------------------------------------------------------------
//   S I M U L E   D O N A N I M   D U R U M U
//  ---------------------------------------------------------------

namespace TestBoard {

static uint64_t nowUs = 0;                 // simule edilen zaman
static uint8_t adcRaw[16] = {0};           // MUX kanali -> ham ADC degeri
static bool motorDriverEnabled = false;    // INH (D12) seviyesi
static bool interruptsDisabled = false;    // cli()/sei() izleme

}  // namespace TestBoard

static inline void adcConvert() {
  const uint8_t channel = (uint8_t)(PORTC & 0x0F);
  ADCH = TestBoard::adcRaw[channel & 0x0F];
  ADCSRA = (uint8_t)(ADCSRA & ~_BV(ADSC));  // donusum tamamlandi
}

// ADC'nin "donusum bitti" kontrolu. readADC() fonksiyonu ADSC bitini
// yoklarken donusumu burada tamamliyoruz; boylece .ino kodu hic
// degistirilmeden gercek okuma akisi taklit edilir.
static inline bool bit_is_set_impl(volatile uint8_t &sfr, uint8_t bit) {
  if (bit == ADSC && (sfr & _BV(ADSC))) {
    adcConvert();
    return false;
  }
  return (sfr & _BV(bit)) != 0;
}
#define bit_is_set(sfr, bit) bit_is_set_impl(sfr, bit)

//  ---------------------------------------------------------------
//   A R D U I N O   A P I
//  ---------------------------------------------------------------

static inline void pinMode(uint8_t, uint8_t) {}
static inline int digitalRead(uint8_t) { return HIGH; }

static inline void digitalWrite(uint8_t pin, uint8_t value) {
  if (pin == 12) {  // INH: surucu uyandirma
    TestBoard::motorDriverEnabled = (value != 0);
  }
  // INL1/INR1 (yon) pinleri masaustu modelde onemsizdir: motor komutlari
  // outputPWML/outputPWMR globallerinden okunur.
}

static inline void delay(uint32_t ms) {
  TestBoard::nowUs += (uint64_t)ms * 1000ULL;
}
static inline void delayMicroseconds(uint32_t us) {
  TestBoard::nowUs += (uint64_t)us;
}
static inline uint32_t micros() { return (uint32_t)TestBoard::nowUs; }
static inline uint32_t millis() { return (uint32_t)(TestBoard::nowUs / 1000ULL); }

// Serial yalnizca derlenebilir olmali; testte cikti uretmez.
class TestSerial {
 public:
  void begin(uint32_t) {}
  void print(const char *) {}
  void print(char) {}
  void print(int) {}
  void print(unsigned int) {}
  void print(long) {}
  void print(unsigned long) {}
  void println() {}
  void println(const char *) {}
  void println(int) {}
};

static TestSerial Serial;

#endif
