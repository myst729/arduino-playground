// `uf2conv.py nrf52.ino.hex -c -f 0xADA52840 -o nrf52.uf2`

#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_Client_NRF52.h>

#define ERASE_PIN 38 // P1.06
#define LED1_PIN 42 // P1.10
#define LED2_PIN 15 // P0.15

#define KEY1_PIN 30 // P0.30
#define KEY2_PIN 31 // P0.31
#define KEY3_PIN 29 // P0.29
#define KEY4_PIN 2  // P0.02
#define KEY5_PIN 45 // P1.13
#define KEY6_PIN 3  // P0.03
#define KEY7_PIN 28 // P0.28
#define KEY8_PIN 43 // P1.11

BLEMIDI_CREATE_INSTANCE("Pocket Master BLE", MIDI);

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
  // Serial.begin(115200);

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

  BLEMIDI.configErasePin(ERASE_PIN);
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
