// #include "DHT.h"

// #define DHTPIN 2     
// #define DHTTYPE DHT11

// DHT dht(DHTPIN, DHTTYPE);

// void setup() {
//   Serial.begin(9600);
//   Serial.println("DHT11 Solo Test");
//   dht.begin();
// }

// void loop() {
//   delay(2000); // Wait 2 seconds between measurements
//   float h = dht.readHumidity();
//   float t = dht.readTemperature();

//   if (isnan(h) || isnan(t)) {
//     Serial.println("Failed to read from DHT sensor! Check wiring.");
//   } else {
//     Serial.print("Humidity: "); Serial.print(h);
//     Serial.print("%  Temperature: "); Serial.print(t); Serial.println("°C");
//   }
// }