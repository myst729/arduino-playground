#include <Adafruit_MPU6050.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "STM32LowPower.h"

/**
 * ┌────┬────┬────┬────┬────┬────┬────┬────┐
 * │  0 │  1 │  2 │  3 │  4 │  5 │  6 │  7 │
 * ├────┼────┼────┼────┼────┼────┼────┼────┤
 * │ 15 │ 14 │ 13 │ 12 │ 11 │ 10 │  9 │  8 │
 * ├────┼────┼────┼────┼────┼────┼────┼────┤
 * │ 16 │ 17 │ 18 │ 19 │ 20 │ 21 │ 22 │ 23 │
 * ├────┼────┼────┼────┼────┼────┼────┼────┤
 * │ 31 │ 30 │ 29 │ 28 │ 27 │ 26 │ 25 │ 24 │
 * ├────┼────┼────┼────┼────┼────┼────┼────┤
 * │ 32 │ 33 │ 34 │ 35 │ 36 │ 37 │ 38 │ 39 │
 * ├────┼────┼────┼────┼────┼────┼────┼────┤
 * │ 47 │ 46 │ 45 │ 44 │ 43 │ 42 │ 41 │ 40 │
 * ├────┼────┼────┼────┼────┼────┼────┼────┤
 * │ 48 │ 49 │ 50 │ 51 │ 52 │ 53 │ 54 │ 55 │
 * ├────┼────┼────┼────┼────┼────┼────┼────┤
 * │ 63 │ 62 │ 61 │ 60 │ 59 │ 58 │ 57 │ 56 │
 * └────┴────┴────┴────┴────┴────┴────┴────┘
 */

/* Control */
#define PIN_GYRO_SCL    PB6
#define PIN_GYRO_SDA    PB7
#define PIN_SYS_WAKEUP  PA0
#define PIN_BUZZER      PA4
#define PIN_BATTERY     PA5
#define PIN_CHARGING    PA7
#define PIN_NEON_POWER  PA14
#define LOW_POWER_MODE  SHUTDOWN_MODE // 0: IDLE_MODE, 1: SLEEP_MODE, 2: DEEP_SLEEP_MODE, 3: SHUTDOWN_MODE
#define SLOW_INTERVAL   60000 // unit: ms
#define SLEEP_INTERVAL  90000 // unit: ms

/* WS2812 */
#define PIN_NEON_A      PB14
#define PIN_NEON_B      PB3
#define PIN_NEON_C      PA13
#define PIN_NEON_D      PB13
#define PIN_NEON_E      PB4
#define PIN_NEON_F      PB12
#define NUM_NEON        6
#define NUM_WS2812      64

/* Effects */
#define NEON_ZIGZAG     { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63 }
#define NEON_SCANLINE   { 0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8, 16, 17, 18, 19, 20, 21, 22, 23, 31, 30, 29, 28, 27, 26, 25, 24, 32, 33, 34, 35, 36, 37, 38, 39, 47, 46, 45, 44, 43, 42, 41, 40, 48, 49, 50, 51, 52, 53, 54, 55, 63, 62, 61, 60, 59, 58, 57, 56 }
#define NEON_SWIRL_IN   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 33, 32, 31, 30, 29, 28, 27, 26, 47, 48, 49, 50, 51, 34, 9, 10, 35, 52, 61, 60, 59, 46, 25, 24, 45, 58, 63, 62, 53, 36, 11, 12, 37, 54, 55, 56, 57, 44, 23, 22, 43, 42, 41, 40, 39, 38, 13, 14, 15, 16, 17, 18, 19, 20, 21 }
#define NEON_SWIRL_OUT  { 63, 62, 61, 60, 59, 58, 57, 56, 55, 30, 31, 32, 33, 34, 35, 36, 37, 16, 15, 14, 13, 12, 29, 54, 53, 28, 11, 2, 3, 4, 17, 38, 39, 18, 5, 0, 1, 10, 27, 52, 51, 26, 9, 8, 7, 6, 19, 40, 41, 20, 21, 22, 23, 24, 25, 50, 49, 48, 47, 46, 45, 44, 43, 42 }
#define NEON_CIRCLE_IN  { 0, 1, 2, 3, 3, 2, 1, 0, 1, 2, 3, 4, 4, 3, 2, 1, 2, 3, 4, 5, 5, 4, 3, 2, 3, 4, 5, 6, 6, 5, 4, 3, 3, 4, 5, 6, 6, 5, 4, 3, 2, 3, 4, 5, 5, 4, 3, 2, 1, 2, 3, 4, 4, 3, 2, 1, 0, 1, 2, 3, 3, 2, 1, 0 }
#define NEON_CIRCLE_OUT { 6, 5, 4, 3, 3, 4, 5, 6, 5, 4, 3, 2, 2, 3, 4, 5, 4, 3, 2, 1, 1, 2, 3, 4, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 4, 3, 2, 1, 1, 2, 3, 4, 5, 4, 3, 2, 2, 3, 4, 5, 6, 5, 4, 3, 3, 4, 5, 6 }
#define NEON_SQUARE_IN  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 2, 2, 2, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 2, 2, 2, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
#define NEON_SQUARE_OUT { 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 3, 3, 2, 1, 1, 1, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 1, 1, 1, 2, 3, 3, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3 }
#define NEON_BRIGHTNESS 8
#define NUM_EFFECT      8

