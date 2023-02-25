#include <Adafruit_NeoPixel.h>

#define BUTTON_PIN 2
#define PIXEL_PIN 9
#define PIXEL_NUM 36
#define SNAKE_LENGTH 5

Adafruit_NeoPixel neon = Adafruit_NeoPixel(PIXEL_NUM, PIXEL_PIN, NEO_RGB + NEO_KHZ800);

/**
 * ┌────┬────┬────┬────┬────┬────┐
 * │  5 │  6 │ 17 │ 18 │ 29 │ 30 │
 * ├────┼────┼────┼────┼────┼────┤
 * │  4 │  7 │ 16 │ 19 │ 28 │ 31 │
 * ├────┼────┼────┼────┼────┼────┤
 * │  3 │  8 │ 15 │ 20 │ 27 │ 32 │
 * ├────┼────┼────┼────┼────┼────┤
 * │  2 │  9 │ 14 │ 21 │ 26 │ 33 │
 * ├────┼────┼────┼────┼────┼────┤
 * │  1 │ 10 │ 13 │ 22 │ 25 │ 34 │
 * ├────┼────┼────┼────┼────┼────┤
 * │  0 │ 11 │ 12 │ 23 │ 24 │ 35 │
 * └────┴────┴────┴────┴────┴────┘
 */

int orders[PIXEL_NUM] = { 0, 1, 2, 3, 4, 5, 6, 17, 18, 29, 30, 31, 32, 33, 34, 35, 24, 23, 12, 11, 10, 9, 8, 7, 16, 19, 28, 27, 26, 25, 22, 13, 14, 15, 20, 21 };
bool state = HIGH;
uint16_t wheel = 0;
uint32_t black = neon.ColorHSV(0, 0, 0);
uint32_t colors[SNAKE_LENGTH];

void setup() {
  wheel = millis();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  neon.begin();
}

void loop() {
  bool newState = digitalRead(BUTTON_PIN);
  if (newState == LOW && state == HIGH) {
    delay(25);
    newState = digitalRead(BUTTON_PIN);
    if (newState == LOW) {
      on(25);
    }
  }
  state = newState;
}

void on(uint8_t interval) {
  wheel = (wheel + 6000) % 65535;
  for (int i = 0; i < SNAKE_LENGTH; i++) {
    colors[i] = neon.ColorHSV(wheel, 255, 32 * (SNAKE_LENGTH - 1 - i));
  }
  snake(interval);
}

void off(uint8_t wait) {
  for (uint16_t i = 0; i < PIXEL_NUM; i++) {
    neon.setPixelColor(i, black);
  }
  neon.show();
  delay(wait);
}

void snake(uint8_t interval) {
  off(5);
  for (uint16_t i = 0; i < PIXEL_NUM + SNAKE_LENGTH; i++) {
    for (int j = 0; j < SNAKE_LENGTH; j++) {
      if (i >= j && i - j < PIXEL_NUM) {
        neon.setPixelColor(orders[i - j], colors[j]);
      }
    }
    neon.show();
    delay(interval);
  }
}
