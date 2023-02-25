/*
  Connections:
  - Mic GAIN:
       * VCC for 40dB
       * GND for 50dB
       * floating for 60dB
  - GP26 to mic amp output
  - 3V3, GND, SDA (GP4) and SCL (GP5) to I2C SSD1306
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define FFT_LIB_REV 0x14
#define FFT_FORWARD 0x01
#define FFT_REVERSE 0x00

/* Windowing type */
#define FFT_WIN_TYP_RECTANGLE 0x00 /* rectangle (Box car) */
#define FFT_WIN_TYP_HAMMING 0x01 /* hamming */
#define FFT_WIN_TYP_HANN 0x02 /* hann */
#define FFT_WIN_TYP_TRIANGLE 0x03 /* triangle (Bartlett) */
#define FFT_WIN_TYP_NUTTALL 0x04 /* nuttall */
#define FFT_WIN_TYP_BLACKMAN 0x05 /* blackman */
#define FFT_WIN_TYP_BLACKMAN_NUTTALL 0x06 /* blackman nuttall */
#define FFT_WIN_TYP_BLACKMAN_HARRIS 0x07 /* blackman harris*/
#define FFT_WIN_TYP_FLT_TOP 0x08 /* flat top */
#define FFT_WIN_TYP_WELCH 0x09 /* welch */

/* Mathematial constants */
#define TWO_PI 6.28318531
#define FOUR_PI 12.56637061
#define SIX_PI 18.84955593

uint8_t fft_revision(void) {
  return(FFT_LIB_REV);
}

uint8_t fft_exponent(uint16_t value) {
  // Calculates the base 2 logarithm of a value
  uint8_t result = 0;
  while (((value >> result) & 1) != 1) result++;
  return(result);
}

void fft_parabola(double x1, double y1, double x2, double y2, double x3, double y3, double *a, double *b, double *c) {
  double reversed_denom = 1 / ((x1 - x2) * (x1 - x3) * (x2 - x3));
  *a = (x3 * (y2 - y1) + x2 * (y1 - y3) + x1 * (y3 - y2)) * reversed_denom;
  *b = (x3 * x3 * (y1 - y2) + x2 * x2 * (y3 - y1) + x1*x1 * (y2 - y3)) * reversed_denom;
  *c = (x2 * x3 * (x2 - x3) * y1 + x3 * x1 * (x3 - x1) * y2 + x1 * x2 * (x1 - x2) * y3) * reversed_denom;
}

void fft_swap(double *x, double *y) {
  double temp = *x;
  *x = *y;
  *y = temp;
}

void fft_compute(double *vReal, double *vImag, uint16_t samples, uint8_t power, uint8_t dir) {
  // Computes in-place complex-to-complex FFT
  // Reverse bits
  uint16_t j = 0;
  for (uint16_t i = 0; i < (samples - 1); i++) {
    if (i < j) {
      fft_swap(&vReal[i], &vReal[j]);
      if(dir == FFT_REVERSE)
        fft_swap(&vImag[i], &vImag[j]);
    }
    uint16_t k = (samples >> 1);
    while (k <= j) {
      j -= k;
      k >>= 1;
    }
    j += k;
  }
  // Compute the FFT
  double c1 = -1.0;
  double c2 = 0.0;
  uint16_t l2 = 1;
  for (uint8_t l = 0; (l < power); l++) {
    uint16_t l1 = l2;
    l2 <<= 1;
    double u1 = 1.0;
    double u2 = 0.0;
    for (j = 0; j < l1; j++) {
       for (uint16_t i = j; i < samples; i += l2) {
          uint16_t i1 = i + l1;
          double t1 = u1 * vReal[i1] - u2 * vImag[i1];
          double t2 = u1 * vImag[i1] + u2 * vReal[i1];
          vReal[i1] = vReal[i] - t1;
          vImag[i1] = vImag[i] - t2;
          vReal[i] += t1;
          vImag[i] += t2;
       }
       double z = ((u1 * c1) - (u2 * c2));
       u2 = ((u1 * c2) + (u2 * c1));
       u1 = z;
    }
    c2 = sqrt((1.0 - c1) / 2.0);
    c1 = sqrt((1.0 + c1) / 2.0);
    if (dir == FFT_FORWARD) {
      c2 = -c2;
    }
  }
  // Scaling for reverse transform
  if (dir != FFT_FORWARD) {
    for (uint16_t i = 0; i < samples; i++) {
       vReal[i] /= samples;
       vImag[i] /= samples;
    }
  }
}

