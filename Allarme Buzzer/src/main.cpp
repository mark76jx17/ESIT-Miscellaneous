#include <Arduino.h>

#define BUZZER_P1 18
#define BUZZER_P2 19

void setup() {
  Serial.begin(115200);
  
  // Configura i pin come output
  pinMode(BUZZER_P1, OUTPUT);
  pinMode(BUZZER_P2, OUTPUT);
  
  // Assicurati che siano spenti all'avvio
  digitalWrite(BUZZER_P1, LOW);
  digitalWrite(BUZZER_P2, LOW);
  
  Serial.println("Test buzzer pronto!");
  Serial.println("Comandi: 1=Buzzer1, 2=Buzzer2, 3=Entrambi, 0=Stop");
}

void beep(int pin, int freq, int duration) {
  tone(pin, freq, duration);
}

void testSequence() {
  Serial.println("Test Buzzer 1 (PIN 18)...");
  beep(BUZZER_P1, 1000, 200);
  delay(300);
  
  Serial.println("Test Buzzer 2 (PIN 19)...");
  beep(BUZZER_P2, 1500, 200);
  delay(300);
  
  Serial.println("Test entrambi...");
  tone(BUZZER_P1, 1000);
  tone(BUZZER_P2, 1500);
  delay(300);
  noTone(BUZZER_P1);
  noTone(BUZZER_P2);
}

void loop() {  
    testSequence();
}