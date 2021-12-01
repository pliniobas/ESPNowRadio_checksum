#include <ESP8266WiFi.h>
#include <espnow.h>

// REPLACE WITH THE MAC Address of your receiver
//uint8_t broadcastAddress[] = {0x2C, 0x3A, 0xE8, 0x0F, 0x14, 0x21}; //veia - setar na nova
//uint8_t broadcastAddress[] = {0x2C, 0x3A, 0xE8, 0x0F, 0x14, 0x21}; //nova - setar na veia
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //geral

// Variable to store if sending data was successful
String success;

bool flagDataFail = false;

// Define variables to store DHT readings to be sent
float temperature;
float humidity;
String outString;

// Define variables to store incoming readings
float incomingTemp;
float incomingTempLast;
float incomingHum;


typedef struct struct_message {
  float temp;
  float hum;
  char inout[240];
} struct_message;

// Create a struct_message called DHTReadings to hold sensor readings
struct_message outCourier; //DHTReadings;


// Create a struct_message to hold incoming sensor readings
struct_message inCourier; //incomingReadings;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {

  if (sendStatus == 0) {
    //    Serial.print("Last Packet Send Status: ");
    //    Serial.println("Delivery success");
  }
  else {
    Serial.print("Last Packet Send Status: ");
    Serial.println("Delivery fail");
    flagDataFail = true;
  }
}

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&inCourier, incomingData, sizeof(inCourier));
  //Serial.print("Bytes received: ");
  //Serial.println(len);
  incomingTemp = inCourier.temp;
  incomingHum = inCourier.hum;
 }


String macAdd = "";
unsigned long t0 = millis();

void setup() {
  // Init Serial Monitor
  Serial.begin(460800); //DMS
//  Serial.begin(19200); //Boia
   

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  macAdd = WiFi.macAddress();
  WiFi.disconnect();
  Serial.println("Endereco MAC ");
  Serial.println(macAdd);
  delay(3000);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  outCourier.temp = 0;
  outCourier.hum = 0;
  Serial.setTimeout(1);
 
}


void loop() {

  // Send message via ESP-NOW
  if(Serial.available()){
    int readsize = Serial.readBytes(outCourier.inout,sizeof(outCourier.inout));//outCourier.inout recebe a string
//    for(int aux = 0; aux <= readsize; aux++){
//      if(outCourier.inout[aux] == '\r'){
//        outCourier.inout[aux] = 0;
//        } 
//      }
    outCourier.temp = outCourier.temp + 1; //Indica ao radio receptor que uma nova string chegou;
    esp_now_send(broadcastAddress, (uint8_t *) &outCourier, sizeof(outCourier));
      for(int aux = 0; aux <= readsize; aux++){
        outCourier.inout[aux] = 0;
      }
    }

//  delay(5);
//  if(flagDataFail){
//    flagDataFail = false;
//    delay(100);
//    }
  
  //TESTE DE ENVIO A JATO
//  String temp = String(millis());
//  Serial.println(millis());
//  String a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
//  temp = temp + " " + a + "\n";
//  temp.toCharArray(outCourier.inout,sizeof(outCourier.inout));
//  outCourier.temp = outCourier.temp + 1;
//  esp_now_send(broadcastAddress, (uint8_t *) &outCourier, sizeof(outCourier));
//      for(int aux = 0; aux <= sizeof(outCourier.inout); aux++){
//        outCourier.inout[aux] = 0;
//      }
  ///////////////////////////
  
  // Sem uso no momento
  if (millis() > (t0 + 1000)) {
    t0 = millis();
    }
    
  // Print incoming readings
  if (incomingTemp != incomingTempLast) { //outCourier.temp incrementa o incomingTemp - assim indica nova string
    incomingTempLast = incomingTemp;
    Serial.print(inCourier.inout);
    for(int aux = 0; aux <= sizeof(inCourier.inout);aux++){
      inCourier.inout[aux] = 0;
      }
  }
}
