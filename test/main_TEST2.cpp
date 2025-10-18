
/*
  Basic ESP32 MQTT example
  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP32 board/library.
  It connects to an MQTT server then:
  - publishes "connected to MQTT" to the topic "outTopic"
  - subscribes to the topic "inTopic", printing out messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the "ON" payload is send to the topic "inTopic" , LED will be turned on, and acknowledgement will be send to Topic "outTopic"
  - If the "OFF" payload is send to the topic "inTopic" , LED will be turned OFF, and acknowledgement will be send to Topic "outTopic"
  It will reconnect to the server if the connection is lost using a
  reconnect function.
*/

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <PubSubClient.h>

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <ArduinoJson.h>

const char* mqtt_server = "192.168.8.165"; //mqtt server
const char* ssid = "GL-SFT1200-887";
const char* password = "goodlife";
const char* mqtt_username = "gbdineen";
const char* mqtt_password = "N1mbl3Sh@rk";

char* theTopic = "octoPrint/event/DisplayLayerProgress_timerTrigger";

#define TFT_DC 2
#define TFT_CS 5
#define TFT_CLK 18
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_RST 0

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
WiFiClient espClient;
PubSubClient client(espClient); //lib required for mqtt

int LED = 02;

unsigned long testText() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 0);
  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(2);
  tft.println();
}

void callback(char* topic, byte* payload, unsigned int length) {   //callback includes topic and payload ( from which (topic) the payload is comming)
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");


  JsonDocument doc;
  deserializeJson(doc, payload, length);
  const char* newStamp = doc["new"];
  Serial.print("Printing: ");
  Serial.println(newStamp);
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 0);
  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(2);
  tft.println(topic);



  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  if ((char)payload[0] == 'O' && (char)payload[1] == 'N') //on
  {
    // digitalWrite(LED, HIGH);
    Serial.println("on");
    client.publish("outTopic", "LED turned ON");
  }
  else if ((char)payload[0] == 'O' && (char)payload[1] == 'F' && (char)payload[2] == 'F') //off
  {
    digitalWrite(LED, LOW);
    Serial.println(" off");
    client.publish("outTopic", "LED turned OFF");
  }
  Serial.println();
} 

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32_clientID", mqtt_username, mqtt_password)) {
      Serial.println("re connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "Nodemcu connected to MQTT");
      // ... and resubscribe
      client.subscribe(theTopic);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void connectmqtt()
{
  client.connect("ESP32_clientID", mqtt_username, mqtt_password);  // ESP will connect to mqtt broker with clientID
  {
    Serial.println("connected to MQTT on first try");
    // Once connected, publish an announcement...

    // ... and resubscribe
    client.subscribe(theTopic); //topic=Demo
    client.publish("outTopic",  "connected to MQTT");

    if (!client.connected())
    {
      Serial.print("not connected");
      reconnect();
    }
  }
}


void setup()
{
  Serial.begin(115200);
  // pinMode(LED, OUTPUT);
  // digitalWrite(LED, LOW);
  WiFi.begin(ssid, password);
  Serial.println("connected");
  client.setServer(mqtt_server, 1883);//connecting to mqtt server
  client.setCallback(callback);
  //delay(5000);

  tft.begin();
  tft.setRotation(1);  // HORZ TOP 1; 
  tft.fillScreen(ILI9341_BLACK);;
  tft.setCursor(10, 0);
  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(2);

  connectmqtt();
}


void loop()
{
  // put your main code here, to run repeatedly:
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();
}



