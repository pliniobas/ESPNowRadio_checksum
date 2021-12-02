#include <ESP8266WiFi.h>
#include <espnow.h>

// REPLACE WITH THE MAC Address of your receiver
//uint8_t broadcastAddress[] = {0x2C, 0x3A, 0xE8, 0x0F, 0x14, 0x21}; //veia - setar na nova
//uint8_t broadcastAddress[] = {0x2C, 0x3A, 0xE8, 0x0F, 0x14, 0x21}; //nova - setar na veia
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //geral
//uint8_t broadcastAddress[] = {0xDC, 0x4F, 0x22, 0x18, 0x4C, 0x96}; //Paca com X - upar na com12
//uint8_t broadcastAddress[] = {0xDC, 0x4F, 0x22, 0x18, 0x01, 0xA3}; //geral com 2 - upar na com11
int baud = 115200;
//int baud = 460800;



// Variable to store if sending data was successful
//String success;

bool flagNewSerial = false; //indica novo caractere serial chegando
bool ack = false; //indica sucesso na recepcao de uma mensagem e apaga o conteudo ja enviado
bool notack = false; //indica a falha na recepcao de uma mensagem e nova tentativa
bool printa = false; //indica se o correio tem mensagem a ser printada ou se eh apenas de ack 
int outChecksum; //armazena valor do checksum da mensagem... talvez nao sera usado
char outBuffer[230]; //buffer para receber a leitura Serial.readBytes;
int outIndex = 0; //indice da formacao da mensagem no outCourier.inout[outIndex]
int readsize = 0; //tamanho da mensagem recebida na Serial.readBytes


// Define variables to store incoming readings
float incomingTemp;
float incomingTempLast;
int incomingHum;
int incomingChecksum;


typedef struct struct_message {
  int temp; //indica mensagem nova
  uint8_t hum; //tamanho da mensagem
  int checksum; //soma dos do valor dos bytes da mensagem
  bool ack;
  bool notack;
  bool printa;
  char inout[230]; //string de caracteres da mensagem
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
  }
}

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&inCourier, incomingData, sizeof(inCourier));
  incomingTemp = inCourier.temp;
  incomingHum = inCourier.hum;
  incomingChecksum = inCourier.checksum;
  ack = inCourier.ack;
  notack = inCourier.notack;
  printa = inCourier.printa;
 }


String macAdd = "";
unsigned long t0 = millis();

void setup() {
  // Init Serial Monitor
 // Serial.begin(baud); //DMS
//  Serial.begin(19200); //Boia
  Serial.begin(115200);
   

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  macAdd = WiFi.macAddress();
  WiFi.disconnect();
  Serial.println("\nEndereco MAC ");
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
  Serial.setTimeout(5);
 
}


void loop() {

  //// LEITURA DA PORTA SERIAL PARA ENVIO
  if(Serial.available()){
    readsize = Serial.readBytes(outBuffer,sizeof(outBuffer));//outCourier.inout recebe a string
//    Serial.println("readsize");
//    Serial.println(readsize);
//    Serial.println("outBuffer");
//    Serial.println(outBuffer);
    flagNewSerial = true;
      }
  
  //// MONTAGEM DA MENSAGEM PARA ENVIO
  if (notack){
    Serial.println("notack");
    }
    
  if (flagNewSerial == true or notack == true){
    notack = false;
    flagNewSerial = false;
    ///// Montando a mensagem
    outCourier.temp = outCourier.temp + 1; //Indica ao radio receptor que uma nova string chegou;
    outCourier.hum = readsize;
    //// Copia o conteudo da string na mensagem de ida:
    for(int aux = 0; aux <= readsize; aux++){
      outCourier.inout[outIndex] = outBuffer[aux]; //copia a mensagem no buffer para o correio de saida
      outIndex++;
      if (outIndex > 240){
        Serial.println("OVERBUFF");
        outIndex = 0;
        }
      }
    readsize = 0; //zera o readsize caso passe por aqui de novo por um notack. Evita rescrita em caso de notack
    //// Calcula o checksum da nova mensagem
    int checksum = 0;
    for (int aux = 0; aux <= sizeof(outCourier.inout); aux++){
      checksum += outCourier.inout[aux];
      }
    outCourier.checksum = checksum;
    ////
    outCourier.ack = false; //nao eh uma mensagem de checagem
    outCourier.notack = false; //nao eh uma mensagem de checagem
    outCourier.printa = true; //eh uma mensagem para printar
    
    ///// Enviando a mensagem
    
    esp_now_send(broadcastAddress, (uint8_t *) &outCourier, sizeof(outCourier));
    //Serial.println(outCourier.inout);
    //Serial.println(outCourier.temp);
    //Serial.println(outCourier.hum);
    //Serial.println(outCourier.checksum);
    //Serial.println(outCourier.ack);
    //Serial.println(outCourier.notack);
    //Serial.println(outCourier.printa);
    }
      
  ///// APAGA A MENSAGEM NO CORREIO DE SAIDA DEPOIS DE RECEBER O ACK
  if (ack){
    //Serial.println("if(ack)");
    ack = false;
    notack = false;
    outIndex = 0; //zera o indice da variavel outCourier.inout
    for(int aux = 0; aux <= sizeof(outCourier.inout); aux++){
      outCourier.inout[aux] = 0;
      }
    }
    
  ///// CHECA SE HA NOVAS MENSAGENS E SE EH PARA PRINTAR O CORREIO DE CHEGADA
  if (incomingTemp != incomingTempLast and printa) { //outCourier.temp incrementa o incomingTemp - assim indica nova string
    //Serial.println("incomingTempLast = incomingTemp;");
    incomingTempLast = incomingTemp;
  ////Checagem do tamanho da mensagem
    int checksum = 0;
    for(int aux = 0; aux <= sizeof(inCourier.inout); aux++){
      if(inCourier.inout[aux] != 0){ 
        checksum = checksum + inCourier.inout[aux];
        }
      }
//    Serial.println("inCourier.checksum");  
//    Serial.println(inCourier.checksum);
//    Serial.println("checksum");  
//    Serial.println(checksum);
//    Serial.println("incomingHum");
//    Serial.println(incomingHum);
    //Se o checksum e o tamanho da mensagem bater, printa a mensagem;
    if(checksum == inCourier.checksum){
//      Serial.println("inCourier.inout");
//      char temp[incomingHum + 1];
      int aux = 0;
      for(aux = 0; aux < incomingHum; aux++){
//        temp[aux] = inCourier.inout[aux];
        Serial.print(inCourier.inout[aux]);
        }
//      aux++;
//      temp[aux] = 0;
//      Serial.print(temp);
      outCourier.ack = true;
      outCourier.notack = false;
      outCourier.printa = false;
      }
    else{
      Serial.println("outCourier.notack = true;");
      outCourier.ack = false;
      outCourier.notack = true;
      outCourier.printa = false;
      }
      char tempstring[sizeof(outCourier.inout)];
      for(int aux = 0; aux <= sizeof(tempstring); aux++){
        tempstring[aux] = outCourier.inout[aux];
        outCourier.inout[aux] = 0;
        }
      esp_now_send(broadcastAddress, (uint8_t *) &outCourier, sizeof(outCourier));      
      for(int aux = 0; aux <= sizeof(tempstring); aux++){
         outCourier.inout[aux] = tempstring[aux];
        }       
  }

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
}
