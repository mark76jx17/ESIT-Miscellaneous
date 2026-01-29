/*
 * ============================================================================
 * ALLARME SATURAZIONE OSSIGENO (SpO2)
 * ============================================================================
 * 
 * Logica semplice:
 *   - SpO2 < 90%        → ALLARME (melodia)
 *   - SpO2 >= 90%       → NORMALE (silenzio)
 *   - Allarme > 30s     → PERSISTENTE (tono piatto)
 *   - Sensore staccato  → ERRORE (beep intermittente)
 * 
 * ============================================================================
 */

#include <Arduino.h>
#include "pitches.h"

// ============================================================================
// PIN
// ============================================================================
#define BUZZER_PIN1    18
#define BUZZER_PIN2    19
#define POT_SPO2_P1    34
#define POT_SPO2_P2    35

// ============================================================================
// SOGLIE
// ============================================================================
#define SPO2_ALLARME       90.0
#define SPO2_MIN           70.0
#define SPO2_MAX           100.0

// ============================================================================
// TIMING
// ============================================================================
#define TEMPO_PERSISTENTE      30000  // 30s per tono piatto
#define INTERVALLO_BEEP_ERRORE 300    // ms tra beep errore
#define INTERVALLO_LETTURA     1000   // ms tra letture

// ============================================================================
// FREQUENZE
// ============================================================================
#define FREQ_PERSISTENTE  2000
#define FREQ_ERRORE       600

// ============================================================================
// STATI
// ============================================================================
enum Stato {
  NORMALE,
  ALLARME,
  PERSISTENTE,
  ERRORE
};

// ============================================================================
// STRUTTURA PAZIENTE
// ============================================================================
struct Paziente {
  const char* nome;
  uint8_t buzzerPin;
  uint8_t potPin;
  
  Stato stato;
  float spo2;
  
  unsigned long tempoInizioAllarme;
  unsigned long ultimoBeep;
  bool beepOn;
};

// ============================================================================
// PAZIENTI
// ============================================================================
Paziente p1 = {"Paziente 1", BUZZER_PIN1, POT_SPO2_P1, NORMALE, 98.0, 0, 0, false};
Paziente p2 = {"Paziente 2", BUZZER_PIN2, POT_SPO2_P2, NORMALE, 98.0, 0, 0, false};

// ============================================================================
// TIMING LETTURA
// ============================================================================
unsigned long ultimaLettura = 0;

// ============================================================================
// MELODIA ALLARME
// ============================================================================
const int melody[] = {NOTE_E6, NOTE_E6, 0, NOTE_E6, NOTE_E6, 0, NOTE_E6, NOTE_E6, 0, 0};
const int durate[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 4};
const int NUM_NOTE = 10;

// Per melodia non bloccante
int notaCorrente1 = 0;
int notaCorrente2 = 0;
unsigned long ultimaNota1 = 0;
unsigned long ultimaNota2 = 0;

// ============================================================================
// FUNZIONI
// ============================================================================

float mapFloat(int val, int inMin, int inMax, float outMin, float outMax) {
  return (float)(val - inMin) * (outMax - outMin) / (float)(inMax - inMin) + outMin;
}

void cambiaStato(Paziente &p, Stato nuovo) {
  if (p.stato == nuovo) return;
  
  Serial.printf("[%s] %d -> %d\n", p.nome, p.stato, nuovo);
  
  noTone(p.buzzerPin);
  p.beepOn = false;
  
  p.stato = nuovo;
  
  if (nuovo == ALLARME) {
    p.tempoInizioAllarme = millis();
  }
}

void leggiSensore(Paziente &p) {
  int raw = analogRead(p.potPin);
  
  // Errore sensore: valori estremi
  if (raw < 50 || raw > 4000) {
    cambiaStato(p, ERRORE);
    return;
  }
  
  // Se era in errore e ora è ok
  if (p.stato == ERRORE) {
    cambiaStato(p, NORMALE);
  }
  
  p.spo2 = mapFloat(raw, 0, 4095, SPO2_MIN, SPO2_MAX);
  
  Serial.printf("[%s] SpO2: %.1f%%\n", p.nome, p.spo2);
}

void verificaSoglia(Paziente &p) {
  if (p.stato == ERRORE) return;
  
  if (p.spo2 < SPO2_ALLARME) {
    if (p.stato == NORMALE) {
      cambiaStato(p, ALLARME);
    }
  } else {
    if (p.stato == ALLARME || p.stato == PERSISTENTE) {
      cambiaStato(p, NORMALE);
    }
  }
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

void suonaErrore(Paziente &p) {
  unsigned long now = millis();
  
  if (now - p.ultimoBeep >= INTERVALLO_BEEP_ERRORE) {
    p.ultimoBeep = now;
    p.beepOn = !p.beepOn;
    
    if (p.beepOn) {
      tone(p.buzzerPin, FREQ_ERRORE);
    } else {
      noTone(p.buzzerPin);
    }
  }
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
    case ERRORE:
      suonaErrore(p);
      break;
  }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  
  pinMode(BUZZER_PIN1, OUTPUT);
  pinMode(BUZZER_PIN2, OUTPUT);
  
  Serial.println("\n=== ALLARME SpO2 ===");
  Serial.println("NORMALE < 90% -> ALLARME");
  Serial.println("ALLARME 30s -> PERSISTENTE");
  Serial.println("Sensore staccato -> ERRORE\n");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  unsigned long now = millis();
  
  // Lettura sensori ogni secondo
  if (now - ultimaLettura >= INTERVALLO_LETTURA) {
    ultimaLettura = now;
    
    leggiSensore(p1);
    leggiSensore(p2);
    
    verificaSoglia(p1);
    verificaSoglia(p2);
  }
  
  // Controllo timeout persistente
  controllaTimeout(p1);
  controllaTimeout(p2);
  
  // Audio
  gestisciAudio(p1, notaCorrente1, ultimaNota1);
  gestisciAudio(p2, notaCorrente2, ultimaNota2);
  
  delay(10);
}