#include <ESP8266WiFi.h>
#include <espnow.h>

// REPLACE WITH THE MAC Address of your receiver
//uint8_t broadcastAddress[] = {0x2C, 0x3A, 0xE8, 0x0F, 0x14, 0x21}; //veia - setar na nova
//uint8_t broadcastAddress[] = {0x2C, 0x3A, 0xE8, 0x0F, 0x14, 0x21}; //nova - setar na veia
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //geral
//uint8_t broadcastAddress[] = {0xDC, 0x4F, 0x22, 0x18, 0x4C, 0x96}; //Paca com X - upar na com12
//uint8_t broadcastAddress[] = {0xDC, 0x4F, 0x22, 0x18, 0x01, 0xA3}; //geral com 2 - upar na com11
const char sendTo = 'X';
const char recFrom = '2';
int baud = 115200;
//int baud = 460800;

//Variaveis de controle de envio
bool flagNewSerial = false; //indica novo caractere serial chegando --------------------------------------
bool ack = false; //indica sucesso na recepcao de uma mensagem e apaga o conteudo ja enviado --------------------------------------
bool notack = false; //indica a falha na recepcao de uma mensagem e nova tentativa --------------------------------------
bool printa = false; //indica se o correio tem mensagem a ser printada ou se eh apenas de ack --------------------------------------
int outChecksum; //armazena valor do checksum da mensagem... talvez nao sera usado --------------------------------------
char outBuffer[230]; //buffer para receber a leitura Serial.readBytes; --------------------------------------

int outIndex = 0; //indice da formacao da mensagem no outCourier.inout[outIndex]
const int maxTry = 10; //numero de tentativas de envio
int lastCheckSum = 0; // E usado no if(ack). Trabalha com o inCourier.mNumber para averiguar o ack;


// Variaveis de leitura de string
String sb[1000]; //serialBuffer - armazena as strings que forem sendo recebidas
bool sbf[1000]; //serialBufferFlag - indica se o indice tem mensagem a ser enviada.
int try2send = 0;
int sbi = 0; //serialBufferIndex - indice dos arrays de sb. Tambem sera usado como message number. Entra no hum da
int readsize = 0; // --------------------------------------------------------------------------------------------------------
uint8_t sbArraySize = 1000; //igual ao tamanho do array sb[sbArraySize]


// Define variables to store incoming readings
float incomingTemp;
float incomingTempLast;
int incomingHum;
int incomingChecksum;
int lastmNumber = -1; //tem que comecar com valor impossivel de se ter no lastmNumber



typedef struct struct_message {
  uint8_t temp; //indica mensagem nova
  //uint8_t hum; //tamanho da mensagem
  uint8_t mSize; //tamanho da mensagem
  uint16_t mNumber; //numero da mensagem
  uint32_t checksum; //soma dos do valor dos bytes da mensagem
  bool ack;
  bool notack;
  bool printa;
  char inout[200]; //string de caracteres da mensagem
} struct_message;
uint8_t espBuffSize = 200; //igual ao valor de inout[espBuffSize]

// Create a struct_message called DHTReadings to hold sensor readings
struct_message outCourier; //DHTReadings;


// Create a struct_message to hold incoming sensor readings
struct_message inCourier; //incomingReadings;

String macAdd = "";

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
//  incomingHum = inCourier.hum;
  incomingChecksum = inCourier.checksum;
  ack = inCourier.ack;
  notack = inCourier.notack;
  printa = inCourier.printa;
 }

unsigned long t0 = millis();

void setup() {
//Init Serial Monitor
//Serial.begin(baud); //DMS
//Serial.begin(19200); //Boia
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
//  outCourier.hum = 0;
  Serial.setTimeout(5);
}


