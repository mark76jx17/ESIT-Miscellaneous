/*
 * ============================================================================
 * ALLARME SATURAZIONE OSSIGENO (SpO2)
 * ============================================================================
 * 
 *   - SpO2 < 90%        → ALLARME (melodia)
 *   - SpO2 >= 90%       → NORMALE (silenzio)
 *   - Allarme > 30s     → PERSISTENTE (tono piatto)
 * 
 * ============================================================================
 */

#include <Arduino.h>
#include "pitches.h"

#define BUZZER_PIN1    18
#define BUZZER_PIN2    19
#define POT_SPO2_P1    34
#define POT_SPO2_P2    35
#define SPO2_ALLARME       90.0
#define SPO2_MIN           70.0
#define SPO2_MAX           100.0
#define TEMPO_PERSISTENTE      30000  // 30s per tono piatto
#define INTERVALLO_LETTURA     1000   // ms tra letture
#define FREQ_PERSISTENTE  2000

enum Stato {
  NORMALE,
  ALLARME,
  PERSISTENTE
};

// =====================
// STRUTTURA PAZIENTE
// =====================

struct Paziente {
  const char* nome;
  uint8_t buzzerPin;
  uint8_t potPin;  
  Stato stato;
  float spo2;  
  unsigned long tempoInizioAllarme;
};

Paziente p1 = {"Paziente 1", BUZZER_PIN1, POT_SPO2_P1, NORMALE, 98.0, 0};
Paziente p2 = {"Paziente 2", BUZZER_PIN2, POT_SPO2_P2, NORMALE, 98.0, 0};
unsigned long ultimaLettura = 0;

const int melody[] = {NOTE_E6, NOTE_E6, 0, NOTE_E6, NOTE_E6, 0, NOTE_E6, NOTE_E6, 0, 0};
const int durate[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 4};
const int NUM_NOTE = 10;

int notaCorrente1 = 0;
int notaCorrente2 = 0;
unsigned long ultimaNota1 = 0;
unsigned long ultimaNota2 = 0;

float mapFloat(int val, int inMin, int inMax, float outMin, float outMax) {
  return (float)(val - inMin) * (outMax - outMin) / (float)(inMax - inMin) + outMin;
}

void cambiaStato(Paziente &p, Stato nuovo) {
  if (p.stato == nuovo) return;
  
  Serial.printf("[%s] %d -> %d\n", p.nome, p.stato, nuovo);
  
  noTone(p.buzzerPin);
  p.stato = nuovo;
  
  if (nuovo == ALLARME) {
    p.tempoInizioAllarme = millis();
  }
}

void leggiSensore(Paziente &p) {
  int raw = analogRead(p.potPin);
  p.spo2 = mapFloat(raw, 0, 4095, SPO2_MIN, SPO2_MAX);

  if (p.spo2 < SPO2_ALLARME) {
    if (p.stato == NORMALE) {
      cambiaStato(p, ALLARME);
    }
  } else {
    if (p.stato == ALLARME || p.stato == PERSISTENTE) {
      cambiaStato(p, NORMALE);
    }
  }
  
  Serial.printf("[%s] SpO2: %.1f%%\n", p.nome, p.spo2);
}

void controllaTimeout(Paziente &p) {
  if (p.stato == ALLARME) {
    if (millis() - p.tempoInizioAllarme >= TEMPO_PERSISTENTE) {
      cambiaStato(p, PERSISTENTE);
    }
  }
}

void suonaAllarme(Paziente &p, int &nota, unsigned long &ultimaNota) {
  unsigned long now = millis();
  int durata = 1000 / durate[nota];
  
  if (now - ultimaNota >= durata * 1.3) {
    ultimaNota = now;
    
    if (melody[nota] != 0) {
      tone(p.buzzerPin, melody[nota], durata);
    } else {
      noTone(p.buzzerPin);
    }
    
    nota = (nota + 1) % NUM_NOTE;
  }
}

void suonaPersistente(Paziente &p) {
  tone(p.buzzerPin, FREQ_PERSISTENTE);
}

void gestisciAudio(Paziente &p, int &nota, unsigned long &ultimaNota) {
  switch (p.stato) {
    case NORMALE:
      break;
    case ALLARME:
      suonaAllarme(p, nota, ultimaNota);
      break;
    case PERSISTENTE:
      suonaPersistente(p);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  
  pinMode(BUZZER_PIN1, OUTPUT);
  pinMode(BUZZER_PIN2, OUTPUT);
}

void loop() {
  unsigned long now = millis();
  
  if (now - ultimaLettura >= INTERVALLO_LETTURA) {
    ultimaLettura = now;    
    leggiSensore(p1);
    leggiSensore(p2);
  }
  
  controllaTimeout(p1);
  controllaTimeout(p2);
  
  gestisciAudio(p1, notaCorrente1, ultimaNota1);
  gestisciAudio(p2, notaCorrente2, ultimaNota2);
  
  delay(10);
}