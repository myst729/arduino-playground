int buzzer = 13;
int buttonPins[8] = { 2, 3, 4, 5, 6, 7, 8, 9 };
int buttonStates[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned int frequencies[7] = { 493.88, 440.000, 391.995, 349.228, 329.628, 293.665, 261.626 };

void setup() {
  pinMode(buzzer, OUTPUT);
  for (int i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void loop() {
  for (int i = 0; i < 8; i++) {
    buttonStates[i] = digitalRead(buttonPins[i]);
  }

  for (int j = 0; j < 7; j++) {
    if (buttonStates[j] == LOW) {
      tone(buzzer, frequencies[j]);
      return;
    };
  }

  if (buttonStates[7] == LOW) {
    tone(buzzer, 329.628);
    noTone(buzzer);
    delay(100);
    tone(buzzer, 329.628);
    delay(100);
    tone(buzzer, 329.628);
    delay(200);
    tone(buzzer, 261.626);
    delay(200);
    noTone(buzzer);
    delay(2000);
    tone(buzzer, 329.628);
    delay(200);
    tone(buzzer, 97.9989);
    delay(100);
    return;
  }

  noTone(buzzer);
}
