#include <Arduino.h>

const int POT_PIN_1 = 34;
const int POT_PIN_2 = 35;

const float SPO2_MIN = 70.0;
const float SPO2_MAX = 100.0;

const unsigned long INTERVALLO_TRASMISSIONE_SPO2 = 1000;
unsigned long ultimoInvio = 0;

float mapFloat(int value, int inMin, int inMax, float outMin, float outMax) {
  return (float)(value - inMin) * (outMax - outMin) / (float)(inMax - inMin) + outMin;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  unsigned long ora = millis();
  
  if (ora - ultimoInvio >= INTERVALLO_TRASMISSIONE_SPO2) {
    ultimoInvio = ora;
    
    int valore1 = analogRead(POT_PIN_1);
    int valore2 = analogRead(POT_PIN_2);
    
    float spo2_1 = mapFloat(valore1, 0, 4095, SPO2_MIN, SPO2_MAX);
    float spo2_2 = mapFloat(valore2, 0, 4095, SPO2_MIN, SPO2_MAX);
    
    Serial.print("Paziente 1 - SpO2: ");
    Serial.print(spo2_1, 1);
    Serial.print("% | Paziente 2 - SpO2: ");
    Serial.print(spo2_2, 1);
    Serial.println("%");
  }
  delay(10);
}
