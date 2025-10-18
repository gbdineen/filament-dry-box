

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "EspMQTTClient.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Fonts/FreeSans9pt7b.h>


#define TFT_DC 2
#define TFT_CS 5
#define TFT_CLK 18
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_RST 0

EspMQTTClient client(
  "GL-SFT1200-887",
  "goodlife",
  "192.168.8.165",  // MQTT Broker server ip
  "gbdineen",   // Can be omitted if not needed
  "N1mbl3Sh@rk",   // Can be omitted if not needed
  "drybox_client",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);


bool setMaxPacketSize(256);

void setup()
{
  Serial.begin(115200);
  tft.begin();
  tft.setTextColor(ILI9341_WHITE); 
  tft.setFont(&FreeSans9pt7b);
  tft.setRotation(1);  // HORZ TOP 1; 
  tft.fillScreen(ILI9341_BLACK);

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  // client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  // client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  // client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // Subscribe to "mytopic/test" and display received message to Serial
  // client.subscribe("octoPrint/M117/message", [](const String & payload) {
  //   Serial.println(payload);
  // });

  // Subscribe to "mytopic/wildcardtest/#" and display received message to Serial
  // client.subscribe("octoPrint/#", [](const String & topic, const String & payload) {
  //   Serial.println("(From wildcard) topic: " + topic + ", payload: " + payload);
  // });

  client.subscribe("octoPrint/temperature/tool0", [](const String & topic, const String & payload) {
    // Serial.println("Payload: " + payload);

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, payload);

    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    double actual = doc["actual"]; // 19.5
    Serial.print("Actual Temp: "); Serial.println(actual);
    double target = doc["target"]; // 0.0
    Serial.print("Target Temp: "); Serial.println(target);

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(ILI9341_WHITE); 
    tft.setTextSize(1);
    tft.println("Actual Temp: " + String(actual));
    tft.setCursor(10, 20);
    tft.println("Target Temp: " + String(target));


  });

  // Publish a message to "mytopic/test"
  client.publish("mytopic/test", "This is a message"); // You can activate the retain flag by setting the third parameter to true

  // Execute delayed instructions
  // client.executeDelayed(5 * 1000, []() {
  //   client.publish("mytopic/wildcardtest/test123", "This is a message sent 5 seconds later");
  // });
}

void loop()
{
  client.loop();
}
