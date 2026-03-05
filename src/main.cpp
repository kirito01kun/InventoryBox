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
#define BATTERY_PIN  A0  // Battery Voltage Sensor Pin

// ---------- Battery Constants ----------
const float voltMax = 12.6; // 100% (4.2V * 3)
const float voltMin = 9.0;  // 0% (3.0V * 3)
const float dividerRatio = 5.0; 
unsigned long prevBatMillis = 0;
const unsigned long batteryInterval = 60000; // Update battery every 60 seconds
uint32_t lastStablePct = 0; 

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



// --- Updated refreshNextion Function ---
void refreshNextion(float temperature, float humidity, int gasValue, bool flameDetected) {
  
  unsigned long currentMillis = millis();

  // ONLY update battery if 60 seconds have passed OR if it's the first run
  if (currentMillis - prevBatMillis >= batteryInterval || prevBatMillis == 0) {
    prevBatMillis = currentMillis;

    // Smooth Sampling: Take 10 readings and average them
    long runningTotal = 0;
    for(int i=0; i<10; i++) {
      runningTotal += analogRead(BATTERY_PIN);
      delay(5); 
    }
    int rawBat = runningTotal / 10;

    float vOut = (rawBat * 5.0) / 1024.0;
    float batteryVoltage = vOut * dividerRatio;
    float batteryPct = ((batteryVoltage - voltMin) / (voltMax - voltMin)) * 100.0;
    lastStablePct = (uint32_t)constrain(batteryPct, 0, 100);
  }

  // --- Send Data to Nextion ---
  nex.writeNum("nTemp.val", (uint32_t)temperature);
  nex.writeNum("nHumidity.val", (uint32_t)humidity);
  nex.writeNum("nGas.val", (uint32_t)gasValue);
  nex.writeNum("nVoltage.val", lastStablePct); // Use the last stable average

  nex.writeNum("pFlame.pic", flameDetected ? 0 : 1);
  nex.writeNum("pGas.pic", (gasValue > sGas) ? 11 : 12);
  nex.writeNum("pTemp.pic", (temperature > sTemp) ? 3 : 2);
  nex.writeNum("pHumidity.pic", (humidity > sHumidity) ? 5 : 4);

  bool fanState = (digitalRead(RELAY_FAN) == RELAY_ON);
  nex.writeNum("pFan.pic", fanState ? 7 : 8); 
  
  bool pumpState = (digitalRead(RELAY_PUMP) == RELAY_ON);
  nex.writeNum("pPompe.pic", pumpState ? 9 : 10);
}

// ---------- Logic Control ----------
void checkThresholds(float temperature, float humidity, int gasValue, bool flameDetected) {
  bool gasAlert = (gasValue > sGas);
  bool criticalAlert = (flameDetected || gasAlert);
  
  // --- 1. ALARM HARDWARE (Buzzer & LED) ---
  if (criticalAlert) {
    digitalWrite(LED_ALARM, HIGH);
    // Non-blocking Pulse for Buzzer
    unsigned long currentMillis = millis();
    if (currentMillis - buzzerMillis >= 150) { 
      buzzerMillis = currentMillis;
      buzzerState = !buzzerState; 
      digitalWrite(BUZZER_PIN, buzzerState);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW); 
    digitalWrite(LED_ALARM, LOW);
  }

  // --- 2. PUMP LOGIC (Flame or High Heat) ---
  if (flameDetected) {
    digitalWrite(RELAY_PUMP, RELAY_ON); // Priority: Fire suppression
  } else {
    // Normal Automatic Heat Management for Pump
    if (!isnan(temperature)) {
      if (temperature > (sTemp + 10)) digitalWrite(RELAY_PUMP, RELAY_ON); 
      else if (temperature < (sTemp + 8)) digitalWrite(RELAY_PUMP, RELAY_OFF);
    } else {
      digitalWrite(RELAY_PUMP, RELAY_OFF);
    }
  }

  // --- 3. FAN LOGIC (Gas or High Temp/Humidity) ---
  if (gasAlert) {
    digitalWrite(RELAY_FAN, RELAY_ON); // Priority: Exhausting Gas
  } else {
    // Normal Ventilation Logic
    if (!isnan(temperature) && !isnan(humidity)) {
      if (temperature > sTemp || humidity > sHumidity) digitalWrite(RELAY_FAN, RELAY_ON);
      else if (temperature < (sTemp - 1) && humidity < (sHumidity - 1)) digitalWrite(RELAY_FAN, RELAY_OFF);
    } else {
      digitalWrite(RELAY_FAN, RELAY_OFF);
    }
  }
}

// ---------- Nextion Triggers ----------
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
    digitalWrite(BUZZER_PIN, HIGH); delay(1000); digitalWrite(BUZZER_PIN, LOW);
    syncNextionWithEEPROM();  
  } else {
    for(int i=0; i<3; i++){
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); delay(100);
    }
  }
}

void trigger1() {
  syncNextionWithEEPROM();
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
  pinMode(BATTERY_PIN, INPUT);
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