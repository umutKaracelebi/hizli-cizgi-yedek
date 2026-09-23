#ifndef TEST_ADAFRUIT_ILI9341_H
#define TEST_ADAFRUIT_ILI9341_H

#include <stdint.h>
#include <map>
#include <utility>

constexpr uint16_t ILI9341_BLACK = 0x0000;
constexpr uint16_t ILI9341_WHITE = 0xFFFF;
constexpr uint16_t ILI9341_CYAN = 0x07FF;
constexpr uint16_t ILI9341_GREEN = 0x07E0;
constexpr uint16_t ILI9341_YELLOW = 0xFFE0;
constexpr uint16_t ILI9341_RED = 0xF800;

// Fiziksel SPI/LCD'yi degil, yazilan karakterleri ve silme cagrilarini izler.
class Adafruit_ILI9341 {
 public:
  struct Cell {
    char character;
    uint16_t foreground;
    uint16_t background;
  };
  std::map<std::pair<int, int>, Cell> cells;
  unsigned writes = 0;
  unsigned clears = 0;

  Adafruit_ILI9341(uint8_t, uint8_t, uint8_t) {}
  void begin() {}
  void setRotation(uint8_t) {}
  void setTextWrap(bool) {}
  void fillScreen(uint16_t) { ++clears; cells.clear(); }
  void fillRect(int, int, int, int, uint16_t) { ++clears; }
  void setTextSize(uint8_t size) { size_ = size; }
  void setCursor(int x, int y) { x_ = x; y_ = y; }
  void setTextColor(uint16_t color) { foreground_ = color; }
  void setTextColor(uint16_t color, uint16_t background) {
    foreground_ = color;
    background_ = background;
  }
  void print(char character) {
    cells[{x_, y_}] = {character, foreground_, background_};
    ++writes;
    x_ += 6 * size_;
  }
  void print(const char* text) {
    while (*text) print(*text++);
  }

 private:
  int x_ = 0;
  int y_ = 0;
  uint8_t size_ = 1;
  uint16_t foreground_ = ILI9341_WHITE;
  uint16_t background_ = ILI9341_BLACK;
};

#endif
