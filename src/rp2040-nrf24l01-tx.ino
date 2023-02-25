#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>

/*
 *      ┌──────┬──────┐
 *      │ IRQ  │ MISO │ GP16
 *      ├──────┼──────┤
 * GP19 │ MOSI │ SCK  │ GP18
 *      ├──────┼──────┤
 * GP17 │ CSN  │ CE   │ GP6
 *      ├──────┼──────┤
 *  3V3 │ 3V3  │ GND  │ GND
 *      └──────┴──────┘
 */

struct payload_t {
  bool state;
};

RF24 radio(6, 17); // CE, CSN
RF24Network network(radio);

const uint8_t channel = 81;
const uint16_t tx_node = 0x1;
const uint16_t rx_node = 0x0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  radio.begin();
  radio.setChannel(channel);
  network.begin(tx_node);
  pinMode(15, INPUT_PULLUP);
  delay(100);
}

void loop() {
  network.update();
  bool state = digitalRead(15);
  payload_t payload = { !state };
  RF24NetworkHeader header(rx_node);
  network.write(header, &payload, sizeof(payload));
}
