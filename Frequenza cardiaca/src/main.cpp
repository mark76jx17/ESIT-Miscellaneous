#include <Arduino.h>

const int BTN_PIN_1 = 27;
const int BTN_PIN_2 = 17;

int counter1 = 3;
int counter2 = 3;

bool btnPrec1 = false;
bool btnPrec2 = false;

// Intervalli tra battiti in ms (60000 / BPM)
const int INTERVALLO_BATTITO[] = {1333, 1000, 750, 545, 400};  // ~45, 60, 80, 110, 150 BPM
const char* STATO[] = {"Bradicardia grave", "Bradicardia", "Normale", "Tachicardia", "Tachicardia grave"};

unsigned long ultimoBattito1 = 0;
unsigned long ultimoBattito2 = 0;

bool battito1 = false;
bool battito2 = false;

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN_1, INPUT_PULLUP);
  pinMode(BTN_PIN_2, INPUT_PULLUP);
}

void loop() {
  unsigned long ora = millis();
  
  // Lettura bottoni
  bool lettura1 = digitalRead(BTN_PIN_1) == LOW;
  bool lettura2 = digitalRead(BTN_PIN_2) == LOW;
  
  if (lettura1 && !btnPrec1) {
    counter1 = (counter1 % 5) + 1;
    Serial.print("Paziente 1 - Livello: ");
    Serial.println(STATO[counter1 - 1]);
  }
  if (lettura2 && !btnPrec2) {
    counter2 = (counter2 % 5) + 1;
    Serial.print("Paziente 2 - Livello: ");
    Serial.println(STATO[counter2 - 1]);
  }
  
  btnPrec1 = lettura1;
  btnPrec2 = lettura2;
  
  // Genera impulsi paziente 1
  if (ora - ultimoBattito1 >= INTERVALLO_BATTITO[counter1 - 1]) {
    ultimoBattito1 = ora;
    battito1 = true;
  }
  
  // Genera impulsi paziente 2
  if (ora - ultimoBattito2 >= INTERVALLO_BATTITO[counter2 - 1]) {
    ultimoBattito2 = ora;
    battito2 = true;
  }
  
  // Trasmetti impulsi
  if (battito1) {
    Serial.print("P1:BEAT (");
    Serial.print(STATO[counter1 - 1]);
    Serial.println(")");
    battito1 = false;
  }
  if (battito2) {
    Serial.print("P2:BEAT (");
    Serial.print(STATO[counter2 - 1]);
    Serial.println(")");
    battito2 = false;
  }
}