/* Color */
#define COLOR_LOW_BATT  0x12FF00 // red
#define COLOR_CHARGING0 0x66FF00 // orange
#define COLOR_CHARGING1 0xFFCC00 // yellow
#define COLOR_OFF       0x0      // off

/************************************************************************************/

Adafruit_MPU6050 mpu;
sensors_event_t mpu_a, mpu_g, mpu_temp;
float ax, ay, az;

Adafruit_NeoPixel neon_a(NUM_WS2812, PIN_NEON_A, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_b(NUM_WS2812, PIN_NEON_B, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_c(NUM_WS2812, PIN_NEON_C, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_d(NUM_WS2812, PIN_NEON_D, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_e(NUM_WS2812, PIN_NEON_E, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_f(NUM_WS2812, PIN_NEON_F, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon[NUM_NEON] = { neon_a, neon_b, neon_c, neon_d, neon_e, neon_f };

int neon_effects[NUM_EFFECT][NUM_WS2812] = { NEON_ZIGZAG, NEON_SCANLINE, NEON_SWIRL_IN, NEON_SWIRL_OUT, NEON_CIRCLE_IN, NEON_CIRCLE_OUT, NEON_SQUARE_IN, NEON_SQUARE_OUT };
int face = 0, frame = 0, effect = 0, effect_interval = 30;
uint32_t color = 0;

bool is_charging = false, low_battery = false;
int running_mode = 1; // 1: fast, 2: slow, 3: sleep
uint32_t power_saving_timer = 0;

void play_tone_raise() {
  tone(PIN_BUZZER, 330, 100); // Note E4
  delay(100);
  noTone(PIN_BUZZER);
  tone(PIN_BUZZER, 440, 100); // Note A4
  delay(100);
  noTone(PIN_BUZZER);
}

void play_tone_fall() {
  tone(PIN_BUZZER, 440, 100); // Note A4
  delay(100);
  noTone(PIN_BUZZER);
  tone(PIN_BUZZER, 330, 100); // Note E4
  delay(100);
  noTone(PIN_BUZZER);
}

void init_gyro() {
  if (!mpu.begin()) {
    while (1) {
      delay(10);
    }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
}

void update_gyro() {
  int top = 7;
  mpu.getEvent(&mpu_a, &mpu_g, &mpu_temp);
  ax = mpu_a.acceleration.x;
  ay = mpu_a.acceleration.y;
  az = mpu_a.acceleration.z;

  if (ax > 7 || ax < -7) {
    if (ay > -2 && ay < 2 && az > -2 && az < 2) {
      top = ax < 0 ? 0 : 1;
    }
  } else if (ay > 7 || ay < -7) {
    if (ax > -2 && ax < 2 && az > -2 && az < 2) {
      top = ay < 0 ? 2 : 3;
    }
  } else if (az > 7 || az < -7) {
    if (ax > -2 && ax < 2 && ay > -2 && ay < 2) {
      top = az < 0 ? 4 : 5;
    }
  }

  if (top < 7 && top != face) {
    neon[face].clear();
    neon[face].show();
    face = top;
    power_saving_timer = millis();
    play_tone_fall();
  }
}

void update_neon() {
  if (is_charging) {
    // charging animation
    color = low_battery ? COLOR_CHARGING0 : COLOR_CHARGING1;
    neon[face].clear();
    neon[face].fill(color, frame - (frame % 8), 8);
  } else if (low_battery) {
    // blink to charge
    color = (frame % 16 < 8) ? COLOR_LOW_BATT : COLOR_OFF;
    neon[face].clear();
    neon[face].fill(color, 0, 8);
  } else {
    // play neon effects
    int index = (face + effect) % NUM_EFFECT;
    int threshold = index < 4 ? 6 : 3;
    for (int j = 0; j < NUM_WS2812; j++) {
      int diff = index < 4 ? (frame - neon_effects[index][j] + NUM_WS2812) % NUM_WS2812 : (frame / 4 - neon_effects[index][j] + 8) % 8;
      if (diff >= 0 && diff < threshold) {
        color = neon[face].ColorHSV((uint16_t)(65535 / NUM_EFFECT * index), 200, 160 / diff);
        neon[face].setPixelColor(j, color);
      } else {
        neon[face].setPixelColor(j, COLOR_OFF);
      }
    }
  }
  neon[face].show();
  delay(effect_interval);
}

void speed_up() {
  for (int i = 0; i < NUM_NEON; i++) {
    neon[i].setBrightness(NEON_BRIGHTNESS);
  }
  effect_interval = 30;
  running_mode = 1;
  power_saving_timer = millis();
  digitalWrite(PIN_NEON_POWER, LOW);
  mpu.setMotionInterrupt(false);
}

void slow_down() {
  for (int i = 0; i < NUM_NEON; i++) {
    neon[i].setBrightness(NEON_BRIGHTNESS / 4);
  }
  effect_interval = 80;
  running_mode = 2;
}

void fall_asleep() {
  for (int i = 0; i < NUM_NEON; i++) {
    neon[i].setBrightness(0);
    neon[i].clear();
    neon[i].show();
  }
  effect_interval = 1000;
  running_mode = 3;
  digitalWrite(PIN_NEON_POWER, HIGH);

  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);
  mpu.setInterruptPinPolarity(false);
  mpu.setMotionInterrupt(true);

#if (LOW_POWER_MODE == IDLE_MODE)
  LowPower.idle();
#elif (LOW_POWER_MODE == SLEEP_MODE)
  LowPower.sleep();
#elif (LOW_POWER_MODE == DEEP_SLEEP_MODE)
  LowPower.deepSleep();
#elif (LOW_POWER_MODE == SHUTDOWN_MODE)
  LowPower.shutdown();
#endif
}

void wake_up() {
  speed_up();
  play_tone_raise();
}

void check_battery() {
  bool charging_stat = !digitalRead(PIN_CHARGING);
  uint32_t raw = analogRead(PIN_BATTERY);
  uint32_t vbatt = raw * 3320 * 2 / 1024; // unit: mV
  if (charging_stat) {
    power_saving_timer = millis();
  }
  is_charging = charging_stat && vbatt < 4200;
  low_battery = vbatt < 3300;
}

void check_state() {
  uint32_t time_gap = millis() - power_saving_timer;
  switch (running_mode) {
    case 1:
      if (time_gap > SLOW_INTERVAL) {
        slow_down();
      }
      break;
    case 2:
      if (time_gap > SLEEP_INTERVAL) {
        fall_asleep();
      } else if (time_gap <= SLOW_INTERVAL) {
        speed_up();
      }
      break;
  }
}

void setup() {
  pinMode(PIN_BATTERY,    INPUT_ANALOG);
  pinMode(PIN_CHARGING,   INPUT_PULLUP);
  pinMode(PIN_BUZZER,     OUTPUT);
  pinMode(PIN_NEON_POWER, OUTPUT);

  init_gyro();
  for (int i = 0; i < NUM_NEON; i++) {
    neon[i].begin();
  }
  delay(500);
  wake_up();

  LowPower.begin();
  LowPower.attachInterruptWakeup(PIN_SYS_WAKEUP, wake_up, RISING, LOW_POWER_MODE);
}

void loop() {
  if (running_mode < 3) {
    update_gyro();
    update_neon();

    frame++;
    if (frame == NUM_WS2812) {
      frame = 0;
      effect++;
      if (effect == NUM_EFFECT) {
        effect = 0;
      }
      check_battery();
      check_state();
    }
  }
}
