#include <Arduino.h>
#include <Keypad.h>
#include <BleGamepad.h>

#define MODE_LED 11
#define NUM_AXES 4
#define NUM_ROWS 5
#define NUM_COLS 4

bool analogOn = false;
uint8_t analogPins[NUM_AXES] = { 6, 7, 8, 9 };
uint8_t rowPins[NUM_ROWS] = { 2, 13, 14, 25, 10 };
uint8_t colPins[NUM_COLS] = { 4, 23, 21, 22 };
uint8_t keymap[NUM_ROWS][NUM_COLS] = {
  { 1, 2, 3, 4 },
  { 5, 6, 7, 8 },
  { 9, 10, 11, 12 },
  { 13, 14, 15, 16 },
  { 17, 18, 19, 20 }
};

Keypad customKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, NUM_ROWS, NUM_COLS);
BleGamepad bleGamepad("ESP32 BLE Gamepad", "Leo Deng", 100);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Gamepad!");

  pinMode(MODE_LED, OUTPUT);
  digitalWrite(MODE_LED, LOW);

  BleGamepadConfiguration bleGamepadConfig;
  bleGamepadConfig.setAutoReport(false);
  bleGamepad.begin();
}

void loop() {
  if (bleGamepad.isConnected()) {
    customKeypad.getKeys();

    /* Check Mode */
    if (customKeypad.key[16].stateChanged) {
      uint8_t mode_state = customKeypad.key[16].kstate;
      if (mode_state == PRESSED) {
        if (analogOn) {
          analogOn = false;
          digitalWrite(MODE_LED, LOW);
        } else {
          analogOn = true;
          digitalWrite(MODE_LED, HIGH);
        }
      }
      return;
    }

    /* Update Buttons */
    for (int i = 0; i < 12; i++) {
      if (customKeypad.key[i].stateChanged) {
        uint8_t key_state = customKeypad.key[i].kstate;
        if (key_state == PRESSED) {
          bleGamepad.press(customKeypad.key[i].kchar);
          // bleGamepad.pressStart();
          // bleGamepad.pressSelect();
        }
        if (key_state == RELEASED) {
          bleGamepad.release(customKeypad.key[i].kchar);
          // bleGamepad.releaseStart();
          // bleGamepad.releaseSelect();
        }
      }
    }

    /* Update Hat */
    int hatState = 0;
    if (customKeypad.key[12].kstate == PRESSED) hatState += 1; // up
    if (customKeypad.key[13].kstate == PRESSED) hatState += 2; // down
    if (customKeypad.key[14].kstate == PRESSED) hatState += 4; // left
    if (customKeypad.key[15].kstate == PRESSED) hatState += 8; // right
    bleGamepad.setHat(getHatValue(hatState));

    /* Update Joystick */
    int16_t x = getAxisValue(analogPins[0]);
    int16_t y = getAxisValue(analogPins[1]);
    int16_t z = getAxisValue(analogPins[2]);
    int16_t rz = getAxisValue(analogPins[3]);
    bleGamepad.setAxes(x, y, z, rz);

    bleGamepad.sendReport();
    delay(10);
  }
}

signed char getHatValue(int hatState) {
  switch (hatState) {
    case 1:
      return HAT_UP;
    case 2:
      return HAT_DOWN;
    case 4:
      return HAT_LEFT;
    case 8:
      return HAT_RIGHT;
    case 5:
      return HAT_UP_LEFT;
    case 6:
      return HAT_DOWN_LEFT;
    case 9:
      return HAT_UP_RIGHT;
    case 10:
      return HAT_DOWN_RIGHT;
    default:
      return HAT_CENTERED;
  }
}

int16_t getAxisValue(uint8_t pin) {
  uint32_t value = analogRead(pin);
  // Convert 0~1023 to 0~32767
  return value * 32;
}
