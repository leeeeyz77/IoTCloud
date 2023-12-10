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
  warmpad.init();//열선패드 초기화

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
    getDeviceStatus(payload);//디바이스 상태 JSON 문자열로 가져옴(payload)
    //Serial.print("payload:");
    //Serial.println(payload);
    sendMessage(payload);//payload aws iot로 전송
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

//현재 디바이스 상태를 얻는 함수
void getDeviceStatus(char* payload) {
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();//현재 온도(온습도 센서)

  //WarmPad의 3가지 설정
  const char* options[] = { "COLD", "WARM", "HOT"};

  lcd.clear();//lcd 초기화
  //warmpad의 0,1,2 상태 가져오기
  int state = warmpad.getState();

  //getState()로 가져온 상태(0,1,2)를 문자열로 바꾸기("COLD","WARM","HOT")
  const char* selectedOption = (state >= 0 && state < 3) ? options[state] : "Invalid";

  
  if (t < 0){//영하이면 3색 LED는 WHITE
   //payload에 AWS IOT에 전송할 JSON 문자열 넣는 작업
   sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"WHITE\",\"WarmPad\":\"%s\",\"WarmPadState\":\"ON\"}}}",t,selectedOption);//WHITE(영하)
   //WHITE를 켜기 위해 3색 LED 조정(사용한 3색 LED는 모든 값을 다 끌 경우 WHITE가 나오는 버전)
   //자신이 사용하는 3색 LED에 따라 변경이 필요한 부분
   ledR.off(); 
   ledG.off();
   ledB.off();
   //lcd에 출력
   lcd.print("WarmPad:");
   lcd.print(selectedOption);//현재 온도 설정
   lcd.setCursor(0,1);
   lcd.print("ON");//현재 열선패드 상태
   delay(100);
  }
  else if (t >=0 and t < 10){//0~9도 일때 3색 LED는 BLUE
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
  else if (t >=10 and t < 20){//10~19도 일때 3색 LED는 YELLOW
   ledR.off();
   ledG.off();
   ledB.on();
   lcd.print("WarmPad:");
   lcd.print(selectedOption);
   lcd.setCursor(0,1);
  //만약 온도 설정이 COLD일 때,
   if(selectedOption == "COLD"){
    //온도가 13도 이상이면 열선패드 OFF
    if (t >=13){
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"YELLOW\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//YELLOW(10~20도)
    }
   }
  //만약 온도 설정이 WARM일 때,
   else if(selectedOption == "WARM"){
    //온도가 16도 이상이면 열선패드 OFF
    if(t >= 16){
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"YELLOW\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//YELLOW(10~20도)
    }
   }
   else{//이외의 상황에서는 다 ON
    lcd.print("ON");
    sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"YELLOW\",\"WarmPad\":\"%s\",\"WarmPadState\":\"ON\"}}}",t,selectedOption);//YELLOW(10~20도)
   }
   delay(100);
  }
  else if(t >= 20){//20도 이상일 때 3색 LED는 RED
   ledR.off();
   ledG.on();
   ledB.on();
   lcd.print("WarmPad:");
   lcd.print(selectedOption);
   lcd.setCursor(0,1);
    //만약 온도 설정이 COLD이면
   if(selectedOption == "COLD"){
     //열선패드 OFF
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"RED\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//RED(20도 이상)
   }
    //만약 온도 설정이 WARM이면
   else if(selectedOption == "WARM"){
     //열선패드 OFF
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"RED\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//RED(20도 이상)
   }
    //만약 온도 설정이 HOT이면
   else if(selectedOption == "HOT"){
     //온도가 20도 이상이면 OFF
    if(t>=20){
      lcd.print("OFF");
      sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"RED\",\"WarmPad\":\"%s\",\"WarmPadState\":\"OFF\"}}}",t,selectedOption);//RED(20도 이상)
    }
   }
   else{//이외의 상황에서는 다 ON
    lcd.print("ON");
    sprintf(payload, "{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED3\":\"RED\",\"WarmPad\":\"%s\",\"WarmPadState\":\"ON\"}}}",t,selectedOption);//RED(20도 이상)
   }
   delay(100);
  }
}

void sendMessage(char* payload) { //JSON 문자열을 AWS IoT MQTT Client에 보내는 코드
  char TOPIC_NAME[]= "$aws/things/MyMKRWiFi1010/shadow/update";
  
  Serial.print("Publishing send message:");
  Serial.println(payload);
  mqttClient.beginMessage(TOPIC_NAME);
  mqttClient.print(payload);
  mqttClient.endMessage();
}

//AWS IoT에서 받아오는 메세지
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

  //받은 값이 COLD이면 warmpad.on_C
  if (strcmp(WP,"COLD")==0) {
    warmpad.on_C();
    sprintf(payload,"{\"state\":{\"reported\":{\"WARMPAD\":\"%s\"}}}","COLD");
    sendMessage(payload);

    //받은 값이 WARM이면 warmpad.on_W
  } else if (strcmp(WP, "WARM") == 0){
    warmpad.on_W();
    sprintf(payload, "{\"state\":{\"reported\":{\"WARMPAD\":\"%s\"}}}","WARM");
    sendMessage(payload);

    //받은 값이 HOT이면 warmpad.on_H
  } else if (strcmp(WP,"HOT")==0) {
    warmpad.on_H();
    sprintf(payload,"{\"state\":{\"reported\":{\"WARMPAD\":\"%s\"}}}","HOT");
    sendMessage(payload);
  
  }
}
