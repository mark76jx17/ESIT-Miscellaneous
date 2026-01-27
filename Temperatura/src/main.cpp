#include <Arduino.h>
#include <DHT.h>

const int DHT_PIN_1 = 32;
const int DHT_PIN_2 = 33;
const int DHT_TYPE = DHT11;

DHT dht1(DHT_PIN_1, DHT_TYPE);
DHT dht2(DHT_PIN_2, DHT_TYPE);

const float TEMP_CORPO_MIN = 34;
const float TEMP_CORPO_MAX = 41.0;

const unsigned long INTERVALLO = 1000;
unsigned long ultimoInvio = 0;

float mapFloat(float value, float inMin, float inMax, float outMin, float outMax) {
  return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

void setup() {
  Serial.begin(115200);
  dht1.begin();
  dht2.begin();
}

void loop() {
  unsigned long ora = millis();
  
  if (ora - ultimoInvio >= INTERVALLO) {
    ultimoInvio = ora;
    
    float temp1 = dht1.readTemperature();
    float temp2 = dht2.readTemperature();
    
    Serial.print("Paziente 1 - Temp: ");
    if (isnan(temp1)) {
      Serial.print("Errore");
    } else {
      float tempCorpo1 = mapFloat(temp1, 0, 50, TEMP_CORPO_MIN, TEMP_CORPO_MAX);
      tempCorpo1 = constrain(tempCorpo1, TEMP_CORPO_MIN, TEMP_CORPO_MAX);
      Serial.print(tempCorpo1, 1);
      Serial.print("°C");
    }
    
    Serial.print(" | Paziente 2 - Temp: ");
    if (isnan(temp2)) {
      Serial.println("Errore");
    } else {
      float tempCorpo2 = mapFloat(temp2, 0, 50, TEMP_CORPO_MIN, TEMP_CORPO_MAX);
      tempCorpo2 = constrain(tempCorpo2, TEMP_CORPO_MIN, TEMP_CORPO_MAX);
      Serial.print(tempCorpo2, 1);
      Serial.println("°C");
    }
  }
}