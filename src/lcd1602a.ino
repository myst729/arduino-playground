#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

Adafruit_LiquidCrystal lcd(13, 12, 11, 10, 9, 8);
uint32_t timer = 0;

void setup() {
  lcd.begin(16, 2);
  lcd.print("Initializing...");
  delay(3000);
  lcd.clear();
}

void loop() {
  if (millis() - timer > 1000) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BPM : ");
    lcd.print(timer % 1000);

    lcd.setCursor(0, 1);
    lcd.print("SpO2: ");
    lcd.print(timer % 100);
    lcd.print("%");

    timer = millis();
  }
}
