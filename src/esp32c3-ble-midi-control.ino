/*
 * Requires Arduino core for ESP32 family: https://github.com/espressif/arduino-esp32
 * Requires Arduino BLE MIDI library: https://github.com/lathoub/Arduino-BLE-MIDI
 * For MIDI CC details, get Sonicake Pocket Master user manual at https://sonicake.net/product/pocket-master/
 */

#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>

#define LED1_PIN GPIO_NUM_12
#define LED2_PIN GPIO_NUM_13

#define KEY1_PIN GPIO_NUM_2
#define KEY2_PIN GPIO_NUM_3
#define KEY3_PIN GPIO_NUM_10
#define KEY4_PIN GPIO_NUM_6
#define KEY5_PIN GPIO_NUM_5
#define KEY6_PIN GPIO_NUM_4
#define KEY7_PIN GPIO_NUM_8
#define KEY8_PIN GPIO_NUM_9

BLEMIDI_CREATE_INSTANCE("ESP32 Pocket Slave", MIDI);

unsigned long t0 = millis();
bool isConnected = false;

void onConnected() {
  isConnected = true;
  digitalWrite(LED1_PIN, HIGH);
}

void onDisconnected() {
  isConnected = false;
  digitalWrite(LED1_PIN, LOW);
}

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);

  pinMode(KEY1_PIN, INPUT_PULLUP);
  pinMode(KEY2_PIN, INPUT_PULLUP);
  pinMode(KEY3_PIN, INPUT_PULLUP);
  pinMode(KEY4_PIN, INPUT_PULLUP);
  pinMode(KEY5_PIN, INPUT_PULLUP);
  pinMode(KEY6_PIN, INPUT_PULLUP);
  pinMode(KEY7_PIN, INPUT_PULLUP);
  pinMode(KEY8_PIN, INPUT_PULLUP);

  BLEMIDI.setHandleConnected(onConnected);
  BLEMIDI.setHandleDisconnected(onDisconnected);
  MIDI.begin();
}

void loop() {
  MIDI.read();

  if (isConnected && (millis() - t0) > 1000) {
    t0 = millis();
    if (digitalRead(KEY1_PIN) == LOW) { MIDI.sendControlChange(24, 0, 1);   return; } // Preset -
    if (digitalRead(KEY2_PIN) == LOW) { MIDI.sendControlChange(25, 0, 1);   return; } // Preset +
    if (digitalRead(KEY3_PIN) == LOW) { MIDI.sendControlChange(58, 0, 1);   return; } // Tuner GUI Off
    if (digitalRead(KEY4_PIN) == LOW) { MIDI.sendControlChange(58, 127, 1); return; } // Tuner GUI On
    if (digitalRead(KEY5_PIN) == LOW) { MIDI.sendControlChange(92, 0, 1);   return; } // Drum GUI Off
    if (digitalRead(KEY6_PIN) == LOW) { MIDI.sendControlChange(92, 127, 1); return; } // Drum GUI On
    if (digitalRead(KEY7_PIN) == LOW) { MIDI.sendControlChange(93, 0, 1);   return; } // Drum Stop
    if (digitalRead(KEY8_PIN) == LOW) { MIDI.sendControlChange(93, 127, 1); return; } // Drum Play
  }

  delay(10);
}
