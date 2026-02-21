#include <Arduino.h>
#include <EasyNextionLibrary.h>

EasyNex nex(Serial);

void setup() {
  // flamme : pic0 off pic1 on
  // gas : pic3 off pic2 on
  // temp : pic5 off pic4 on
  // humidity : pic7 off pic6 on
  // ventilateur : pic10 off pic9 on
  // pompe : pic12 off pic11 on
  Serial.begin(9600);
  delay(500);
  
  Serial.println("\n\nSystem Starting...");
  Serial.println("Initializing Nextion Display...");
  
  // Initialize the Nextion display
  nex.begin();
  delay(500);

  Serial.println("Nextion initialized");
  Serial.println("Waiting for button clicks...\n");
}


void loop() {
  // Listen for incoming button clicks from Nextion
  nex.writeNum("nGas.val", 100);
  nex.writeNum("pGas.pic", 2);
  nex.writeNum("pFlame.pic", 1);
  nex.writeNum("nTemp.val", 37);
  nex.writeNum("pTemp.pic", 4);  
  nex.writeNum("nHumidity.val", 85);
  nex.writeNum("pHumidity.pic", 6);  
  nex.writeNum("pPompe.pic",11);
  nex.writeNum("pFan.pic", 9);

  delay(2000);

  nex.writeNum("nGas.val", 0);
  nex.writeNum("pGas.pic", 3);
  nex.writeNum("pFlame.pic", 0);
  nex.writeNum("nTemp.val", 10);
  nex.writeNum("pTemp.pic", 5);  
  nex.writeNum("nHumidity.val", 20);
  nex.writeNum("pHumidity.pic", 7);  
  nex.writeNum("pPompe.pic", 12);
  nex.writeNum("pFan.pic", 10);

  delay(2000);
}