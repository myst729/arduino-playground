#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_8x16matrix matrix1 = Adafruit_8x16matrix();
Adafruit_8x16matrix matrix2 = Adafruit_8x16matrix();

void setup() {
  Serial.begin(9600);
  Serial.println("16x8 LED Matrix Test");
  matrix1.begin(0x74);
  matrix2.begin(0x70);
  matrix1.setRotation(1);
  matrix2.setRotation(1);
}

static const uint8_t PROGMEM
  smile_bmp[] = {
    B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10100101,
    B10011001,
    B01000010,
    B00111100
  },
  neutral_bmp[] = {
    B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10111101,
    B10000001,
    B01000010,
    B00111100
  },
  frown_bmp[] = {
    B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10011001,
    B10100101,
    B01000010,
    B00111100
  };

void loop() {
  matrix1.clear();
  matrix1.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON);
  matrix1.writeDisplay();
  matrix2.clear();
  matrix2.drawBitmap(8, 0, frown_bmp, 8, 8, LED_ON);
  matrix2.writeDisplay();
  delay(500);

  matrix1.clear();
  matrix1.drawBitmap(4, 0, neutral_bmp, 8, 8, LED_ON);
  matrix1.writeDisplay();
  matrix2.clear();
  matrix2.drawBitmap(4, 0, neutral_bmp, 8, 8, LED_ON);
  matrix2.writeDisplay();
  delay(500);

  matrix1.clear();
  matrix1.drawBitmap(8, 0, frown_bmp, 8, 8, LED_ON);
  matrix1.writeDisplay();
  matrix2.clear();
  matrix2.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON);
  matrix2.writeDisplay();
  delay(500);

  matrix1.setTextSize(1);
  matrix1.setTextWrap(false);
  matrix1.setTextColor(LED_ON);
  matrix2.setTextSize(1);
  matrix2.setTextWrap(false);
  matrix2.setTextColor(LED_ON);

  for (int8_t x = 7; x >= -38; x--) {
    matrix1.clear();
    matrix1.setCursor(x, 0);
    matrix1.print("Hello         .");
    matrix1.writeDisplay();
    matrix2.clear();
    matrix2.setCursor(x, 0);
    matrix2.print("World        .");
    matrix2.writeDisplay();
    delay(100);
  }
}