void fft_complex_to_magnitude(double *vReal, double *vImag, uint16_t samples) {
  // vM is half the size of vReal and vImag
  for (uint16_t i = 0; i < samples; i++) {
    vReal[i] = sqrt(pow(vReal[i], 2) + pow(vImag[i], 2));
  }
}

void fft_dc_removal(double *vData, uint16_t samples) {
  // calculate the mean of vData
  double mean = 0;
  for (uint16_t i = 0; i < samples; i++) {
    mean += vData[i];
  }
  mean /= samples;
  // Subtract the mean from vData
  for (uint16_t i = 0; i < samples; i++) {
    vData[i] -= mean;
  }
}

void fft_windowing(double *vData, uint16_t samples, uint8_t windowType, uint8_t dir) {
  // Weighing factors are computed once before multiple use of FFT
  // The weighing function is symmetric; half the weighs are recorded
  double samplesMinusOne = (double)samples - 1.0;
  for (uint16_t i = 0; i < (samples >> 1); i++) {
    double indexMinusOne = (double)i;
    double ratio = (indexMinusOne / samplesMinusOne);
    double weighingFactor = 1.0;
    // Compute and record weighting factor
    switch (windowType) {
    case FFT_WIN_TYP_RECTANGLE: // rectangle (box car)
      weighingFactor = 1.0;
      break;
    case FFT_WIN_TYP_HAMMING: // hamming
      weighingFactor = 0.54 - (0.46 * cos(TWO_PI * ratio));
      break;
    case FFT_WIN_TYP_HANN: // hann
      weighingFactor = 0.54 * (1.0 - cos(TWO_PI * ratio));
      break;
    case FFT_WIN_TYP_TRIANGLE: // triangle (Bartlett)
      weighingFactor = 1.0 - ((2.0 * abs(indexMinusOne - (samplesMinusOne / 2.0))) / samplesMinusOne);
      break;
    case FFT_WIN_TYP_NUTTALL: // nuttall
      weighingFactor = 0.355768 - (0.487396 * (cos(TWO_PI * ratio))) + (0.144232 * (cos(FOUR_PI * ratio))) - (0.012604 * (cos(SIX_PI * ratio)));
      break;
    case FFT_WIN_TYP_BLACKMAN: // blackman
      weighingFactor = 0.42323 - (0.49755 * (cos(TWO_PI * ratio))) + (0.07922 * (cos(FOUR_PI * ratio)));
      break;
    case FFT_WIN_TYP_BLACKMAN_NUTTALL: // blackman nuttall
      weighingFactor = 0.3635819 - (0.4891775 * (cos(TWO_PI * ratio))) + (0.1365995 * (cos(FOUR_PI * ratio))) - (0.0106411 * (cos(SIX_PI * ratio)));
      break;
    case FFT_WIN_TYP_BLACKMAN_HARRIS: // blackman harris
      weighingFactor = 0.35875 - (0.48829 * (cos(TWO_PI * ratio))) + (0.14128 * (cos(FOUR_PI * ratio))) - (0.01168 * (cos(SIX_PI * ratio)));
      break;
    case FFT_WIN_TYP_FLT_TOP: // flat top
      weighingFactor = 0.2810639 - (0.5208972 * cos(TWO_PI * ratio)) + (0.1980399 * cos(FOUR_PI * ratio));
      break;
    case FFT_WIN_TYP_WELCH: // welch
      weighingFactor = 1.0 - pow((indexMinusOne - samplesMinusOne / 2.0) / (samplesMinusOne / 2.0), 2);
      break;
    }
    if (dir == FFT_FORWARD) {
      vData[i] *= weighingFactor;
      vData[samples - (i + 1)] *= weighingFactor;
    }
    else {
      vData[i] /= weighingFactor;
      vData[samples - (i + 1)] /= weighingFactor;
    }
  }
}

void fft_major_peak(double *vData, uint16_t samples, double samplingFrequency, double *f, double *v) {
  double maxY = 0;
  uint16_t IndexOfMaxY = 0;
  //If sampling_frequency = 2 * max_frequency in signal,
  //value would be stored at position samples/2
  for (uint16_t i = 1; i < ((samples >> 1) + 1); i++) {
    if ((vData[i - 1] < vData[i]) && (vData[i] > vData[i + 1])) {
      if (vData[i] > maxY) {
        maxY = vData[i];
        IndexOfMaxY = i;
      }
    }
  }
  double delta = 0.5 * ((vData[IndexOfMaxY - 1] - vData[IndexOfMaxY + 1]) / (vData[IndexOfMaxY - 1] - (2.0 * vData[IndexOfMaxY]) + vData[IndexOfMaxY + 1]));
  double interpolatedX = ((IndexOfMaxY + delta) * samplingFrequency) / (samples - 1);
  if (IndexOfMaxY == (samples >> 1)) {
    //To improve calculation on edge values
    interpolatedX = ((IndexOfMaxY + delta) * samplingFrequency) / (samples);
  }
  // returned value: interpolated frequency peak apex
  *f = interpolatedX;
  *v = abs(vData[IndexOfMaxY - 1] - (2.0 * vData[IndexOfMaxY]) + vData[IndexOfMaxY + 1]);
}

