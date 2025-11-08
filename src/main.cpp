#include <SPI.h>
#include <HTTPClient.h>
// #include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <WebSocketsClient.h>
#include <PubSubClient.h> // For MQTT
#include <WiFi.h>

// #define USE_SERIAL Serial
// #define WIFI_un "GL-SFT1200-887"
// #define WIFI_pw "goodlife"
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS_0 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_ADDRESS_1 0x3D 
#define SCREEN_ADDRESS_2 0x3C
#define SCREEN_ADDRESS_3 0x3D  
#define I2C0_SDA 9 // &Wire Bus (default )
#define I2C0_SCL 8
#define I2C1_SDA 5  // &I2C_Bus1  
#define I2C1_SCL 4
#define WIRE Wire
// #define HOST_IP "192.168.8.228"
// #define WS_PORT 7912
// #define MQTT_PORT 1883

const char * hostIP = "192.168.8.228";
String baseAPI_URL = "http://192.168.8.228:7912/api/v1/";
int wsPort = 7912;

// Example struct to hold parsed data
struct SpoolEvent {
  String type;
  int id;
  String name;
  float weight;
};

TwoWire  I2C_Bus0 = TwoWire(0);
TwoWire  I2C_Bus1 = TwoWire(1);

const char* ssid = "GL-SFT1200-887";
const char* password = "goodlife";
// bool setMaxPacketSize(3000);

// MQTT broker details
const char* mqtt_broker = hostIP;
const int mqtt_port = 1883; // Or 8883 for SSL/TLS
const char* mqtt_client_id = "DryboxPeripherals";
const char* mqttUN = "gbdineen";
const char* mqttPW = "N1mbl3Sh@rk";


WiFiClient wifiClient; 
// HttpClient http = HttpClient(wifiClient, hostIP, wsPort);
HTTPClient http;
WebSocketsClient webSocket;
PubSubClient mqttClient(wifiClient);


// DISPLAY STUFF
int slots = 4;  // Number of spool slots, also happens to be the number of displays

// Init 4 oled screen objects, one for each slot in the box
Adafruit_SSD1306 display0(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus0, OLED_RESET);
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus0, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus1, OLED_RESET);
Adafruit_SSD1306 display3(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus1, OLED_RESET);
Adafruit_SSD1306 currDisplay = display0;
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

// Array of screens for programatic doing things to each one 
Adafruit_SSD1306 displayArray[] = {display0, display1, display2, display3};

// Four addresses for pointing to each oled individually over I2C
int addressArray[] = {SCREEN_ADDRESS_0, SCREEN_ADDRESS_1, SCREEN_ADDRESS_2, SCREEN_ADDRESS_3};

void handleSpoolEvent(const SpoolEvent& event) { // Struct example from CGPT
  Serial.println("🔔 Received Spool Event:");
  Serial.printf("  Type:   %s\n", event.type.c_str());
  Serial.printf("  ID:     %d\n", event.id);
  Serial.printf("  Name:   %s\n", event.name.c_str());
  Serial.printf("  Weight: %.2f g\n", event.weight);
}

void initDisplays() {

  for (int i=0; i<slots; i++){  
    if(!displayArray[i].begin(SSD1306_SWITCHCAPVCC, addressArray[i])) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    } else {
      // Adafruit_SSD1306 buff = displayArray[i];
      Serial.println("Display " + String(i) + "initialized");

      displayArray[i].clearDisplay();
      // The INVERSE color is used so circles alternate white/black
      displayArray[i].fillCircle(displayArray[i].width() / 2, displayArray[i].height() / 2, 10, SSD1306_WHITE);
      displayArray[i].display(); // Update screen with each newly-drawn circle
      delay(1000);

      // delay(2000);
    }
  }
}

void initDisplays_OLD(Adafruit_SSD1306 &display, String &payload, int &address) {

  //  if(!display.begin(SSD1306_SWITCHCAPVCC, address)) {
  //   Serial.println(F("SSD1306 allocation failed"));
  //   for(;;); // Don't proceed, loop forever
  // }
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_crox2hb_tr);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  u8g2_for_adafruit_gfx.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  u8g2_for_adafruit_gfx.setFontDirection(0);
  u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
  int16_t disp_center_x = display.width()/2;
  int16_t disp_center_y = display.height()/2;
  int16_t text_width = u8g2_for_adafruit_gfx.getUTF8Width(payload.c_str());
  
  // Pointer receives for getTextBounds -- not sure I'm going to use tjis. getUTF8Width seems more accurate
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(payload, 0, 0, &x1, &y1, &w, &h);
  
  int16_t text_center_x = (display.width() - text_width) /2;
  int16_t text_center_y = disp_center_y + h;
  
  u8g2_for_adafruit_gfx.setCursor(0, 20);     // Start at top-left corner
  u8g2_for_adafruit_gfx.print(payload);

  display.display();
  delay(10);
}

