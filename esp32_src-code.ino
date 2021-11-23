#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoWebsockets.h>

//Configuração pinos RFID
#define RST_PIN         22          // Pino de RST
#define SS_PIN          21         // Pino SDA
#define ledVerde 4

//Criação de instância do módulo RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Ssid e senha WiFi
const char* ssid = "figueiredolima";
const char* password = "henschelf63lima*";

//Conexão com websockets server
const char* websockets_server = "ws://35.199.80.139/abertura";
using namespace websockets;

void onMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened"); 
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

WebsocketsClient client;

//Variável globais
String rfidUID; //UID RFID
String url;
int httpResponseCode;

//Funções

void conectarWifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando WiFi..");
  }
  Serial.println("IP: " + (String)WiFi.localIP());
}

void iniciarRfid(){
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
}

bool rfidPresente() {
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }
}

void enviarTag(){
  HTTPClient http;
  url = "http://35.199.80.139/tag?rfiduid="+rfidUID;//IP NodeJS
  http.begin(url.c_str());
  http.addHeader("content-type", "text/plain");
  http.addHeader("auth-key", "3df456dgfjga5hdk74");
  httpResponseCode = http.POST("");
  delay(300);
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    if(httpResponseCode == 200){
      digitalWrite(ledVerde, HIGH);
      delay(2000);
      digitalWrite(ledVerde, LOW);
    }
    //else{
      //digitalWrite(ledVermelho, HIGH);
      //delay(2000);
      //digitalWrite(ledVermelho, LOW);
    //}
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void iniciarWs(){
  //Conexão WebSockets
  
  //- Setup Callbacks
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);
  
  //- Connect to server
  bool connected = client.connect(websockets_server);

  while(!connected){
    // Exibimos mensagem de falha
    Serial.println("Not Connected!");
    Serial.println("Tentando reconectar em 2s");
    delay(2000);
    connected = client.connect(websockets_server);
  }

  // Exibimos mensagem de sucesso
  Serial.println("Connected!");
  // Enviamos uma msg "Hello Server" para o servidor
  client.send("Hello Server");
  // Se não foi possível conectar

  // Iniciamos o callback onde as mesagens serão recebidas
  client.onMessage([&](WebsocketsMessage message)
  {        
    // Exibimos a mensagem recebida na serial
    Serial.print("Got Message: ");
    Serial.println(message.data());

    // Ligamos/Desligamos o led de acordo com o comando
    if(message.data().equalsIgnoreCase("ON")){
        digitalWrite(ledVerde, HIGH);
        delay(2000);
        digitalWrite(ledVerde, LOW);
    }
    if(message.data().equalsIgnoreCase("OFF"))
        digitalWrite(ledVerde, LOW);
  });
}

//-------Setup--------

void setup() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  //Serial.println("My Sketch has started");
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  
  //Conexão com Wi-Fi
  conectarWifi();

  //Conexão WebSockets 
  iniciarWs();

  //Iniciar módulo RFID
  iniciarRfid();

  //led
  pinMode(ledVerde, OUTPUT);
}
//-------Loop--------

void loop() {
  client.poll();
  if(!client.available()) {
    iniciarWs();
  }
  
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if(rfidPresente()){
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      rfidUID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
      rfidUID.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    rfidUID.toUpperCase();
    Serial.println("UID da Tag: " + rfidUID);

    enviarTag();

    rfidUID = "";
  }
  delay(1000);
}
