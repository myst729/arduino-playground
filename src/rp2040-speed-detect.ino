#define SAMPLES 1048576 // MUST be a power of 2
#define ANALOG_PIN 26

// Rough calculation of maximum sample rate for Pico

unsigned long start;
int value;

void setup () {
  Serial.begin(9600);
  Serial.println("Sampling... please, wait.");
}

void loop () {
  start = micros();
  for (int i = 0; i < SAMPLES; i++) {
    value = analogRead(ANALOG_PIN);
  }
  float samplingPeriod = (micros() - start) / SAMPLES;

  Serial.print("Time per sample (us): ");
  Serial.println(samplingPeriod);
  Serial.print("Max sampling frequency (hz): ");
  Serial.println(pow(10, 6) / samplingPeriod);
}