// Get all current spools from spoolman
void getSpools() {

  Serial.println("getSpools");

  http.useHTTP10(true);
  http.begin(wifiClient, String(baseAPI_URL + "spool?location=Drybox")); // Query spoolman to get only spools that are in the 'Drybox' location. Should be just4 spools.
  
  http.GET();
  // http.sendRequest("GET", "/spool?location=Drybox");
  // // http.beginRequest();
  // http.get("/spool?location=Drybox");
  // // http.endRequest();

  // int response = http.responseStatusCode();

  // Serial.println(response);

  JsonDocument spoolsJson;

  deserializeJson(spoolsJson, http.getStream());

  // // Initial call so /spool returns wrapped in a JSON array so we need to go down a level to get to the data
  JsonArray spoolsJsonRoot = spoolsJson.as<JsonArray>();
  Serial.println(spoolsJsonRoot.size());

  for (int i=0; i<4; i++){

    JsonObject spoolObj = spoolsJsonRoot[i];
    JsonObject extraObj = spoolObj["extra"];
    JsonObject filamentObj = spoolObj["filament"];
    int spoolID = spoolObj["id"];
    int remWeight = spoolObj["remaining_weight"];
    String status = extraObj["active"];
    String slot = extraObj["slot"];
    String material = filamentObj["material"];
    String name = filamentObj["name"];
    
    // if (status == "true") { // Grab just the spools that have an active status of 'true' in spoolman


      // String payloadPtr*;
      String payload = name + "\n" + material + "\n" + "Rem Wt. " + remWeight + "g\n" + "Spool ID: " + spoolID;
      // String info = "Display: " + String(slotCount) + "\n" + String(name) + "\n" + material + "\n" + "Rem Wt. " + remWeight + "g\n" + "Spool ID: " + spoolID;
      currDisplay = displayArray[i];
      int address = addressArray[i];
      initDisplays_OLD(currDisplay, payload, address);
      // if (slotCount<4){
      //   slotCount++;
      // } else {
      //   slotCount=0;
      // }
      // Serial.println(i);
    // }

  }
  http.end();
}


void selectSpool(const char * spoolID){

  Serial.println("Spool selected");

};

void getSpool(const char * spoolID){

};

