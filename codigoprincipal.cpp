//Sckech Arduino com SENSOR BME BSEC I2C, ThingSpeak, AWS iOT MQTT, TELEGRAM e controle de Risco Inicial;

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <DHT.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <ThingSpeak.h>
#include <UniversalTelegramBot.h>
#include <bsec.h>
#include <EEPROM.h>
#include "acerts.h"
#include "pwifi.h"


//Definição dos pinos
//#define bmeCSPin 15 // Sensor BME680 SPI CS
//#define ...Pin 2
#define dhtPin 4    // Sensor DHT11
//#define ...Pin 16
//#define ...Pin 17
//#define ...Pin 5 
//#define bmeSDKPin 18 // Sensor BME680 SPI SCL
//#define bmeMISOPin 19 // Sensor BME680 SPI SDO
//#define bme1Pin 21  // Sensor BME680 I2C pino SDA
//#define ...Pin 3
//#define ...Pin 1
//#define bme2Pin 22  // Sensor BME680 I2C pino SCL
//#define bmeMOSIPin 23 // Sensor BME680 SPI SDA
//#define ...Pin 13
#define ledBPin 12
#define mq2dPin 14  // Sensor MQ2 pino DO
#define ledGPin 27
//#define ...Pin 26  
#define ledRPin 25   // Buzzer
#define mq2aPin 33  // Sensor MQ2 pino AO
#define buzPin 32   // Led
//#define ...Pin 35
//#define ...Pin 34
//#define ...Pin 39
//#define ...Pin 36

const uint8_t bsec_config_iaq[] = { #include "config/generic_33v_3s_4d/bsec_iaq.txt"};
#define PERIODO_SALVA_ESTADO UINT32_C(360 * 60 * 1000) //360 minutors ou 4 vezes ao dia;

#define MODULO(x) ((x)>=0?(x):-(x))

DHT dht(dhtPin, DHT11);
Bsec iaqSensor;
WiFiClient clientTS;
WiFiClientSecure net = WiFiClientSecure();
WiFiClientSecure netTG = WiFiClientSecure();
MQTTClient client = MQTTClient(512);
UniversalTelegramBot bot(CHAVE_TELEGRAM, netTG);

const uint32_t DEVICE_ID_ATUAL = ESP.getEfuseMac();

uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
uint16_t stateUpdateCounter = 0;

int riscoPredio = 0;
int riscoAmbiente = 2;

float umidadeAtualDHT = 0.0;
float tempAtualDHT = 0.0;
float calorAtualDHT = 0.0;

int gasAtualMQa = 0;
uint8_t gasAtualMQd = 0;

uint8_t hallAtual = 0;
bool presencaAtual = 1;

float tempAtualBME = 0.0;
float umidadeAtualBME = 0.0;
float pressaoAtualBME = 0.0;
float gasAtualBME = 0.0;
float altitudeAtualBME = 0.0;
float iaqAtualBME = 0;
uint8_t iaqAcAtualBME = 0;
uint8_t gasAcAtualBME = 0;

unsigned long timer = millis();
unsigned long lastTime[4]{ 3000, 10000, 0, 300000 };
unsigned long timerDelay[4]{ 3000, 10000, 100000, 300000 };

String CMD_LIST[] = {"Led R","Led G","Led B","Buz On","Buz Off","Status",};

float tempInicialDHT = 0.0;
float tempInicialBME = 0.0;
int gasInicialMQa = 0.0;
float gasInicialBME = 0.0;
float iaqInicialBME = 0.0;
float umidadeInicialBME = 0.0;
float umidadeInicialDHT = 0.0;
 
int num_mensagens_recebidas_telegram = 0;
String resposta_msg_recebida;
String risco_msg;


void configuraBME(void);
void checkSituacaoBME(void);
void carregaEstadoBME(void);
void atualizaEstadoBME(void);
void getBME(void);
void getDHT11(void);
void getMQ2(void);
void getHALL(void);
void verificaRisco(void);
String mensagemRetorno(void);
void inicializaPin(void);
void configuraDHT(void);
void configuraWifi(void);
void restartESP(void);
void configuraTS(void);
void configuraAWS(void);
void reconectaAWS(void);
void recebeAWS(String &topic, String &payload);
void enviaAWS(void);
void enviaTS(void);
void consultaTelegram(void);
String trataMensagemTelegram(String msg_recebida);
void ledOn (String tipo);


void configuraBME(){
  EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1);
  Wire.begin();
  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  checkSituacaoBME();

  iaqSensor.setConfig(bsec_config_iaq);
  
  carregaEstadoBME();  
  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };
  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkSituacaoBME();
}
void checkSituacaoBME(void){
 String output;
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = String(iaqSensor.status);
      Serial.printf("\nFalha na biblioteca BSEC!! código: %s", output);
        Serial.println("Erro BSEC");
        ledOn ("R");
        return;
    } else {
       output = String(iaqSensor.status);
       Serial.printf("\nAlerta na biblioteca BSEC!! código: %s", output);
       ledOn ("R");
      }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
       output = String(iaqSensor.bme680Status);
       Serial.printf("\nFalha na leitura do sensor BME680!! código: %s", output);
       ledOn ("R");
       tempAtualBME = 0.0;
       pressaoAtualBME = 0.0;
       umidadeAtualBME = 0.0;
       gasAtualBME = 0.0;
       iaqAtualBME = 0;
       iaqAcAtualBME = 0;
       return;
    } else {
       output = String(iaqSensor.bme680Status);
       Serial.printf("\nAlerta no sensor BME680!! codigo: %s", output);
       ledOn ("R");
      }
  }
 iaqSensor.status = BSEC_OK;
}

