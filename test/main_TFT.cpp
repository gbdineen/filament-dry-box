/***************************************************
  This is our GFX example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "EspMQTTClient.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

// For the Adafruit shield, these are the default.
#define TFT_DC 2
#define TFT_CS 5
#define TFT_CLK 18
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_RST 0

// StaticJsonDocument<256> doc;

EspMQTTClient client(
  "GL-SFT1200-887",
  "goodlife",
  "192.168.8.165",  // MQTT Broker server ip
  "gbdineen",   // Can be omitted if not needed
  "N1mbl3Sh@rk",   // Can be omitted if not needed
  "DryBox",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

const char* ssid = "GL-SFT1200-887";
const char* password = "goodlife";
const char* mqtt_server = "192.168.8.165";

WiFiClient espClient;
PubSubClient client(espClient); //lib required for mqtt


// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
// If using the breakout, change pins as desired
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);



void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  deserializeJson(doc, payload, length);
  const char* newStamp = doc["new"];
  Serial.print("Printing: ");
  Serial.println(newStamp);
  testText(newStamp);
  // Serial.println(printing);

  // strlcpy(name, doc["name"] | "default", sizeof(name));
  // use the JsonDocument as usual...
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

unsigned long testText(String thePayload) {
  // tft.setRotation(1);  // HORZ TOP 1; 
  tft.fillScreen(ILI9341_BLACK);
  // unsigned long start = micros();
  tft.setCursor(10, 0);
  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(2);
  tft.println(thePayload);
  // tft.println("Hello World!");
  // tft.println(start);
  // tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  // tft.println(1234.56);
  // tft.setTextColor(ILI9341_RED);    tft.setTextSize(3);
  // tft.println(0xDEADBEEF, HEX);
  // tft.println();
  // tft.setTextColor(ILI9341_GREEN);
  // tft.setTextSize(5);
  // tft.println("Groop");
  // tft.setTextSize(2);
  // tft.println("I implore thee,");
  // tft.setTextSize(1);
  // tft.println("my foonting turlingdromes.");
  // tft.println("And hooptiously drangle me");
  // tft.println("with crinkly bindlewurdles,");
  // tft.println("Or I will rend thee");
  // tft.println("in the gobberwarts");
  // tft.println("with my blurglecruncheon,");
  // tft.println("see if I don't!");
  // return micros() - start;
}


void setup() {
  Serial.begin(9600);
  Serial.println("ILI9341 Test!"); 

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // StaticJsonDocument<256> doc;
   
  tft.begin();
  tft.setRotation(1);  // HORZ TOP 1; 
  tft.fillScreen(ILI9341_BLACK);


  // testText();


  // read diagnostics (optional but can help debug problems)
  // uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  // Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  // x = tft.readcommand8(ILI9341_RDMADCTL);
  // Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  // x = tft.readcommand8(ILI9341_RDPIXFMT);
  // Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  // x = tft.readcommand8(ILI9341_RDIMGFMT);
  // Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  // x = tft.readcommand8(ILI9341_RDSELFDIAG);
  // Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
  

}

void loop(void) {
  // for(uint8_t rotation=0; rotation<4; rotation++) {
  //   tft.setRotation(rotation);
  //   testText();
  //   delay(1000);
  // }
   if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
  }
}