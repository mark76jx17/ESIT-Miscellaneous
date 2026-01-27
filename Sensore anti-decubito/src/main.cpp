#include <Arduino.h>

const int TILT_PIN_1 = 25;
const int TILT_PIN_2 = 26;

const unsigned long INTERVALLO = 1000;
unsigned long ultimoInvio = 0;

void setup() {
  Serial.begin(115200);
  pinMode(TILT_PIN_1, INPUT_PULLUP);
  pinMode(TILT_PIN_2, INPUT_PULLUP);
}

void loop() {
  unsigned long ora = millis();
  
  if (ora - ultimoInvio >= INTERVALLO) {
    ultimoInvio = ora;
    
    int stato1 = digitalRead(TILT_PIN_1);
    int stato2 = digitalRead(TILT_PIN_2);
    
    Serial.print("Paziente 1 - Posizione: ");
    Serial.print(stato1 == HIGH ? "Sdraiato" : "In piedi/Seduto");
    
    Serial.print(" | Paziente 2 - Posizione: ");
    Serial.println(stato2 == HIGH ? "Sdraiato" : "In piedi/Seduto");
  }
}