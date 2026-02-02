#include <Arduino.h>
#include <DHT.h>
#include "pitches.h"

// Paziente 1
#define BTN_HR_P1       27
#define POT_SPO2_P1     34
#define TILT_P1         25
#define DHT_P1          32
#define BUZZER_P1       18

// Paziente 2
#define BTN_HR_P2       17
#define POT_SPO2_P2     35
#define TILT_P2         26
#define DHT_P2          33
#define BUZZER_P2       19

// Frequenza cardiaca
const int BPM_VALUES[] = {45, 60, 80, 110, 150};
const char* HR_STATI[] = {"Bradicardia grave", "Bradicardia", "Normale", "Tachicardia", "Tachicardia grave"};
const int HR_LIVELLO_NORMALE = 2;

// Saturazione ossigeno
#define SPO2_MIN            70.0
#define SPO2_MAX            100.0
#define SPO2_SOGLIA_ALLARME 90.0

// Temperatura corporea
#define TEMP_MIN            34.0
#define TEMP_MAX            41.0
#define TEMP_SOGLIA_BASSA   35.5
#define TEMP_SOGLIA_ALTA    38.0

// Temporizzazioni
#define INTERVALLO_LETTURA      1000
#define INTERVALLO_DHT          2500
#define TEMPO_PERSISTENTE       30000
#define FREQ_PERSISTENTE        2000    
#define INTERVALLO_STAMPA       2000    

enum StatoAllarme {
  NORMALE,
  ALLARME,
  PERSISTENTE
};

const char* STATO_ALLARME_STR[] = {"OK", "ALLARME", "CRITICO"};

struct AudioAllarme {
  int notaCorrente;
  unsigned long ultimaNota;
};

struct Paziente {
  uint8_t id;
  
  uint8_t pinBtnHR;
  uint8_t pinPotSpO2;
  uint8_t pinTilt;
  uint8_t pinDHT;
  uint8_t pinBuzzer;
  uint8_t canaleLedc;     
  
  int livelloHR;
  int bpm;
  float spo2;
  bool posizioneSdraiato;
  float temperatura;
  bool tempValida;
  
  bool btnPrecedente;
  
  StatoAllarme allarmeHR;
  StatoAllarme allarmeSpo2;
  StatoAllarme allarmeTemp;
  
  unsigned long tempoInizioAllarmeHR;
  unsigned long tempoInizioAllarmeSpo2;
  unsigned long tempoInizioAllarmeTemp;
  
  AudioAllarme audio;
};

DHT dht1(DHT_P1, DHT11);
DHT dht2(DHT_P2, DHT11);
Paziente pazienti[2];

unsigned long ultimaLettura = 0;
unsigned long ultimaLetturaDHT = 0;
unsigned long ultimaStampa = 0;

const int MELODIA[] = {NOTE_E6, NOTE_E6, 0, NOTE_E6, NOTE_E6, 0, NOTE_E6, NOTE_E6, 0, 0};
const int DURATE[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 4};
const int NUM_NOTE = 10;

void buzzerTone(Paziente &p, uint32_t freq) {
  if (freq > 0) {
    ledcWriteTone(p.canaleLedc, freq);
  } else {
    ledcWriteTone(p.canaleLedc, 0);
  }
}

void buzzerStop(Paziente &p) {
  ledcWriteTone(p.canaleLedc, 0);
}

float mapFloat(int val, int inMin, int inMax, float outMin, float outMax) {
  return (float)(val - inMin) * (outMax - outMin) / (float)(inMax - inMin) + outMin;
}

StatoAllarme statoAllarmeGlobale(Paziente &p) {
  StatoAllarme peggiore = NORMALE;
  if (p.allarmeHR > peggiore) peggiore = p.allarmeHR;
  if (p.allarmeSpo2 > peggiore) peggiore = p.allarmeSpo2;
  if (p.allarmeTemp > peggiore) peggiore = p.allarmeTemp;
  return peggiore;
}

