/**********************************

FILAMENT DRYBOX DISPLAY & LEDS
V 1.2
10/15/25

**********************************/

#include <Arduino.h>
#include <iostream>
#include <string>
#include <cstring>
#include <SPI.h>
#include <TFT_eSPI.h>
// #include "EspMQTTClient.h"
#include <WiFi.h>
#include <PubSubClient.h> // For MQTT
#include <ArduinoJson.h>
#include "Free_Fonts.h"
#include "NotoSansBold15.h"
#include "NotoSansBold36.h"


// #define TEST_MODE // UNCOMMMENT FOR TEST MODE
#define LOOP_DELAY 0 // This controls how frequently the meter is updateD. For test purposes this is set to 0
#define DARKER_GREY 0x18E3
#define TFT_FONT NotoSans_Bold
#define FlashFS LittleFS
// The font names are arrays references, thus must NOT be in quotes ""
#define AA_FONT_SMALL NotoSansBold15
#define AA_FONT_LARGE NotoSansBold36
// #define AA_FONT_MONO  NotoSansMonoSCB20 // NotoSansMono-SemiCondensedBold 20pt


TFT_eSPI tft = TFT_eSPI();            // Invoke custom library with default width and height
TFT_eSprite spr = TFT_eSprite(&tft);  // Declare Sprite object "spr" with pointer to "tft" object

int8_t progress;
uint16_t rgbR;
uint16_t rgbG;
uint16_t rgbB;
uint32_t runTime = 0;       // time for next update
bool range_error = 0; 
bool initMeter = true;
bool setMaxPacketSize(3000);
int barWidth = 280;
int barHeight = 40;
int barX;
int barY = 10;

String oldVal;
String valStr;
bool firstTime = true;

// MQTT broker details
const char * hostIP = "192.168.8.228";
const char* mqtt_broker = hostIP;
const int mqtt_port = 1883; // Or 8883 for SSL/TLS
const char* mqtt_client_id = "drybox_tft";
const char* mqttUN = "gbdineen";
const char* mqttPW = "N1mbl3Sh@rk";
int lastReconnectAttempt = 0;

const char* ssid = "GL-SFT1200-887";
const char* password = "goodlife";

void mqttCallback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


void ringMeter(int x, int y, int r, int val, const char *units)
{
  // This function draws or updates a ring meter at position x,y
  // of radius r to show the value val with the units label units

  // Constrain value to valid range
  if (val < 0)
  {
    val = 0;
    range_error = true;
  }
  if (val > 100)
  {
    val = 100;
    range_error = true;
  }

  // Draw the meter only once
  if (initMeter)
  {
    // Draw the grey meter background
    // tft.drawCircle(x, y, r, DARKER_GREY);
    // tft.drawCircle(x, y, r - 1, DARKER_GREY);
    // tft.drawCircle(x, y, r - 2, DARKER_GREY);
    tft.fillCircle(x, y, r, DARKER_GREY);
    tft.drawSmoothCircle(x, y, r, TFT_SILVER, DARKER_GREY);
    initMeter = false;
  }

  // // TEXT STUFF
  // ofr.setDrawer(spr); 
  // ofr.setFontSize((6 * r) / 4);
  // ofr.setFontColor(TFT_WHITE, DARKER_GREY);

  // uint8_t w = ofr.getTextWidth("444");
  // uint8_t h = ofr.getTextHeight("4") + 4;
  // spr.createSprite(w, h + 2);
  // spr.fillSprite(DARKER_GREY); // (TFT_BLUE); // (DARKER_GREY);
  // char str_buf[8];         // Buffed for string
  // itoa (val, str_buf, 10); // Convert value to string (null terminated)
  // uint8_t ptr = 0;         // Pointer to a digit character
  // uint8_t dx = 4;          // x offset for cursor position
  // if (val < 100) dx = ofr.getTextWidth("4") / 2; // Adjust cursor x for 2 digits
  // if (val < 10) dx = ofr.getTextWidth("4");      // Adjust cursor x for 1 digit
  // while ((uint8_t)str_buf[ptr] != 0) ptr++;      // Count the characters
  // while (ptr) {
  //   ofr.setCursor(w - dx - w / 20, -h / 2.5);    // Offset cursor position in sprite
  //   ofr.rprintf(str_buf + ptr - 1);              // Draw a character
  //   str_buf[ptr - 1] = 0;                        // Replace character with a null
  //   dx += 1 + w / 3;                             // Adjust cursor for next character
  //   ptr--;                                       // Decrement character pointer
  // }
  // spr.pushSprite(x - w / 2, y - h / 2); // Push sprite containing the val number
  // spr.deleteSprite();                   // Recover used memory

  // // Make the TFT the print destination, print the units label direct to the TFT
  // ofr.setDrawer(tft);
  // ofr.setFontColor(TFT_GOLD, DARKER_GREY);
  // ofr.setFontSize(r / 2.0);
  // ofr.setCursor(x, y + (r * 0.4));
  // ofr.cprintf("%");
  // // Draw the meter value arc
  // uint8_t thickness = r / 5;
  // int16_t angleStart = 30; // Start angle of meter arc
  // int16_t angleEnd = 330;   // End angle of meter arc
  // int16_t angleRange = angleEnd - angleStart;
  // int16_t angleVal = angleStart + (angleRange * val) / 100;

  // // Draw the filled arc for the value
  // tft.drawArc(x, y, r, r - thickness, angleStart, angleVal, TFT_MAGENTA, DARKER_GREY);

  
}