void loop() {

  //// LEITURA DA PORTA SERIAL PARA ENVIO
  if(Serial.available()){
    sb[sbi] = Serial.readString();
//    Serial.print(sb[sbi]);
//    Serial.println(sbi);
//    Serial.println(sb[sbi].length());
    sbf[sbi] = true;
    if(sb[sbi].length() > espBuffSize){ //caso a mensagem seja maior do que o buffer de envio da mensagem
//      Serial.println("sb[sbi].length() > espBuffSize");
      int divi = sb[sbi].length() / espBuffSize;
//      Serial.println("divi");
//      Serial.println(divi);
      String temp = sb[sbi]; //string temporaria para ajudar a splitar a principal
      int aux = 0;
      for(aux = 0; aux <= divi; aux++){
        sb[sbi] = temp.substring(aux*espBuffSize,(aux+1)*espBuffSize);
        sbf[sbi] = true;
//        Serial.print(" sb[sbi] = ");
//        Serial.print(sb[sbi]);
//        Serial.print(" sbi = ");
//        Serial.println(sbi);
        sbi++;
        }
      }
//    Serial.println(sbi);
    sbi++;  
    if(sbi > sbArraySize){
      sbi = 0;
      }
    }
  
  //// MONTAGEM DA MENSAGEM PARA ENVIO
  int outIndexIni = outIndex; //controle de saida do loop. 
  int outIndexNow = 0; //marca mensagem que acabou de tentar ser enviada. E usada no if(try2send > maxTry) para calcelar a mensagem
  
  while(outIndex != outIndexIni - 1){ //usa a variável outIndex para controle de posicoes do array
    
    if (sbf[outIndex]){ //verifica se tem algo a mandar na string
      Serial.println();
      Serial.print("outIndex = ");
      Serial.print(outIndex);
      Serial.print(" try2send = ");
      Serial.print(try2send);
      Serial.print(" sb[outIndex] = ");
      Serial.print(sb[outIndex]);
            
      outIndexNow = outIndex; //Ajuda a cancelar o indice correto da mensagem;
      ///// Montando a mensagem
      outCourier.temp = outCourier.temp + 1; //Indica ao radio receptor que uma nova string chegou;
      unsigned int len = sb[outIndex].length();
      outCourier.mSize = len;
      outCourier.mNumber = outIndex;

      Serial.print(" len = ");
      Serial.println(len);
      //// Copia o conteudo da string na mensagem de ida:
      //sb[outIndex].toCharArray(outCourier.inout,len); //copia a mensagem no buffer para o correio de saida - dando problema
      for(int aux = 0; aux < len; aux++){
        outCourier.inout[aux] = sb[outIndex].charAt(aux);
        }
      //// Calcula o checksum da nova mensagem
      int checksum = 0;
      for (int aux = 0; aux < len ; aux++){
        checksum = checksum + outCourier.inout[aux];
//        Serial.print("outCourier.inout[aux] = ");
//        Serial.print(outCourier.inout[aux]);
//        Serial.print(" checksum = ");
//        Serial.println(checksum,DEC);
        }
      outCourier.checksum = checksum;
      lastCheckSum = checksum;
      outCourier.ack = false; //nao eh uma mensagem de checagem
      outCourier.printa = true; //eh uma mensagem para printar
      
      ///// Enviando a mensagem
      esp_now_send(broadcastAddress, (uint8_t *) &outCourier, sizeof(outCourier));
      delay(10);
      try2send++;
//      Serial.print("outIndex = ");
//      Serial.println(outIndex);
      //Serial.println(millis());
      break; //Manda a mensagem e sai do loop. O outIndex sera alterado caso receba o ack do outro radio.
      }//termina o if (sbf[outIndex])
      
    else{ //Caso a flag de mensagem na String esteja falsa, passa para a próxima
      outIndex++; 
      if(outIndex > sbArraySize){
        outIndex = 0;
        }
      }
     if(outIndex == outIndexIni){//indica que circulou em todas as flags e nao ha mensagem a ser enviada.
      break; 
      }
    }

  ///// ASSINALA FLAG NA message number que receptor recebeu DE SAIDA DEPOIS DE RECEBER O ACK
  if (inCourier.ack){
    if(inCourier.checksum == outCourier.checksum and inCourier.mNumber == outCourier.mNumber)
      {
      Serial.print("ack");
      Serial.print(" inCourier.mNumber = ");
      Serial.println(inCourier.mNumber);
      inCourier.ack = false;
      sb[inCourier.mNumber] = "";
      sbf[inCourier.mNumber] = false; //ajusta flag para mensagem ja enviada
      outIndex = inCourier.mNumber + 1; //incrementa para tentar enviar proxima mensagem
      try2send = 0;
      if(outIndex > sbArraySize){
        outIndex = 0;
        }
      }
    else{
      Serial.print("notack");
      Serial.print(" inCourier.mNumber = ");
      Serial.println(inCourier.mNumber);
        }
    }
    
  if(try2send > maxTry){
    try2send = 0;
    sb[outIndexNow] = "";
    sbf[outIndexNow] = false;
    Serial.print("Cancelando indice = ");
    Serial.println(outIndexNow);
    Serial.println(millis());
    }
    
  ///// CHECA SE HA NOVAS MENSAGENS E SE EH PARA PRINTAR O CORREIO DE CHEGADA
  if (incomingTemp != incomingTempLast and printa) { //outCourier.temp incrementa o incomingTemp - assim indica nova string
    
    incomingTempLast = incomingTemp;
    ////Checagem do tamanho da mensagem
    int checksum = 0;
    for(int aux = 0; aux < inCourier.mSize; aux++){
        checksum = checksum + inCourier.inout[aux];
//        Serial.print("checksum = ");
//        Serial.println(checksum,DEC);
      }
    Serial.println("New Messagem detected +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    Serial.print("inCourier.checksum = ");  
    Serial.print(inCourier.checksum);
    Serial.print(" checksum = ");  
    Serial.print(checksum);
    Serial.print(" inCourier.mSize = ");
    Serial.print(inCourier.mSize);
    Serial.print(" lastmNumber = ");
    Serial.print(lastmNumber);
    Serial.print(" inCourier.mNumber = ");
    Serial.println(inCourier.mNumber);
    //Se o checksum e o tamanho da mensagem bater, printa a mensagem;
    //lastmNumber recorda o nNumber da ultima mensagem e nao printa novamente caso seja igual,
    if(checksum == inCourier.checksum and lastmNumber != inCourier.mNumber){ //o checksum esta ok e a mensagem e nova. Printa tudo.
      Serial.println("The message is:");
      int aux = 0;
      for(aux = 0; aux < inCourier.mSize; aux++){
        Serial.print(inCourier.inout[aux]);
        }
      Serial.println();
      Serial.println("The message end -------------------------------------------------------------------------------------");
      //Configura as flags da mensagem para fazer o emissor parar de mandar as mensagens
      outCourier.ack = true; //Diz para o emissor que recebeu a mensagem
      outCourier.printa = false; //Diz para o emissor que nao eh pra printar o conteudo
      outCourier.checksum = inCourier.checksum;
      outCourier.mNumber = inCourier.mNumber; // informa o numero da mensagem para o emissor cancelar a transmissao
      lastmNumber = inCourier.mNumber;
      //outCourier.temp++; 
      for(int aux = 0; aux <= sizeof(espBuffSize); aux++){
        outCourier.inout[aux] = 0;
        }
      esp_now_send(broadcastAddress, (uint8_t *) &outCourier, sizeof(outCourier));      
      }
    else if(checksum == inCourier.checksum and lastmNumber == inCourier.mNumber){//o checksum veio correto, mas a mensagem veio repetida
      Serial.print("Please cancel transmission of mNumber = ");
      Serial.print(inCourier.mNumber);
      Serial.println("--------------------------------------------------------------");
      outCourier.ack = true; //Diz para o emissor que recebeu a mensagem
      outCourier.printa = false;
      outCourier.checksum = inCourier.checksum;
      outCourier.mNumber = inCourier.mNumber; // informa o numero da mensagem novamente para o emissor cancelar a transmissao
      //outCourier.temp++;      
      for(int aux = 0; aux <= sizeof(espBuffSize); aux++){
        outCourier.inout[aux] = 0;
        }
      esp_now_send(broadcastAddress, (uint8_t *) &outCourier, sizeof(outCourier));      
      }
    else if(checksum =! inCourier.checksum){
      Serial.print("Checksum Error. Waiting for next transmission ");
      Serial.print(inCourier.mNumber);
      Serial.println("--------------------------------------------------------------");
      }
    else if(lastmNumber == inCourier.mNumber){ //O checksum veio incorreto e a ultima mensagem ainda nao foi printada
      Serial.print("Erro do tipo lastmNumber == inCourier.mNumber = ");
      Serial.print(inCourier.mNumber);
      Serial.println("--------------------------------------------------------------");
      }
    else if(lastmNumber == inCourier.mNumber){ //O checksum veio incorreto e a ultima mensagem ainda nao foi printada
      Serial.print("Erro do tipo lastmNumber == inCourier.mNumber = ");
      Serial.print(inCourier.mNumber);
      Serial.println("--------------------------------------------------------------");
      }
    else{
      Serial.print("Erro desconhecido. inCourier.mNumber = ");
      Serial.print(inCourier.mNumber);
      Serial.println("--------------------------------------------------------------");
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