void carregaEstadoBME(){
 if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {
    // Existing state in EEPROM
    Serial.println("Reading state from EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
      bsecState[i] = EEPROM.read(i + 1);
      Serial.print(bsecState[i], HEX);
    }

    iaqSensor.setState(bsecState);
    checkSituacaoBME();
  } else {
    // Erase the EEPROM with zeroes
    Serial.println("Erasing EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE + 1; i++)
      EEPROM.write(i, 0);

    EEPROM.commit();
  }
}
void atualizaEstadoBME(){
  bool update = false;
  if (stateUpdateCounter == 0) {
    if (iaqSensor.iaqAccuracy >= 3) {
      update = true;
      stateUpdateCounter++;
    }
  } else {
    if ((stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update) {
    iaqSensor.getState(bsecState);
    checkSituacaoBME();

    Serial.println("Writing state to EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE ; i++) {
      EEPROM.write(i + 1, bsecState[i]);
      Serial.println(bsecState[i], HEX);
    }

    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
    EEPROM.commit();
  }
}


void getBME(){
   tempAtualBME = iaqSensor.temperature;
   pressaoAtualBME = iaqSensor.pressure;
   umidadeAtualBME = iaqSensor.humidity;
   gasAtualBME = iaqSensor.gasResistance;
   iaqAtualBME = iaqSensor.iaq;
   iaqAcAtualBME = iaqSensor.iaqAccuracy;
   gasAcAtualBME = iaqSensor.co2Accuracy + iaqSensor.breathVocAccuracy;
   float atmospheric = pressaoAtualBME / 100.0F;
   altitudeAtualBME = 44330.0 * (1.0 - pow(atmospheric / 1013.25, 0.1903));
   atualizaEstadoBME();
}

void getDHT11(){
  umidadeAtualDHT = dht.readHumidity();
  tempAtualDHT = dht.readTemperature();
  if (isnan(umidadeAtualDHT) || isnan(tempAtualDHT)){
   Serial.printf("\nFalha na leitura do sensor DHT!");
   ledOn ("R");
   umidadeAtualDHT = 0.0;
   tempAtualDHT = 0.0;
   calorAtualDHT = 0.0;
   return;
  }
 calorAtualDHT = dht.computeHeatIndex(tempAtualDHT, umidadeAtualDHT, false);
}

void getMQ2(){
  gasAtualMQa = analogRead(mq2aPin);
  gasAtualMQd = digitalRead(mq2dPin);
   if (gasAtualMQa < 500 && gasAtualMQd == 0) {
    Serial.printf("\nFalha na leitura do sensor MQ2!");
    ledOn ("R");
    gasAtualMQa = 0;
    gasAtualMQd = 0;
    return;
  } 
}

void getHALL(){
  hallAtual = hallRead();
   if (hallAtual == 0) {
    Serial.printf("\nFalha na leitura do sensor hall!");
    ledOn ("R");
    return;
  } 
}

void verificaRisco(){
   riscoAmbiente = 2;
   risco_msg = "";
 
   float tempVarDHT = MODULO(tempInicialDHT - tempAtualDHT);
   float tempVarBME = MODULO(tempInicialBME - tempAtualBME);
   int gasVarMQa = MODULO(gasInicialMQa - gasAtualMQa);
   float gasVarBME = MODULO(gasInicialBME - gasAtualBME);
   int iaqVarBME = MODULO(iaqInicialBME - iaqAtualBME);
 
  if (tempVarDHT != 0 && tempVarBME != 0 ){
      if ((((tempVarDHT / tempAtualDHT) * 100) > 20) || (((tempVarBME / tempAtualBME) * 100) > 20)){
         riscoAmbiente = riscoAmbiente * 3;
         risco_msg +="Var temp>20; ";
      }
   }
  if (gasVarMQa != 0 && gasVarBME != 0 && iaqAcAtualBME >0){
      if ((((gasVarMQa / gasAtualMQa) * 100) > 20) || ((((gasVarBME / gasAtualBME) * 100) > 20) && iaqAcAtualBME > 0)){
         riscoAmbiente = riscoAmbiente * 5;
         risco_msg +="Var Gas>20; ";
      }
  } 
  if (iaqVarBME != 0){
    if ((((iaqVarBME / iaqAtualBME) * 100) > 40) && iaqAcAtualBME >0){
      riscoAmbiente = riscoAmbiente * 3;
      risco_msg +="Var iaq>40; ";
    }
  } 
  if (tempAtualDHT > 35.00 || tempAtualBME > 35.00){
     riscoAmbiente = riscoAmbiente * 3;
     risco_msg +="Temp > 35; ";
   }
  
  if ((umidadeAtualDHT < 20.00 && umidadeAtualDHT != 0.0) || (umidadeAtualBME < 20.00 && umidadeAtualBME != 0.0)) {
    riscoAmbiente = riscoAmbiente * 2; 
    risco_msg +="Umid  < 20; ";
  }
  
  if ((gasAtualMQa > 1000) || (gasAtualBME < 10000.00 && iaqAcAtualBME > 0)) {
    riscoAmbiente = riscoAmbiente * 4; 
    if (gasAtualMQa > 2000){ 
     riscoAmbiente = riscoAmbiente * (gasAtualMQa/1000); 
     risco_msg +="Gas MQa >1000 ou BME <10.000; ";
    }
  }

   if ((iaqAtualBME > 100) && iaqAcAtualBME > 0){
     if (iaqAtualBME > 301){ //very bad
       riscoAmbiente = riscoAmbiente * 11;
       risco_msg +="IAQ > 301; ";
     } else {
        if (iaqAtualBME > 201){ // worse
           riscoAmbiente = riscoAmbiente * 9;
           risco_msg +="IAQ entre 201 e 300; ";
        } else {
           if (iaqAtualBME > 151){ //bad
             riscoAmbiente = riscoAmbiente * 7;
             risco_msg +="IAQ entre 151 e 200; ";
           } else{
              if (iaqAtualBME > 101){ //little bad
                 riscoAmbiente = riscoAmbiente * 5;
                 risco_msg +="IAQ entre 101 e 150; ";
             } 
           }
         }
       }
     }
  
  if (presencaAtual == 0 && riscoAmbiente > 2){
    riscoAmbiente = riscoAmbiente * 4;
    risco_msg +="Sem presença; ";
  }
  
  if (tempAtualDHT > 45.00 || gasAtualMQa > 3000 || (umidadeAtualDHT < 20 && umidadeAtualDHT != 0.0) || (gasAtualBME < 5000.0 && gasAtualBME != 0 && iaqAcAtualBME > 0)) {
   riscoAmbiente = riscoAmbiente * 8;
   risco_msg +="Limitas máximos excedidos! ";
  }
  
  if (riscoAmbiente > 24 || riscoPredio > 90){
    String alerta = "Alterta Risco ambiente!!\n";
           alerta += mensagemRetorno();
    bot.sendMessage("@CanalAlertaRiscos", alerta, "");
    digitalWrite(ledRPin,HIGH);
    digitalWrite(buzPin,HIGH);
    delay(500);
    digitalWrite(ledRPin,LOW);
   digitalWrite(buzPin,LOW);
  }
}

String mensagemRetorno(){
 String mensagem = "\nDispositivo:" + String(DEVICE_NAME)+";\n";
        mensagem += "Temperatura: aBME=" + String(tempAtualBME) + "ºC/iBME=" + String(tempInicialBME) + "ºC/; aDHT=" + String(tempAtualDHT) + "ºC/; iDHT="+ String(tempInicialDHT) + ";\n";
        mensagem += "Qualidade do Ar: aIAQ=" + String(iaqAtualBME) + "; iIAQ=" + String(iaqInicialBME) + "; Acurácia=" + String(iaqAcAtualBME) + ";\n";
        mensagem += "Umidade: aBME=" + String(umidadeAtualBME) + "%r.H; iBME=" + String(umidadeInicialBME) + "%r.H; aDHT=" + String(umidadeAtualDHT) + "%r.H; iDHT=" + String(umidadeInicialDHT) + ";\n";
        mensagem += "Resistência gases: aBME=" + String(gasAtualBME) + "Ohm; iBME=" + String(gasInicialBME) + "Ohm; aMQ2=" + String(gasAtualMQa) + "kppm; iMQ2=" + String(gasInicialMQa) + "kppm; MQ2 Digital=" + String(gasAtualMQd) + ";\n";
        mensagem += "Pressao BME =" + String(pressaoAtualBME) + "hPa; Altitude BME=" + String(altitudeAtualBME) + "m; Hall=" + String(hallAtual) + ";\n";
        mensagem += "Tempo de funcionamento =" + String((timer/1000)/60) + "m;\n";  
        mensagem += "Risco: Ambiente =" + String(riscoAmbiente) + "; Prédio=" + String(riscoPredio) + ";\n";
        mensagem += "Risco: "+ String(risco_msg) + ";\n";
    return mensagem;
}

void inicializaPin(){
  pinMode(ledRPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ledGPin, OUTPUT);
  pinMode(ledBPin, OUTPUT);
  pinMode(buzPin,  OUTPUT);
  pinMode(mq2dPin, INPUT);
}

void configuraDHT(){
    dht.begin();
}

void configuraWifi(){
  delay(2000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int count = 0;
  delay(2000);
  Serial.printf("\nConectando na rede Wifi! ");
  while (WiFi.status() != WL_CONNECTED && count < 50){
    Serial.print(".");
    ledOn("I");
    count++;
  }

  if(WiFi.status() != WL_CONNECTED){
    restartESP();
  } 
    else{
     Serial.printf("\nWifi conectado! Endereço IP: ");
     Serial.println(WiFi.localIP());
     digitalWrite(ledGPin, HIGH);
     delay(800);
     digitalWrite(ledGPin, LOW);
    }
}

void restartESP(){
    digitalWrite(buzPin, HIGH);
    delay (100);
    digitalWrite(buzPin, LOW);
    delay (100);
    digitalWrite(buzPin, HIGH);
    delay (100);
    digitalWrite(buzPin, LOW);
    esp_restart();
}

void configuraTS(){
    ThingSpeak.begin(clientTS);
}

void configuraAWS(){
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  client.begin(AWS_IOT_ENDPOINT, 8883, net);
  int count = 0;
  Serial.printf("\nConectando na AWS iOT! Device: %s", DEVICE_NAME);
  while (!client.connect(DEVICE_NAME) && count < 50) {
    Serial.print(".");
    ledOn("B");
    count++;
  }

  if(!client.connected()){
     esp_restart();
  }
   else{
    Serial.printf("\nAWS iOT conectado! \n");
     digitalWrite(ledBPin, HIGH);
     delay(800);    
     digitalWrite(ledBPin, LOW);
   }
  client.onMessage(recebeAWS);
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 }

 void reconectaAWS(){
  if (client.connected()){
    client.loop();
  } else {
    configuraAWS();   
    Serial.printf("\nReconectando AWS iOT\n");
    }
}

void recebeAWS(String &topic, String &payload){
  Serial.println("\n\nRecebendo mensagem: \n " + topic + " - " + payload);
  delay(500);
  StaticJsonDocument<512> doc;
  deserializeJson(doc, payload);
  riscoPredio = doc["riscopredio"];
}

void enviaAWS(){
  StaticJsonDocument<512> jsonDoc;
  jsonDoc["dispositivo"] = DEVICE_NAME;
  jsonDoc["localizacao"] = DEVICE_LOCAL;
  jsonDoc["riscoambiente"] = riscoAmbiente;
  jsonDoc["tempatualBME"] = tempAtualBME;
  jsonDoc["gasatualBME"] = gasAtualBME;
  jsonDoc["iaqatualBME"] = iaqAcAtualBME;
  jsonDoc["iaqacatualBME"] = iaqAcAtualBME;
  char jsonBuffer[512];
  serializeJson(jsonDoc, jsonBuffer);
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.printf("\nMensagem enviada AWS: %S \n", jsonBuffer);
}

void enviaTS(){
  ThingSpeak.setField(1, riscoAmbiente);
  ThingSpeak.setField(2, riscoPredio);
  ThingSpeak.setField(3, tempAtualDHT);
  ThingSpeak.setField(4, tempAtualBME);
  ThingSpeak.setField(5, gasAtualMQa);
  ThingSpeak.setField(6, gasAtualBME);
  ThingSpeak.setField(7, umidadeAtualDHT);
  ThingSpeak.setField(8, iaqAtualBME);
  ThingSpeak.setStatus("dispositivo em funcionamento");
  int mensagem = ThingSpeak.writeFields(CANAL_TS, CHAVE_TS);
  if(mensagem == 200){
    Serial.printf("Mensagem enviada ThingSpeak!!\n");
  }
  else{
    Serial.printf("Prolema ao atualizar canal. HTTP error code %S \n", String(mensagem));
    ledOn ("R");
  }
}

void consultaTelegram(){
  int count;
  num_mensagens_recebidas_telegram = bot.getUpdates(bot.last_message_received + 1);
  if (num_mensagens_recebidas_telegram > 0)
    {
      Serial.printf("\n %d mensagens recebidas no bot Telegram \n",num_mensagens_recebidas_telegram);
    }
  while(num_mensagens_recebidas_telegram) 
    {
      for (count=0; count<num_mensagens_recebidas_telegram; count++) 
      {                
       resposta_msg_recebida = "";
       resposta_msg_recebida = trataMensagemTelegram(bot.messages[count].text);
       bot.sendMessage(bot.messages[count].chat_id, resposta_msg_recebida, "");
      }
    num_mensagens_recebidas_telegram = bot.getUpdates(bot.last_message_received + 1);
  }
}

String trataMensagemTelegram(String msg_recebida){
    String resposta = "";
    bool comando_valido = false;

    if (msg_recebida.equals(CMD_LIST[0])){
      digitalWrite(ledGPin, LOW);
      digitalWrite(ledBPin, LOW);
      digitalWrite(ledRPin, HIGH);
      resposta = "led Red ligado";
      comando_valido = true;
    }
 
     if (msg_recebida.equals(CMD_LIST[1])){
      digitalWrite(ledRPin, LOW);
      digitalWrite(ledBPin, LOW);
      digitalWrite(ledGPin, HIGH);
      resposta = "led Green ligado";
      comando_valido = true;
    }
 
    if (msg_recebida.equals(CMD_LIST[2])){
      digitalWrite(ledRPin, LOW);
      digitalWrite(ledGPin, LOW);
      digitalWrite(ledBPin, HIGH);
      resposta = "led Blue ligado";
      comando_valido = true;
    }
 
    if (msg_recebida.equals(CMD_LIST[3])){
      digitalWrite(buzPin, HIGH);
      resposta = "Buz ligado";
      comando_valido = true;
    }
 
    if (msg_recebida.equals(CMD_LIST[4])){
      digitalWrite(buzPin, LOW);
      resposta = "Buz desligado";
      comando_valido = true;
    }
  
    if (msg_recebida.equals(CMD_LIST[5])){
      resposta = mensagemRetorno();
      comando_valido = true;
    }

    if (comando_valido == false){
      resposta = "Comando invalido: "+msg_recebida;      
      ledOn ("R");
    }
    return resposta;
}

void ledOn (String tipo){
  if (tipo == "R" || tipo == "r"){
  digitalWrite(ledRPin, HIGH);
  delay(100);
  digitalWrite(ledRPin, LOW);
  delay(100);
  }
  if (tipo == "G"  || tipo == "g"){
  digitalWrite(ledGPin, HIGH);
  delay(100);
  digitalWrite(ledGPin, LOW);
  delay(100);
  }
  if (tipo == "B" || tipo == "b"){
  digitalWrite(ledBPin, HIGH);
  delay(100);
  digitalWrite(ledBPin, LOW);
  delay(100);
  }
  if (tipo == "I" || tipo == "i"){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  }
}

//-----------------------------------------------------------------------

void setup(){
  Serial.begin(115200);
  while (!Serial) {;}
  while (DEVICE_ID != DEVICE_ID_ATUAL) {
   Serial.printf("\nDispositivo diferente do esperado, substituia o dispositivo ou Certificado - ID: %d \n", DEVICE_ID_ATUAL);
   ledOn ("R");
   delay(5000);
  }
  inicializaPin();
  configuraWifi();
  configuraAWS();
  configuraTS();
  configuraDHT();
  configuraBME();
}


void loop(){
   
  timer = millis();

  if (iaqSensor.run()) {
    getBME();
    Serial.printf("\r-\r");
   }

  
  if ((timer - lastTime[0]) > timerDelay[0]) {
   getDHT11();
   getMQ2();
   getHALL();
   checkSituacaoBME();
   Serial.print(mensagemRetorno());
   reconectaAWS();
   lastTime[0] = millis(); 
  }

  if ((timer - lastTime[1]) > timerDelay[1]) {
   consultaTelegram();
   verificaRisco();
   lastTime[1] = millis();
   Serial.printf("Loop 2\n");

  }

  if ((timer - lastTime[2]) > timerDelay[2]) {
   enviaAWS();
   enviaTS();
   lastTime[2] = millis();
   Serial.printf("Loop 3\n");
  }
 
  if ((timer - lastTime[3]) > timerDelay[3]) {
   tempInicialDHT = tempAtualDHT;
   tempInicialBME = tempAtualBME;
   gasInicialMQa = gasAtualMQa;
   gasInicialBME = gasAtualBME;
   iaqInicialBME = iaqAtualBME;
   umidadeInicialDHT = umidadeAtualDHT;
   umidadeInicialBME = umidadeAtualBME;
   lastTime[3] = millis();
   Serial.printf("Loop 4\n");
  }
}