void fft_major_peak_parabola(double *vData, uint16_t samples, double samplingFrequency, double *f) {
  double maxY = 0;
  uint16_t IndexOfMaxY = 0;
  //If sampling_frequency = 2 * max_frequency in signal,
  //value would be stored at position samples/2
  for (uint16_t i = 1; i < ((samples >> 1) + 1); i++) {
    if ((vData[i - 1] < vData[i]) && (vData[i] > vData[i + 1])) {
      if (vData[i] > maxY) {
        maxY = vData[i];
        IndexOfMaxY = i;
      }
    }
  }
  double freq = 0;
  if( IndexOfMaxY > 0) {
    // Assume the three points to be on a parabola
    double a,b,c;
    fft_parabola(IndexOfMaxY-1, vData[IndexOfMaxY-1], IndexOfMaxY, vData[IndexOfMaxY], IndexOfMaxY+1, vData[IndexOfMaxY+1], &a, &b, &c);
    // Peak is at the middle of the parabola
    double x = -b / (2 * a);
    // And magnitude is at the extrema of the parabola if you want It...
    // double y = a*x*x+b*x+c;
    // Convert to frequency
    freq = (x * samplingFrequency) / (samples);
  }
  *f = freq;
}

// Mic
#define FFT_ANALOG_PIN 26 // RP2040 GP26
#define FFT_SAMPLES 1024 // power of 2
#define FFT_SAMPLING_FREQ 30000 // 15 kHz Fmax = sampleF /2 
#define FFT_AMPLITUDE 100 // sensitivity
#define FFT_FREQUENCY_BANDS 54

// OLED
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_SCREEN_WIDTH 128
#define OLED_SCREEN_HEIGHT 32
#define OLED_BAR_WIDTH 2
#define OLED_BAR_GAP 1
#define OLED_BARS 43

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET);

// adjust reference to get remove background noise
float reference = log10(50.0);
unsigned long samplingPeriod;
double cutoffFrequencies[FFT_FREQUENCY_BANDS];
double vReal[FFT_SAMPLES];
double vImag[FFT_SAMPLES];
uint8_t power;

void setup() {
  power = fft_exponent(FFT_SAMPLES);

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
}

int oldHeight[60];
int oldMax[60];

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
  fft_dc_removal(vReal, FFT_SAMPLES);
  fft_windowing(vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  fft_compute(vReal, vImag, FFT_SAMPLES, power, FFT_FORWARD);
  fft_complex_to_magnitude(vReal, vImag, FFT_SAMPLES);

  double median[60];
  double max[60];
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

  for (int i = FFT_FREQUENCY_BANDS - 1; i > 10; i--) {
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
    if (newHeight >= OLED_SCREEN_HEIGHT - 1) {
      newHeight = OLED_SCREEN_HEIGHT - 2;
    }
    if (newMax >= OLED_SCREEN_HEIGHT - 1) {
      newMax = OLED_SCREEN_HEIGHT - 2;
    }

    int barX = bar * (OLED_BAR_WIDTH + OLED_BAR_GAP);
    // remove old level median
    if (oldHeight[i] > newHeight) {
      display.fillRect(barX, newHeight + 1, OLED_BAR_WIDTH, oldHeight[i], SSD1306_BLACK);
    }
    // remove old max level
    if (oldMax[i] > newHeight) {
      for (int j = oldMax[i]; j > newHeight; j -= 2) {
        display.drawFastHLine(barX, j, OLED_BAR_WIDTH, SSD1306_BLACK);
      }
    }
    // paint new max level
    for (int j = newMax; j > newHeight; j -= 2) {
      display.drawFastHLine(barX, j, OLED_BAR_WIDTH, SSD1306_WHITE);
    }
    // paint new level median
    display.fillRect(barX, 1, OLED_BAR_WIDTH, newHeight, SSD1306_WHITE);

    oldMax[i] = newMax;
    oldHeight[i] = newHeight;
    bar++;
  }
  display.display();
}
