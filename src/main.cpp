#include <Arduino.h>
#include <EasyNextionLibrary.h>
#include <DHT.h>
#include <EEPROM.h>

// ---------- User Configuration ----------
#define RELAY_ON    LOW   
#define RELAY_OFF   HIGH  

// ---------- Pin Definitions ----------
#define DHTPIN        8       
#define DHTTYPE      DHT11
#define MQ6_PIN      A3      
#define FLAME_PIN    3        
#define RELAY_FAN    4        
#define RELAY_PUMP   5        
#define BUZZER_PIN   6        
#define LED_ALARM    7        

EasyNex nex(Serial); 
DHT dht(DHTPIN, DHTTYPE);

// ---------- Threshold Variables ----------
float sTemp = 30.0;   
float sHumidity = 60.0;   
int sGas = 300;

// ---------- Alarm Timing Variables ----------
unsigned long buzzerMillis = 0;
bool buzzerState = false;

unsigned long previousMillis = 0;
const unsigned long sensorInterval = 1000; 

// ---------- EEPROM Functions ----------
void saveThresholdsToEEPROM() {
  EEPROM.put(0, sTemp);
  EEPROM.put(sizeof(sTemp), sHumidity);
  EEPROM.put(sizeof(sTemp) + sizeof(sHumidity), sGas);
}

void loadThresholdsFromEEPROM() {
  EEPROM.get(0, sTemp);
  EEPROM.get(sizeof(sTemp), sHumidity);
  EEPROM.get(sizeof(sTemp) + sizeof(sHumidity), sGas);
  
  if (isnan(sTemp) || sTemp < 0 || sTemp > 100) sTemp = 30.0;
  if (isnan(sHumidity) || sHumidity < 0 || sHumidity > 100) sHumidity = 60.0;
  if (sGas < 0 || sGas > 1023) sGas = 300;
}

// ---------- HMI Functions ----------
void syncNextionWithEEPROM() {
  nex.writeNum("sTemp.val", (uint32_t)sTemp);
  nex.writeNum("sHumidity.val", (uint32_t)sHumidity);
  nex.writeNum("sGas.val", (uint32_t)sGas);
  
  nex.writeNum("nnTemp.val", (uint32_t)sTemp);
  nex.writeNum("nnHumidity.val", (uint32_t)sHumidity);
  nex.writeNum("nnGas.val", (uint32_t)sGas);
}

void refreshNextion(float temperature, float humidity, int gasValue, bool flameDetected) {
  nex.writeNum("nTemp.val", (uint32_t)temperature);
  nex.writeNum("nHumidity.val", (uint32_t)humidity);
  nex.writeNum("nGas.val", (uint32_t)gasValue);

  nex.writeNum("pFlame.pic", flameDetected ? 0 : 1);
  nex.writeNum("pGas.pic", (gasValue > sGas) ? 11 : 12);
  nex.writeNum("pTemp.pic", (temperature > sTemp) ? 3 : 2);
  nex.writeNum("pHumidity.pic", (humidity > sHumidity) ? 5 : 4);

  bool fanState = (digitalRead(RELAY_FAN) == RELAY_ON);
  nex.writeNum("pFan.pic", fanState ? 8 : 7); 
  
  bool pumpState = (digitalRead(RELAY_PUMP) == RELAY_ON);
  nex.writeNum("pPompe.pic", pumpState ? 9 : 10);
}

// ---------- Logic Control ----------
void checkThresholds(float temperature, float humidity, int gasValue, bool flameDetected) {
  bool criticalAlert = (flameDetected || gasValue > sGas);
  
  if (criticalAlert) {
    digitalWrite(LED_ALARM, HIGH);
    digitalWrite(RELAY_PUMP, RELAY_ON); 

    // --- WAR ALERT BUZZER (Non-blocking Pulse) ---
    // Pulses every 150ms for a high-intensity "War" sound
    unsigned long currentMillis = millis();
    if (currentMillis - buzzerMillis >= 150) { 
      buzzerMillis = currentMillis;
      buzzerState = !buzzerState; 
      digitalWrite(BUZZER_PIN, buzzerState);
    }
  } else {
    // Normal Mode: Buzzer OFF
    digitalWrite(BUZZER_PIN, LOW); 
    digitalWrite(LED_ALARM, LOW);
    
    // Automatic Cooling/Heat Management
    if (!isnan(temperature)) {
      if (temperature > (sTemp + 10)) digitalWrite(RELAY_PUMP, RELAY_ON); 
      else if (temperature < (sTemp + 8)) digitalWrite(RELAY_PUMP, RELAY_OFF);
    } else {
      digitalWrite(RELAY_PUMP, RELAY_OFF);
    }
  }

  // Fan Logic
  if (!isnan(temperature) && !isnan(humidity)) {
    if (temperature > sTemp || humidity > sHumidity) digitalWrite(RELAY_FAN, RELAY_ON);
    else if (temperature < (sTemp - 1) && humidity < (sHumidity - 1)) digitalWrite(RELAY_FAN, RELAY_OFF);
  } else {
    digitalWrite(RELAY_FAN, RELAY_OFF);
  }
}

// ---------- Nextion Triggers ----------

// SAVE BUTTON (Trigger 0)
void trigger0() {
  delay(100);  
  uint32_t nt = nex.readNumber("sTemp.val");
  uint32_t nh = nex.readNumber("sHumidity.val");
  uint32_t ng = nex.readNumber("sGas.val");

  if (nt != 777777 && nh != 777777 && ng != 777777) {
    sTemp = (float)nt;
    sHumidity = (float)nh;
    sGas = (int)ng;
    saveThresholdsToEEPROM();
    
    // Success: 1 Long Beep
    digitalWrite(BUZZER_PIN, HIGH); delay(1000); digitalWrite(BUZZER_PIN, LOW);
    syncNextionWithEEPROM();  
  } else {
    // Failure: 3 Short Beeps
    for(int i=0; i<3; i++){
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); delay(100);
    }
  }
}

// HANDSHAKE REQUEST (Trigger 1)
void trigger1() {
  syncNextionWithEEPROM();
  // Handshake: 2 Short Beeps
  for(int i=0; i<2; i++){
    digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); delay(100);
  }
}

// ---------- Main Setup & Loop ----------
void setup() {
  nex.begin(9600);
  dht.begin();
  
  pinMode(FLAME_PIN, INPUT);
  pinMode(MQ6_PIN, INPUT);
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_ALARM, OUTPUT);

  digitalWrite(RELAY_FAN, RELAY_OFF);
  digitalWrite(RELAY_PUMP, RELAY_OFF);

  loadThresholdsFromEEPROM();
  syncNextionWithEEPROM();
}

void loop() {
  nex.NextionListen();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= sensorInterval) {
    previousMillis = currentMillis;

    bool flameDetected = (digitalRead(FLAME_PIN) == LOW); 
    int gasValue = analogRead(MQ6_PIN);
    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();

    checkThresholds(temperature, humidity, gasValue, flameDetected);

    float t_disp = isnan(temperature) ? 0 : temperature;
    float h_disp = isnan(humidity) ? 0 : humidity;
    
    refreshNextion(t_disp, h_disp, gasValue, flameDetected);
  }
}