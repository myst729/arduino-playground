#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel neon_a(64, PB4,  NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_b(64, PA13, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_c(64, PB12, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_d(64, PB13, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_e(64, PB14, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon_f(64, PB3,  NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon[6] = { neon_a, neon_b, neon_c, neon_d, neon_e, neon_f };
uint32_t color = 0;
uint32_t frame = 0;

void setup() {
  for (int j = 0; j < 6; j++) {
    neon[j].begin();
    neon[j].setBrightness(4);
  }
  delay(500);
}

void loop() {
  frame++;
  if (frame == 64) {
    frame = 0;
  }
  for (uint32_t i = 0; i < 6; i++) {
    color = neon[i].ColorHSV((1023 * frame + 10922 * i) % 65536, 200, 120);
    neon[i].clear();
    neon[i].fill(color, frame - (frame % 8), 8);
    neon[i].show();
  }
  delay(30);
}
