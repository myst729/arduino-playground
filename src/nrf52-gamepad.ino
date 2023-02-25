#include <bluefruit.h>
#include "Adafruit_Keypad.h"

#define BATTERY_PIN 31 // TODO
#define MODE_LED_PIN 12 // TODO
#define ANALOG_X1_PIN 30 // 2
#define ANALOG_Y1_PIN 3
#define ANALOG_X2_PIN 4
#define ANALOG_Y2_PIN 5
#define ROW_COUNT 4
#define COL_COUNT 5

uint8_t rowPins[ROW_COUNT] = { 16, 15, 7, 11 };
uint8_t colPins[COL_COUNT] = { 28, 29, 25, 13, 14 }; // { 27, 26, 25, 13, 14 };
bool keyScans[ROW_COUNT * COL_COUNT] = { 0 };
/*
 * ┌────────┬────────┬────────┬────────┬────────┐
 * │ SOUTH  │ EAST   │        │ NORTH  │ WEST   │
 * ├────────┼────────┼────────┼────────┼────────┤
 * │        │ TOPL1  │ TOPR1  │ TOPL2  │ TOPR2  │
 * ├────────┼────────┼────────┼────────┼────────┤
 * │ SELECT │ START  │  MODE  │ THUMBL │ THUMBR │
 * ├────────┼────────┼────────┼────────┼────────┤
 * │        │ DNORTH │ DSOUTH │ DWEST  │ DEAST  │
 * └────────┴────────┴────────┴────────┴────────┘
 */
char keys[ROW_COUNT][COL_COUNT] = {
  { 'A', 'B', 'C', 'D', 'E' },
  { 'F', 'G', 'H', 'I', 'J' },
  { 'K', 'L', 'M', 'N', 'O' },
  { 'P', 'Q', 'R', 'S', 'T' }
};

Adafruit_Keypad gpKeys = Adafruit_Keypad(makeKeymap(keys), rowPins, colPins, ROW_COUNT, COL_COUNT);
BLEDis bledis;
BLEHidGamepad blegamepad;
hid_gamepad_report_t gp;

void setup() {
  Serial.begin(115200);
  Serial.println("Bluefruit52 HID Gamepad Example");
  Serial.println("-------------------------------");

  // Keypad setup
  gpKeys.begin();
  for (int i = 0; i < ROW_COUNT * COL_COUNT; i++) {
    keyScans[i] = false;
  }

  Bluefruit.begin();
  Bluefruit.setName("EnjoyaBLE");
  Bluefruit.setTxPower(4);

  // Configure and Start Device Information Service
  bledis.setManufacturer("Clover Gears Studio");
  bledis.setModel("nRF52832");
  bledis.begin();

  /* Start BLE HID
   * Note: Apple requires BLE device must have min connection interval >= 20m
   * ( The smaller the connection interval the faster we could send data).
   * However for HID and MIDI device, Apple could accept min connection interval 
   * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
   * connection interval to 11.25  ms and 15 ms respectively for best performance.
   */
  blegamepad.begin();

  /* Set connection interval (min, max) to your perferred value.
   * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
   * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms
   */
  /* Bluefruit.Periph.setConnInterval(9, 12); */

  // Set up advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_GAMEPAD);
  Bluefruit.Advertising.addService(blegamepad);
  Bluefruit.Advertising.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   *
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

int8_t get_joystick_axis_value(uint32_t pin) {
  // Convert 0~1023 to -127~127
  uint32_t value = analogRead(pin);
  int8_t result = (value - 512) / 4;
  if (result > 127) return 127;
  if (result < -127) return -127;
  return result;
}

void loop() {
  // nothing to do if not connected or
  if ( !Bluefruit.connected() ) return;

  // Reset gamepad data
  gp.x = 0;
  gp.y = 0;
  gp.z = 0;
  gp.rz = 0;
  gp.rx = 0;
  gp.ry = 0;
  gp.hat = 0;
  gp.buttons = 0;

  // Key matrix scan
  gpKeys.tick();
  while (gpKeys.available()) {
    keypadEvent e = gpKeys.read();
    uint8_t keynum = e.bit.ROW * COL_COUNT + e.bit.COL;
    Serial.print((char)e.bit.KEY);
    if (e.bit.EVENT == KEY_JUST_PRESSED) {
      Serial.println(" pressed.");
      keyScans[keynum] = true;
    } else if(e.bit.EVENT == KEY_JUST_RELEASED) {
      Serial.println(" released.");
      keyScans[keynum] = false;
    }
  }

  //------------- Analog Joysticks -------------//
  Serial.print("X1: "); Serial.println(analogRead(ANALOG_X1_PIN));
  Serial.print("Y1: "); Serial.println(analogRead(ANALOG_Y1_PIN));
  Serial.print("X2: "); Serial.println(analogRead(ANALOG_X2_PIN));
  Serial.print("Y2: "); Serial.println(analogRead(ANALOG_Y2_PIN));
  gp.x = get_joystick_axis_value(ANALOG_X1_PIN);
  gp.y = get_joystick_axis_value(ANALOG_Y1_PIN);
  gp.z = get_joystick_axis_value(ANALOG_X2_PIN);
  gp.rz = get_joystick_axis_value(ANALOG_Y2_PIN);

  //------------- Analog Triggers -------------//
  // TODO: remove unused
  // Serial.println("Analog Trigger 1 UP");
  // gp.rx = 127;
  // Serial.println("Analog Trigger 1 DOWN");
  // gp.rx = -127;
  // Serial.println("Analog Trigger 1 CENTER");
  // gp.rx = 0;
  // Serial.println("Analog Trigger 2 UP");
  // gp.ry = 127;
  // Serial.println("Analog Trigger 2 DOWN");
  // gp.ry = -127;
  // Serial.println("Analog Trigger 2 CENTER");
  // gp.ry = 0;

  //------------- Buttons -------------//
  uint16_t button_result = 0;
  for (int j = 0; j < 15; j++) {
    Serial.print("Pressing button "); Serial.println(j);
    if (keyScans[j] == true) {
      button_result += TU_BIT(j);
    }
  }
  gp.buttons = button_result;

  //------------- DPAD / HAT -------------//
  int dpad_result = 0;
  for (int k = 16; k < 20; k++) {
    Serial.print("Pressing button "); Serial.println(k);
    if (keyScans[k] == true) {
      dpad_result += TU_BIT(k - 16);
    }
  }
  /*
   * ┌───┬───┬────┐
   * │ 5 │ 1 │ 9  │
   * ├───┼───┼────┤
   * │ 4 │   │ 8  │
   * ├───┼───┼────┤
   * │ 6 │ 2 │ 10 │
   * └───┴───┴────┘
   */
  switch (dpad_result) {
    case 1:
      gp.hat = GAMEPAD_HAT_UP;
      break;
    case 2:
      gp.hat = GAMEPAD_HAT_DOWN;
      break;
    case 4:
      gp.hat = GAMEPAD_HAT_LEFT;
      break;
    case 8:
      gp.hat = GAMEPAD_HAT_RIGHT;
      break;
    case 5:
      gp.hat = GAMEPAD_HAT_UP_LEFT;
      break;
    case 6:
      gp.hat = GAMEPAD_HAT_DOWN_LEFT;
      break;
    case 9:
      gp.hat = GAMEPAD_HAT_UP_RIGHT;
      break;
    case 10:
      gp.hat = GAMEPAD_HAT_DOWN_RIGHT;
      break;
    default:
      gp.hat = GAMEPAD_HAT_CENTERED;
  }

  blegamepad.report(&gp);
  delay(10);
}
