// #include <Arduino.h>

// const int sensorPin = A0;
// const float dividerRatio = 5.0; 

// // Battery Voltage Thresholds
// const float voltMax = 12.6; // 3 cells * 4.2V
// const float voltMin = 9.0;  // 3 cells * 3.0V (Safety cutoff)

// void setup() {
//   Serial.begin(9600);
//   analogReference(DEFAULT); 
//   Serial.println("--- Battery Monitor with Percentage ---");
// }

// void loop() {
//   int rawValue = analogRead(sensorPin);
  
//   // Calculate Voltage
//   float vOut = (rawValue * 5.0) / 1024.0; 
//   float batteryVoltage = vOut * dividerRatio;

//   // Calculate Percentage
//   // We use constrain to make sure it doesn't show 105% or -5%
//   float batteryPercentage = ((batteryVoltage - voltMin) / (voltMax - voltMin)) * 100.0;
//   batteryPercentage = constrain(batteryPercentage, 0.0, 100.0);

//   // Serial Output
//   Serial.print("Voltage: ");
//   Serial.print(batteryVoltage);
//   Serial.print("V | Percentage: ");
//   Serial.print(batteryPercentage, 1); // 1 decimal place
//   Serial.println("%");

//   delay(2000);
// }