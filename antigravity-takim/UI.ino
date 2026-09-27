/*
  UI.ino - module code for ATLAS series / Antigravity Version
*/

void UIInit() {
  // LED Çıkışları
  DDRD |= 1 << DDD2;     // set PD2 as LED_0 digital output
  DDRB |= 1 << DDB0;     // set PB0 as LED_1 digital output
  DDRB |= 1 << DDB5;     // set PB5 as LED_2 digital output

  // Buton Girişleri (Pull-up ile)
  DDRC &= ~(1 << DDC5);  // set PC5 as input (button_1)
  PORTC |= 1 << PORTC5;  // enable PC5 pull-up

  DDRD &= ~(1 << DDD7);  // set PD7 as input (button_2)
  PORTD |= 1 << PORTD7;  // enable PD7 pull-up

  // MEBSTART Sinyal Girişleri (Pull-up ile)
  DDRD &= ~(1 << DDD3);  // set PD3 as input (READY)
  PORTD |= 1 << PORTD3;  // enable PD3 pull-up

  DDRD &= ~(1 << DDD4);  // set PD4 as input (GO / MEBSTART sinyali)
  PORTD |= 1 << PORTD4;  // enable PD4 pull-up
}

bool readButton_1() {
  return !(PINC & (1 << PINC5));
}

bool readButton_2() {
  return !(PIND & (1 << PIND7));
}

bool readReady() {
  return PIND & (1 << PIND3);
}

bool readGo() {
  return PIND & (1 << PIND4);
}

// MEBSTART Durum Fonksiyonları:
// Beklemede / STOP anında modül 5V (HIGH) verir.
// START verildiğinde modül 0V (LOW) verir.
bool readStartIdle() {
  return (PIND & (1 << PIND4)) != 0;  // HIGH (5V) = Bekleme / STOP
}

bool startSignalLow() {
  return !(PIND & (1 << PIND4));      // LOW (0V) = Aktif START
}

void setLED_0(bool _state) {
  PORTD = (PORTD & ~(1 << PORTD2)) | (_state << PORTD2);
}

void setLED_1(bool _state) {
  PORTB = (PORTB & ~(1 << PORTB0)) | (_state << PORTB0);
}

void setLED_2(bool _state) {
  PORTB = (PORTB & ~(1 << PORTB5)) | (_state << PORTB5);
}

void setLEDS(bool _state) {
  PORTD = (PORTD & ~(1 << PORTD2)) | (_state << PORTD2);
  PORTB = (PORTB & ~(1 << PORTB0)) | (_state << PORTB0);
  PORTB = (PORTB & ~(1 << PORTB5)) | (_state << PORTB5);
}

void displayNumber(uint8_t number) {
  uint8_t config = number > 7 ? 0 : number;

  setLED_0(config & 0b100);
  setLED_2(config & 0b10);
  setLED_1(config & 0b1);
}

void toggleLED_2() {
  PINB |= 1 << PORTB5;
}

void confirmAnimation(int _time, byte _cycles) {
  for (byte i = 0; i < _cycles; i++) {
    setLEDS(1);
    delay(_time);
    setLEDS(0);
    delay(_time);
  }
}

void bootAnimation() {
  for (byte i = 0; i < 20; i++) {
    toggleLED_2();
    delay(25);
  }
}