void inizializzaPaziente(Paziente &p, uint8_t id,
                         uint8_t btnHR, uint8_t potSpO2, uint8_t tilt,
                         uint8_t dht, uint8_t buzzer) {
  p.id = id;
  p.pinBtnHR = btnHR;
  p.pinPotSpO2 = potSpO2;
  p.pinTilt = tilt;
  p.pinDHT = dht;
  p.pinBuzzer = buzzer;
  p.canaleLedc = id - 1;  // Paziente 1 -> indice 0, Paziente 2 -> infice 1
  
  p.livelloHR = HR_LIVELLO_NORMALE;
  p.bpm = BPM_VALUES[HR_LIVELLO_NORMALE];
  p.spo2 = 98.0;
  p.posizioneSdraiato = true;
  p.temperatura = 36.5;
  p.tempValida = false;
  
  p.btnPrecedente = false;
  
  p.allarmeHR = NORMALE;
  p.allarmeSpo2 = NORMALE;
  p.allarmeTemp = NORMALE;
  
  p.tempoInizioAllarmeHR = 0;
  p.tempoInizioAllarmeSpo2 = 0;
  p.tempoInizioAllarmeTemp = 0;
  
  p.audio.notaCorrente = 0;
  p.audio.ultimaNota = 0;
  
  pinMode(btnHR, INPUT_PULLUP);
  pinMode(tilt, INPUT_PULLUP);
  
  // Setup LEDC per il buzzer
  ledcSetup(p.canaleLedc, 2000, 8);
  ledcAttachPin(buzzer, p.canaleLedc);
}


void cambiaStatoAllarme(Paziente &p, StatoAllarme &stato, 
                        unsigned long &tempoInizio, StatoAllarme nuovo,
                        const char* sensore) {
  if (stato == nuovo) return;
  
  Serial.printf("[%d] %s: %s -> %s\n", 
                p.id, sensore, 
                STATO_ALLARME_STR[stato], 
                STATO_ALLARME_STR[nuovo]);
  
  stato = nuovo;
  
  if (nuovo == ALLARME) {
    tempoInizio = millis();
  } else if (nuovo == NORMALE) {
    buzzerStop(p);
  }
}

void controllaTimeoutAllarme(Paziente &p, StatoAllarme &stato, 
                             unsigned long &tempoInizio, const char* sensore) {
  if (stato == ALLARME && millis() - tempoInizio >= TEMPO_PERSISTENTE) {
    cambiaStatoAllarme(p, stato, tempoInizio, PERSISTENTE, sensore);
  }
}


void leggiFrequenzaCardiaca(Paziente &p) {
  bool btnAttuale = (digitalRead(p.pinBtnHR) == LOW);
  
  if (btnAttuale && !p.btnPrecedente) {
    p.livelloHR = (p.livelloHR + 1) % 5;
    p.bpm = BPM_VALUES[p.livelloHR];
    Serial.printf("[%d] HR Livello: %s (%d BPM)\n", 
                  p.id, HR_STATI[p.livelloHR], p.bpm);
  }
  p.btnPrecedente = btnAttuale;
  
  bool inAllarme = (p.livelloHR != HR_LIVELLO_NORMALE);
  
  if (inAllarme && p.allarmeHR == NORMALE) {
    cambiaStatoAllarme(p, p.allarmeHR, p.tempoInizioAllarmeHR, ALLARME, "HR");
  } else if (!inAllarme && p.allarmeHR != NORMALE) {
    cambiaStatoAllarme(p, p.allarmeHR, p.tempoInizioAllarmeHR, NORMALE, "HR");
  }
}

void leggiSaturazioneOssigeno(Paziente &p) {
  int raw = analogRead(p.pinPotSpO2);
  p.spo2 = mapFloat(raw, 0, 4095, SPO2_MIN, SPO2_MAX);
  p.spo2 = constrain(p.spo2, SPO2_MIN, SPO2_MAX);
  
  bool inAllarme = (p.spo2 < SPO2_SOGLIA_ALLARME);
  
  if (inAllarme && p.allarmeSpo2 == NORMALE) {
    cambiaStatoAllarme(p, p.allarmeSpo2, p.tempoInizioAllarmeSpo2, ALLARME, "SpO2");
  } else if (!inAllarme && p.allarmeSpo2 != NORMALE) {
    cambiaStatoAllarme(p, p.allarmeSpo2, p.tempoInizioAllarmeSpo2, NORMALE, "SpO2");
  }
}

void leggiPosizione(Paziente &p) {
  p.posizioneSdraiato = (digitalRead(p.pinTilt) == HIGH);
}

void leggiTemperatura(Paziente &p, DHT &dht) {
  float tempRaw = dht.readTemperature();
  
  if (!isnan(tempRaw)) {
    p.tempValida = true;
    p.temperatura = mapFloat(tempRaw, 0, 50, TEMP_MIN, TEMP_MAX);
    p.temperatura = constrain(p.temperatura, TEMP_MIN, TEMP_MAX);
    
    bool inAllarme = (p.temperatura < TEMP_SOGLIA_BASSA || p.temperatura > TEMP_SOGLIA_ALTA);
    
    if (inAllarme && p.allarmeTemp == NORMALE) {
      cambiaStatoAllarme(p, p.allarmeTemp, p.tempoInizioAllarmeTemp, ALLARME, "TEMP");
    } else if (!inAllarme && p.allarmeTemp != NORMALE) {
      cambiaStatoAllarme(p, p.allarmeTemp, p.tempoInizioAllarmeTemp, NORMALE, "TEMP");
    }
  }
}

