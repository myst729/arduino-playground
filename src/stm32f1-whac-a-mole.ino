#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

#define COLOR_RED   0xFF0000
#define COLOR_GREEN 0x00FF00
#define COLOR_BLUE  0x0000FF

const int NUM_ROWS = 6;
const int NUM_COLS = 5;
const int NUM_KEYS = NUM_ROWS * NUM_COLS;
const int PIN_ROWS[NUM_ROWS] = { PB15, PA5, PA3, PA15, PB7, PB8 };
const int PIN_COLS[NUM_COLS] = { PB13, PB14, PB3, PA4, PA6 };
const int PIN_WS2812 = PB12;
const int WS2812_MAP[NUM_KEYS] = { 0, 1, 2, 3, 4, 9, 8, 7, 6, 5, 10, 11, 12, 13, 14, 29, 28, 27, 26, 25, 20, 21, 22, 23, 24, 19, 18, 17, 16, 15 };

int target_index = -1;
bool target_hit = false;

Adafruit_NeoPixel pixels(NUM_KEYS, PIN_WS2812, NEO_GRB + NEO_KHZ800);

void update_ws2812 (uint32_t color) {
  pixels.clear();
  pixels.setPixelColor(WS2812_MAP[target_index], color);
  pixels.show();
}

void update_index () {
  srand((unsigned int)time(NULL));
  target_index = rand() % NUM_KEYS;
  update_ws2812(COLOR_BLUE);
}

int get_key_index() {
  int key_index = -1;

  for (int r = 0; r < NUM_ROWS; r++) {
    pinMode(PIN_ROWS[r], INPUT);
  }

  for (int row = 0; row < NUM_ROWS; row++) {
    pinMode(PIN_ROWS[row], OUTPUT);
    digitalWrite(PIN_ROWS[row], LOW);

    for (int c = 0; c < NUM_COLS; c++) {
      pinMode(PIN_COLS[c], INPUT_PULLUP);
    }

    delay(5);

    for (int col = 0; col < NUM_COLS; col++) {
      if (digitalRead(PIN_COLS[col]) == LOW) {
        key_index = row * NUM_COLS + col;
      }
    }

    pinMode(PIN_ROWS[row], INPUT);
  }

  return key_index;
}

void setup() {
  pixels.begin();
  update_index();
  delay(50);
}

void loop() {
  int key = get_key_index();

  if (key == -1) {
    // keyup
    if (target_hit == true) {
      target_hit = false;
      update_index();
    } else {
      update_ws2812(COLOR_BLUE);
    }
  } else {
    // keydown
    if (key == target_index) {
      target_hit = true;
      update_ws2812(COLOR_GREEN);
    } else {
      update_ws2812(COLOR_RED);
    }
  }
  delay(50);
}