void updateSpool(JsonObject obj) {

  for (int i=0; i<4; i++){

    JsonDocument spoolsJson;
    deserializeJson(spoolsJson, http.getStream());
    JsonObject spoolObj = spoolsJson[i];
  
    if (spoolObj["id"] == obj["id"]) {

      Serial.println("Updating spool");

      JsonObject filamentObj = spoolObj["filament"];
      String material = filamentObj["material"];
      String name = filamentObj["name"];  
      int remWeight = obj["remaining_weight"];
      int spoolId = spoolObj["id"];

      String info = name + "\n" + material + "\n" + "Rem Wt. " + remWeight + "g\n" + "Spool ID: " + spoolId;

      currDisplay = displayArray[i];
      int address = addressArray[i];
     
      if(! currDisplay.begin(SSD1306_SWITCHCAPVCC,address)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
      }   

      currDisplay.clearDisplay();
      u8g2_for_adafruit_gfx.begin(currDisplay);
      u8g2_for_adafruit_gfx.setFont(u8g2_font_crox2hb_tr);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
      u8g2_for_adafruit_gfx.setFontMode(1);                 // use u8g2 transparent mode (this is default)
      u8g2_for_adafruit_gfx.setFontDirection(0);
      u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
      int16_t disp_center_x = currDisplay.width()/2;
      int16_t disp_center_y = currDisplay.height()/2;

      const char *  updateMsg = "UPDATED";

      int16_t text_width = u8g2_for_adafruit_gfx.getUTF8Width(updateMsg);

      int16_t x1, y1;
      uint16_t w, h;
      currDisplay.getTextBounds(updateMsg, 0, 0, &x1, &y1, &w, &h);

      int16_t text_center_x = (currDisplay.width() - text_width) /2;
      int16_t text_center_y = disp_center_y + h;

      u8g2_for_adafruit_gfx.setCursor(text_center_x,text_center_y);     // Start at top-left corner
      u8g2_for_adafruit_gfx.print(updateMsg); 
      currDisplay.invertDisplay(true);
      currDisplay.display();

      delay(3000);

      currDisplay.clearDisplay();
      currDisplay.invertDisplay(false);

      initDisplays_OLD(currDisplay, info, address);

    }
  
  }
  

};

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  // Serial.println("MQTT Callback");

  // JsonDocument doc;
  // deserializeJson(doc, payload);

  // if (String(topic) == "octoPrint/event/plugin_Spoolman_spool_selected") {
    
  //   String selectedSpoolId = doc["spoolId"];
    
  //   for (int i=0; i<4; i++){
  //     Serial.print("selectedSpoolId: " + selectedSpoolId + "  Slot Spool ID: " + String(spoolsJsonRoot[i]["id"]) + "\n");

  //     if (selectedSpoolId == String(spoolsJsonRoot[i]["id"])) {

  //       displayArray[i].begin(SSD1306_SWITCHCAPVCC, addressArray[i]);
  //       displayArray[i].invertDisplay(true);

  //     } else {

  //       displayArray[i].invertDisplay(false);
      
  //     }

  //   }

  // }

  // JsonObject root = doc.as<JsonObject>();
   
  // String spoolID = doc["spoolId"];
  // Serial.println("Spool ID: " + spoolID);
  // Serial.println(topic);


  // display0.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS_0);
  // display0.invertDisplay(true);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED:
			Serial.printf("[WSc] Connected to url: %s\n", payload);

			// send message to server when Connected
			webSocket.sendTXT("Connected");
			break;
		case WStype_TEXT: {
			
			JsonDocument doc;
  		deserializeJson(doc, payload);
			serializeJsonPretty(doc,Serial);

    // Expect format: {"type":"spool.updated","data":{"id":5,"name":"PLA White","weight":812}}
			// SpoolEvent evt;
			// evt.type = doc["type"].as<String>();
			// evt.id = doc["data"]["id"] | -1;
			// evt.name = doc["data"]["name"].as<String>();
			// evt.weight = doc["data"]["weight"] | -1.0;

      Serial.println("Resource: " + String(doc["resource"]));

      if (doc["type"] == "updated") {

        if (doc["resource"] == "spool") {

          JsonObject payload = doc["payload"];
          int spoolID = payload["id"];

          updateSpool(payload);

        } else if (doc["resource"] == "setting"){
            
          getSpools();

        }

      }
			// handleSpoolEvent(evt);
			break;
		}
		case WStype_BIN:
			Serial.printf("[WSc] get binary length: %u\n", length);
			// hexdump(payload, length); // probably wont be needing this 

			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
			break;
	}

}


void setup() {
  Serial.begin(115200);

  I2C_Bus0.begin(I2C0_SDA, I2C0_SCL, 100000);
  I2C_Bus1.begin(I2C1_SDA, I2C1_SCL, 100000); 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  mqttClient.setServer(mqtt_broker, mqtt_port);

  while (!mqttClient.connected()) {
    delay(1000);
    Serial.print("Attempting MQTT connection...");
    // mqttClient to connect
    if (mqttClient.connect(mqtt_client_id, mqttUN, mqttPW)) {
      Serial.println("connected");

      // Set the callback function for incoming mqtt messages
      mqttClient.setCallback(mqttCallback);

      // Once connected, publish an announcement...
      mqttClient.publish("mqttStatus","MQTT Connectee");
      // ... and resubscribe
      mqttClient.subscribe("octoPrint/event/plugin_Spoolman_spool_selected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  // Start up web socket... should be quick once wifi is started
	webSocket.begin(hostIP, wsPort, "/api/v1/");
	webSocket.onEvent(webSocketEvent);
	webSocket.setReconnectInterval(5000);
  
  // Initialize and begin displays
  initDisplays();
  // delay(500); // Pause for a moment

  getSpools();
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // mqttClient to connect
    if (mqttClient.connect(mqtt_client_id, mqttUN, mqttPW)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("mqttStatus","MQTT Connectee");
      // ... and resubscribe
      mqttClient.subscribe("octoPrint/event/plugin_Spoolman_spool_selected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  webSocket.loop();
  //  if (!mqttClient.connected()) {
  //   reconnect();
  // }
  mqttClient.loop();
}