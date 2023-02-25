#include <TM1640.h>
#include <TM16xxMatrix.h>

#define MATRIX_COLS 8
#define MATRIX_ROWS 8

// for ESP8266: DIN = 13(D7), CLK = 14(D5)
TM1640 module(13, 14);
// depending on my wiring of bicolor 8x8, col 0-7 are green (lime), col 8-15 are red
TM16xxMatrix matrix(&module, MATRIX_COLS * 2, MATRIX_ROWS);
byte r;

byte smile_bmp[] = {
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10100101,
  B10011001,
  B01000010,
  B00111100
};

byte neutral_bmp[] = {
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10000001,
  B10111101,
  B01000010,
  B00111100
};

byte frown_bmp[] = {
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10011001,
  B10100101,
  B01000010,
  B00111100
};

void setup() {
  module.clearDisplay();
}

void loop() {
  for (int i = 0; i < MATRIX_COLS; i++) {
    r = rand() % 8;
    matrix.setPixel(i, r, true);
    delay(80);
  }
  for (int ii = 0; ii < MATRIX_COLS; ii++) {
    matrix.setColumn(ii, smile_bmp[ii]);
  }
  delay(1000);
  matrix.setAll(false);

  for (int j = 0; j < MATRIX_COLS; j++) {
    r = rand() % 8;
    // offset for the red
    matrix.setPixel(j + MATRIX_COLS, r, true);
    delay(80);
  }
  for (int jj = 0; jj < MATRIX_COLS; jj++) {
    matrix.setColumn(jj + MATRIX_COLS, neutral_bmp[jj]);
  }
  delay(1000);
  matrix.setAll(false);

  for (int k = 0; k < MATRIX_COLS; k++) {
    r = rand() % 8;
    // red + lime = orange
    matrix.setPixel(k, r, true);
    matrix.setPixel(k + MATRIX_COLS, r, true);
    delay(80);
  }
  for (int kk = 0; kk < MATRIX_COLS; kk++) {
    matrix.setColumn(kk, frown_bmp[kk]);
    matrix.setColumn(kk + MATRIX_COLS, frown_bmp[kk]);
  }
  delay(1000);
  matrix.setAll(false);
}
