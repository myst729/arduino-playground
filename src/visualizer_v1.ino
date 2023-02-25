/*
  Connections:
  - Mic GAIN:
       * VCC for 40dB
       * GND for 50dB
       * floating for 60dB
  - GP26 to mic amp output
  - 3V3, GND, SDA (GP4) and SCL (GP5) to I2C SSD1306
*/

#include <arduinoFFT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Mic
#define FFT_ANALOG_PIN 26 // RP2040 GP26
#define FFT_SAMPLES 1024 // power of 2
#define FFT_SAMPLING_FREQ 30000 // 15 kHz Fmax = sampleF /2 
#define FFT_AMPLITUDE 100 // sensitivity
#define FFT_FREQUENCY_BANDS 14

// OLED
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_SCREEN_WIDTH 128
#define OLED_SCREEN_HEIGHT 32
#define OLED_BAR_WIDTH 11
#define OLED_BARS 11

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET);

// adjust reference to get remove background noise
float reference = log10(50.0);
unsigned long samplingPeriod;
double cutoffFrequencies[FFT_FREQUENCY_BANDS];
double vReal[FFT_SAMPLES];
double vImag[FFT_SAMPLES];
arduinoFFT fft = arduinoFFT(vReal, vImag, FFT_SAMPLES, FFT_SAMPLING_FREQ);

void setup() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    for (;;); // Don't proceed, loop forever
  }

  // Setup display
  display.clearDisplay();
  display.display();
  display.setRotation(0);
  display.invertDisplay(false);

  samplingPeriod = (1.0 / FFT_SAMPLING_FREQ ) * pow(10.0, 6);

  // Calculate cutoff frequencies, make a logarithmic scale base
  double basePot = pow(FFT_SAMPLING_FREQ / 2.0, 1.0 / FFT_FREQUENCY_BANDS);
  for (int i = 0; i < FFT_FREQUENCY_BANDS; i++) {
    cutoffFrequencies[i] = pow(basePot, i + 1);
  }

  // draw dashed lines to sperate frequency bands
  for (int i = 0; i < OLED_BARS - 1 ; i++) {
    for (int j = 0; j < OLED_SCREEN_HEIGHT ; j += 4) {
      display.writePixel((i + 1) * OLED_BAR_WIDTH + 2 , j, SSD1306_WHITE);
    }
  }
  display.drawRect(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, SSD1306_WHITE);
}

int oldHeight[20];
int oldMax[20];

void loop() {
  // take samples
  for (int i = 0; i < FFT_SAMPLES; i++) {
    unsigned long currentTime = micros();
    int value = analogRead(FFT_ANALOG_PIN);
    vReal[i] = value;
    vImag[i] = 0;
    while (micros() < (currentTime + samplingPeriod)) {
      yield();
    }
  }

  // compute FFT
  fft.DCRemoval();
  fft.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  fft.Compute(FFT_FORWARD);
  fft.ComplexToMagnitude();

  double median[20];
  double max[20];
  int index = 0;
  double hzPerSample = (1.0 * FFT_SAMPLING_FREQ) / FFT_SAMPLES;
  double hz = 0;
  double maxinband = 0;
  double sum = 0;
  int count = 0;
  for (int i = 2; i < (FFT_SAMPLES / 2) ; i++) {
    count++;
    sum += vReal[i];
    if (vReal[i] > max[index]) {
      max[index] = vReal[i];
    }
    if (hz > cutoffFrequencies[index]) {
      median[index] = sum / count;
      sum = 0.0;
      count = 0;
      index++;
      max[index] = 0;
      median[index] = 0;
    }
    hz += hzPerSample;
  }

  // calculate median and maximum per frequency band
  if (sum > 0.0) {
    median[index] = sum / count;
    if (median[index] > maxinband) {
      maxinband = median[index];
    }
  }

  int bar = 0;

  for (int i = FFT_FREQUENCY_BANDS - 1; i > 2; i--) {
    int newHeight = 0;
    int newMax = 0;
    // calculate actual decibels
    if (median[i] > 0 && max[i] > 0) {
      newHeight = 20.0 * (log10(median[i]) - reference);
      newMax = 20.0 * (log10(max[i]) - reference);
    }

    // adjust minimum and maximum levels
    if (newHeight < 0 || newMax < 0) {
      newHeight = 1;
      newMax = 1;
    }
    if (newHeight >= OLED_SCREEN_HEIGHT - 2) {
      newHeight = OLED_SCREEN_HEIGHT - 3;
    }
    if (newMax >= OLED_SCREEN_HEIGHT - 2) {
      newMax = OLED_SCREEN_HEIGHT - 3;
    }

    int barX = bar * OLED_BAR_WIDTH + 5;
    // remove old level median
    if (oldHeight[i] > newHeight) {
      display.fillRect(barX, newHeight + 1, 7, oldHeight[i], SSD1306_BLACK);
    }
    // remove old max level
    if (oldMax[i] > newHeight) {
      for (int j = oldMax[i]; j > newHeight; j -= 2) {
        display.drawFastHLine(barX, j, 7, SSD1306_BLACK);
      }
    }
    // paint new max level
    for (int j = newMax; j > newHeight; j -= 2) {
      display.drawFastHLine(barX, j, 7, SSD1306_WHITE);
    }
    // paint new level median
    display.fillRect(barX, 1, 7, newHeight, SSD1306_WHITE);

    oldMax[i] = newMax;
    oldHeight[i] = newHeight;
    bar++;
  }
  display.drawFastHLine(0, OLED_SCREEN_HEIGHT - 1, OLED_SCREEN_WIDTH, SSD1306_WHITE);
  display.display();
}
