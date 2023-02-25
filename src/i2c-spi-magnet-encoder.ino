#include <MagAlpha.h>
#include "AS5600.h"
#include "Wire.h"

MagAlpha magAlpha; // MA730 over SPI at 3.3v: SCLK -> PB1, MOSI -> PB2, MISO -> PB3, CS -> PF7
AS5600 as5600(&Wire); // AS5600 over I2C at 5v: SCL -> PD0, SDA -> PD1

void setup() {
  Serial.begin(115200);
  magAlpha.begin(10000000, MA_SPI_MODE_3, A0);
  as5600.begin(A1);
  as5600.setDirection(AS5600_CLOCK_WISE);
  delay(1000);
}

void loop() {
  Serial.print("MA730: ");
  Serial.print(magAlpha.readAngle(), 3);
  Serial.print("\t AS5600: ");
  Serial.print(as5600.readAngle());
  Serial.print("\n");
  delay(100);
}