void suonaMelodia(Paziente &p) {
  unsigned long now = millis();
  int durata = 1000 / DURATE[p.audio.notaCorrente];
  
  if (now - p.audio.ultimaNota >= durata * 1.3) {
    p.audio.ultimaNota = now;
    buzzerTone(p, MELODIA[p.audio.notaCorrente]);
    p.audio.notaCorrente = (p.audio.notaCorrente + 1) % NUM_NOTE;
  }
}

void gestisciAudio(Paziente &p) {
  switch (statoAllarmeGlobale(p)) {
    case NORMALE:
      buzzerStop(p);
      break;
    case ALLARME:
      suonaMelodia(p);
      break;
    case PERSISTENTE:
      buzzerTone(p, FREQ_PERSISTENTE);
      break;
  }
}

void stampaStatoPaziente(Paziente &p) {
  StatoAllarme statoGlobale = statoAllarmeGlobale(p);
  
  Serial.printf("Paziente %d │ Stato: %-8s\n", p.id, STATO_ALLARME_STR[statoGlobale]);
  
  char hrInd = (p.allarmeHR != NORMALE) ? '!' : ' ';
  Serial.printf("%c HR:   %3d BPM  [%-18s]\n", hrInd, p.bpm, HR_STATI[p.livelloHR]);
  
  char spo2Ind = (p.allarmeSpo2 != NORMALE) ? '!' : ' ';
  Serial.printf("%c SpO2: %5.1f%%   [Soglia: %.0f%%]\n", spo2Ind, p.spo2, SPO2_SOGLIA_ALLARME);
  
  char tempInd = (p.allarmeTemp != NORMALE) ? '!' : ' ';
  if (p.tempValida) {
    Serial.printf("%c Temp: %5.1f°C  [Range: %.1f-%.1f°C]\n",
                  tempInd, p.temperatura, TEMP_SOGLIA_BASSA, TEMP_SOGLIA_ALTA);
  } else {
    Serial.printf("  Temp: --.-°C  [In attesa...]\n");
  }
  
  Serial.printf("  Pos:  %s\n", p.posizioneSdraiato ? "Sdraiato" : "In piedi/Seduto");
}

void stampaStatoCompleto() {  
  for (int i = 0; i < 2; i++) {
    stampaStatoPaziente(pazienti[i]);    
    Serial.println("----------------------------------------");
  }
}


void setup() {
  Serial.begin(115200);
  analogReadResolution(12); 
  
  dht1.begin();
  dht2.begin();
  
  inizializzaPaziente(pazienti[0], 1, BTN_HR_P1, POT_SPO2_P1, TILT_P1, DHT_P1, BUZZER_P1);
  inizializzaPaziente(pazienti[1], 2, BTN_HR_P2, POT_SPO2_P2, TILT_P2, DHT_P2, BUZZER_P2);
}

void loop() {
  unsigned long now = millis();
  
  // Bottoni sempre letti (risposta immediata)
  leggiFrequenzaCardiaca(pazienti[0]);
  leggiFrequenzaCardiaca(pazienti[1]);
  
  
  if (now - ultimaLettura >= INTERVALLO_LETTURA) {
    ultimaLettura = now;
    
    leggiSaturazioneOssigeno(pazienti[0]);
    leggiSaturazioneOssigeno(pazienti[1]);
    
    leggiPosizione(pazienti[0]);
    leggiPosizione(pazienti[1]);
  }
  
 
  if (now - ultimaLetturaDHT >= INTERVALLO_DHT) {
    ultimaLetturaDHT = now;
    
    leggiTemperatura(pazienti[0], dht1);
    leggiTemperatura(pazienti[1], dht2);
  }
  
  // Timeout allarmi
  for (int i = 0; i < 2; i++) {
    controllaTimeoutAllarme(pazienti[i], pazienti[i].allarmeHR, 
                            pazienti[i].tempoInizioAllarmeHR, "HR");
    controllaTimeoutAllarme(pazienti[i], pazienti[i].allarmeSpo2, 
                            pazienti[i].tempoInizioAllarmeSpo2, "SpO2");
    controllaTimeoutAllarme(pazienti[i], pazienti[i].allarmeTemp, 
                            pazienti[i].tempoInizioAllarmeTemp, "TEMP");
  }
  
  
  gestisciAudio(pazienti[0]);
  gestisciAudio(pazienti[1]);
  
  
  if (now - ultimaStampa >= INTERVALLO_STAMPA) {
    ultimaStampa = now;
    stampaStatoCompleto();
  }
  
  delay(10);
}