void barMeter (int val) {

    // int meterX

    int fillVal = map(val, 0, 100, 0, 280);
    
    valStr = "Print Progress " + String(val) + "%";
  
    tft.setCursor(0, 0);
    tft.drawRect(barX, barY, barWidth+2, barHeight+2, TFT_SILVER);

    spr.loadFont(AA_FONT_SMALL); // Must load the font first
    spr.createSprite(barWidth, barHeight);
    
    if (val==0) {
      spr.fillRect(0, 0,  barWidth , barHeight, TFT_BLACK);
    } else {
      spr.fillRect(0,0, fillVal, barHeight, TFT_DARKGREEN);
    }

    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_WHITE);

    spr.drawString("Print Progress " + String(val) + "%", barWidth /2, barHeight / 2);       
    spr.pushSprite(barX+1 , barY+1);
    spr.unloadFont();
    spr.deleteSprite();

  } 

void onPrintProgressReceived(const String& payload) {
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  progress = doc["progress"];
  Serial.println("Progress: " + String(progress));

  barMeter(progress);

}


void onSpoolInfoReceived(const String& payload) {
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // const char* spoolName = doc["spoolName"];
  String spoolName = doc["spoolName"];
  int slotNumber = spoolName[5]- '0';
  // Serial.println("Spool Name: " + String(slotNumber));

  // Set LED colors based on slot number  
  for (int i=0; i<4; i++) {
    if (i != slotNumber) {
      
      rgbR = 0;
      rgbG = 0;
      rgbB = 255;

      JsonDocument wled;
      JsonArray seg = wled["seg"].to<JsonArray>();
      JsonObject segDetails = seg.add<JsonObject>();
      segDetails["id"] = i; // Convert char to int
      JsonArray col = segDetails["col"].to<JsonArray>();
      JsonArray rgb = col.add<JsonArray>();
      rgb.add(rgbR);
      rgb.add(rgbG);
      rgb.add(rgbB);

      String wledSerialized;
			const char * wledSerializedStr = wledSerialized.c_str();

      serializeJson(wled, wledSerialized);
      mqttClient.publish("wled/drybox/api", wledSerializedStr); // You can activate the retain flag by setting the third parameter to true

    } else {
      rgbR = 0;
      rgbG = 255;
      rgbB = 0;

      JsonDocument wled;
      JsonArray seg = wled["seg"].to<JsonArray>();
      JsonObject segDetails = seg.add<JsonObject>();
      segDetails["id"] = slotNumber;
      JsonArray col = segDetails["col"].to<JsonArray>();
      JsonArray rgb = col.add<JsonArray>();
      rgb.add(rgbR);
      rgb.add(rgbG);
      rgb.add(rgbB);

      String wledSerialized;
			const char * wledSerializedStr = wledSerialized.c_str();

      serializeJson(wled, wledSerialized);
      mqttClient.publish("wled/drybox/api", wledSerializedStr); // You can activate the retain flag by setting the third parameter to true

      // tft.color565(255, 35, 288);

    }
  }  

  // Display sppol info on TFT
  spr.loadFont(AA_FONT_LARGE); // Must load the font first
  spr.createSprite(320, 80);
  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE);
  spr.drawString("Spool Slot: " + String(slotNumber), 160, 40);
  spr.pushSprite(0 , 60);
  spr.unloadFont();
  spr.deleteSprite();


}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("mqtt callback");
  Serial.println(topic);

  JsonDocument filter;
  JsonDocument doc;

  if (String(topic) == "octoPrint/event/plugin_Spoolman_spool_selected") {

    // filter["progress"] = true;
    
    DeserializationError error =  deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    // serializeJsonPretty(doc,Serial);


  } else if (String(topic) == "octoPrint/event/DisplayLayerProgress_timerTrigger")
  {

    filter["progress"] = true;
    
    DeserializationError error =  deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    int progress = doc["progress"];

    barMeter(progress);

    // serializeJsonPretty(doc,Serial);

  } else if (String(topic) == "octoPrint/event/plugin_Spoolman_spool_usage_committed") 
  {

    DeserializationError error =  deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    serializeJsonPretty(doc,Serial);
  }

  // serializeJsonPretty(doc,Serial);
  // std::string remWeightStr = std::to_string(payload);

  // onPrintProgressReceived(remWeightStr);
}


void setup()
{
  Serial.begin(115200);
 

  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(3000);

	WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  lastReconnectAttempt = 0;

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  barX = (tft.width() - barWidth) / 2;

  // Optional functionalities of EspMQTTClient
  // client.enableDebuggingMessages(); // Enable debugg ing messages sent to serial output
  // // client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  // client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  // client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

boolean reconnect() {
  if (mqttClient.connect(mqtt_client_id, mqttUN, mqttPW)) {
    // Serial.println("MQTT connected");
    // Once connected, publish an announcement...
    std::string clientIdStr = mqtt_client_id;
    std::string connectMsg = "MQTT client " + clientIdStr + " connected";
    // const char* connectMsgChar = connectMsg.c_str();


    mqttClient.publish("mqttStatus",connectMsg.c_str());
    // ... and resubscribe
    // mqttClient.subscribe("octoPrint/event/plugin_Spoolman_spool_selected");
    //  mqttClient.setMaxPacketSize(3000);
    // mqttClient.subscribe("octoPrint/event/plugin_spoolmanager_spool_selected");
    mqttClient.subscribe("octoPrint/event/DisplayLayerProgress_timerTrigger");
  }
  return mqttClient.connected();
}

void loop()
{
   if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    mqttClient.loop();
  }
  
    // client.loop();

#ifdef TEST_MODE
  for (int i=0;  i <= 99; i++) {
    if (i != 100) {
      barMeter(barX, 10, i);
    } else {
      barMeter(barX, 10, 0);
    } 
    delay(100);
  }
#endif
  
}

