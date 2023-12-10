/*
  AWS IoT WiFi

  This sketch securely connects to an AWS IoT using MQTT over WiFi.
  It uses a private key stored in the ATECC508A and a public
  certificate for SSL/TLS authetication.

  It publishes a message every 5 seconds to arduino/outgoing
  topic and subscribes to messages on the arduino/incoming
  topic.

  The circuit:
  - Arduino MKR WiFi 1010 or MKR1000

  The following tutorial on Arduino Project Hub can be used
  to setup your AWS account and the MKR board:

  https://create.arduino.cc/projecthub/132016/securely-connecting-an-arduino-mkr-wifi-1010-to-aws-iot-core-a9f365

  This example code is in the public domain.
*/

#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000
#include <LiquidCrystal.h>

#include "arduino_secrets.h"

#include "DHT.h"
#define DHTPIN 2    // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

//LED3 pin 설정
#define LED_R_PIN 13
#define LED_G_PIN 12
#define LED_B_PIN 11

//열선패드 대체 LCD
LiquidCrystal lcd(8,3,4,5,6,7);//LCD pin

#include <ArduinoJson.h>
#include "Led.h"
#include "WarmPad.h"

/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

unsigned long lastMillis = 0;

//Led output 설정 + OFF 설정
Led ledR(LED_R_PIN);
Led ledG(LED_G_PIN);
Led ledB(LED_B_PIN);

WarmPad warmpad;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  dht.begin();
  lcd.begin(16,2);
  warmpad.init();

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, certificate);

  // Optional, set the client id used for MQTT,
  // each device that is connected to the broker
  // must have a unique client id. The MQTTClient will generate
  // a client id for you based on the millis() value if not set
  //
  // mqttClient.setId("clientId");

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    char payload[512];
    getDeviceStatus(payload);
    //Serial.print("payload:");
    //Serial.println(payload);
    sendMessage(payload);
  }
}

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("$aws/things/MyMKRWiFi1010/shadow/update/delta");
}

void getDeviceStatus(char* payload) {
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Read Warmpad status
  const char* options[] = { "COLD", "WARM", "HOT"};

  lcd.clear();
  // 예를 들어, led1.getState()가 0부터 3 사이의 값을 반환한다고 가정하면:
  int state = warmpad.getState();
  const char* selectedOption = (state >= 0 && state < 3) ? options[state] : "Invalid";

  if (t < 23){
   sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"WHITE\",\"WarmPad\":\"%s\",\"WarmPadState\":\"ON\"}}}",t,selectedOption);//WHITE(영하)
   ledR.off();
   ledG.off();
   ledB.off();
   lcd.print("WarmPad:");
   lcd.print(selectedOption);
   lcd.setCursor(0,1);
   lcd.print("ON");
   delay(100);
  }
  else if (t >=23 and t < 24){
   sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"BLUE\",\"WarmPad\":\"%s\",\"WarmPadState\":\"ON\"}}}",t,selectedOption);//BLUE(0~10도)
   ledR.on();
   ledG.on();
   ledB.off();
   lcd.print("WarmPad:");
   lcd.print(selectedOption);
   lcd.setCursor(0,1);
   lcd.print("ON");
   delay(100);
  }
  else if (t >=24 and t < 25){
   ledR.off();
   ledG.off();
   ledB.on();
   lcd.print("WarmPad:");
   lcd.print(selectedOption);
   lcd.setCursor(0,1);
   if(selectedOption == "COLD"){
    if (t >=24.3){
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"YELLOW\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//YELLOW(10~20도)
    }
   }
   else if(selectedOption == "WARM"){
    if(t >= 24.6){
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"YELLOW\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//YELLOW(10~20도)
    }
   }
   else{
    lcd.print("ON");
    sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"YELLOW\",\"WarmPad\":\"%s\",\"WarmPadState\":\"ON\"}}}",t,selectedOption);//YELLOW(10~20도)
   }
   delay(100);
  }
  else if(t >= 25){
   ledR.off();
   ledG.on();
   ledB.on();
   lcd.print("WarmPad:");
   lcd.print(selectedOption);
   lcd.setCursor(0,1);
   if(selectedOption == "COLD"){
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"RED\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//RED(20도 이상)
   }
   else if(selectedOption == "WARM"){
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"RED\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//RED(20도 이상)
   }
   else if(selectedOption == "HOT"){
    if(t>=26){
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"RED\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//RED(20도 이상)
    }
   }
   else{
    lcd.print("ON");
    sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"RED\",\"WarmPad\":\"%s\",\"WarmPadState\":\"ON\"}}}",t,selectedOption);//RED(20도 이상)
   }
   delay(100);
  }
}

void sendMessage(char* payload) {
  char TOPIC_NAME[]= "$aws/things/MyMKRWiFi1010/shadow/update";
  
  Serial.print("Publishing send message:");
  Serial.println(payload);
  mqttClient.beginMessage(TOPIC_NAME);
  mqttClient.print(payload);
  mqttClient.endMessage();
}


void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // store the message received to the buffer
  char buffer[512] ;
  int count=0;
  while (mqttClient.available()) {
     buffer[count++] = (char)mqttClient.read();
  }
  buffer[count]='\0'; // 버퍼의 마지막에 null 캐릭터 삽입
  Serial.println(buffer);
  Serial.println();

  // JSon 형식의 문자열인 buffer를 파싱하여 필요한 값을 얻어옴.
  // 디바이스가 구독한 토픽이 $aws/things/MyMKRWiFi1010/shadow/update/delta 이므로,
  // JSon 문자열 형식은 다음과 같다.
  // {
  //    "version":391,
  //    "timestamp":1572784097,
  //    "state":{
  //        "LED":"ON"
  //    },
  //    "metadata":{
  //        "LED":{
  //          "timestamp":15727840
  //         }
  //    }
  // }
  //
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, buffer);
  JsonObject root = doc.as<JsonObject>();
  JsonObject state = root["state"];
  const char* led3 = state["LED3"];
  const char* WP = state["WarmPad"];
  Serial.println(WP);
  
  char payload[512];
  
  if (strcmp(WP,"COLD")==0) {
    warmpad.on_C();
    sprintf(payload,"{\"state\":{\"reported\":{\"WARMPAD\":\"%s\"}}}","COLD");
    sendMessage(payload);
    
  } else if (strcmp(WP, "WARM") == 0){
    warmpad.on_W();
    sprintf(payload, "{\"state\":{\"reported\":{\"WARMPAD\":\"%s\"}}}","WARM");
    sendMessage(payload);
  
  } else if (strcmp(WP,"HOT")==0) {
    warmpad.on_H();
    sprintf(payload,"{\"state\":{\"reported\":{\"WARMPAD\":\"%s\"}}}","HOT");
    sendMessage(payload);
  
  }
}
