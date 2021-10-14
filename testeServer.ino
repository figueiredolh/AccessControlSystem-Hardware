#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

//Configuração pinos RFID
#define RST_PIN         22          // Pino de RST
#define SS_PIN          21         // Pino SDA
#define ledVerde 4

//Criação de instância do módulo RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Ssid e senha WiFi
const char* ssid = "figueiredolima";
const char* password = "henschelf63lima*";

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
  url = "http://192.168.100.118:3000/tag?rfiduid="+rfidUID; //IP NodeJS
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

void setup() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  //Serial.println("My Sketch has started");
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  
  //Conexão com Wi-Fi
  conectarWifi();

  //Inicia módulo RFID
  iniciarRfid();

  //led
  pinMode(ledVerde, OUTPUT);
}

void loop() {
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
