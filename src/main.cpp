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
#include <iostream>
#include <string> 

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

// JSON STUFF
JsonDocument spoolsJson;
bool useFilters = true;

WiFiClient wifiClient; 
HTTPClient http;
WebSocketsClient webSocket;
PubSubClient mqttClient(wifiClient);

// DISPLAY STUFF
int slots = 4;  // Number of spool slots, also happens to be the number of displays
std::string screenMode = "overview";

// TIMER STUFF
unsigned long previousMillis = 0;

// Init 4 oled screen objects, one for each slot in the box
Adafruit_SSD1306 display0(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus0, OLED_RESET);
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus0, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus1, OLED_RESET);
Adafruit_SSD1306 display3(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus1, OLED_RESET);
// Adafruit_SSD1306 updateDisplay = display0;
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

// Array of screens for programatic doing things to each one 
Adafruit_SSD1306 displayArray[] = {display0, display1, display2, display3};

// Four addresses for pointing to each oled individually over I2C
int addressArray[] = {SCREEN_ADDRESS_0, SCREEN_ADDRESS_1, SCREEN_ADDRESS_2, SCREEN_ADDRESS_3};

void initDisplays() {

  for (int i=0; i<slots; i++){  
    if(!displayArray[i].begin(SSD1306_SWITCHCAPVCC, addressArray[i])) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    } else {

      Serial.println("Display " + String(i) + "initialized");

      displayArray[i].clearDisplay();
      
      for (int y=0; y<20; y+=2){

        displayArray[i].fillCircle(displayArray[i].width() / 2, displayArray[i].height() / 2, y, SSD1306_WHITE);
        displayArray[i].display(); // Update screen with each newly-drawn circle
        delay(1);
      }
    }
  }
  delay(500);
}

void overviewDisplay(int displayId, int spoolId, int remWeight, const char * material, const char * name) {

  screenMode = "overview";

  displayArray[displayId].clearDisplay();
  u8g2_for_adafruit_gfx.begin(displayArray[displayId]);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_crox2hb_tr); // 10px high  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  u8g2_for_adafruit_gfx.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  u8g2_for_adafruit_gfx.setFontDirection(0);
  u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
  int16_t disp_center_x = displayArray[displayId].width()/2;
  int16_t disp_center_y = displayArray[displayId].height()/2;

  int8_t font_height = 10;
  int8_t padding_screen_top = 10;
  int8_t padding_screen_left = 10;
  int8_t padding_font_bottom = 7;
  int8_t character_height = font_height + padding_font_bottom;

  u8g2_for_adafruit_gfx.setCursor(padding_screen_left, padding_screen_top);
  u8g2_for_adafruit_gfx.print(name);

  u8g2_for_adafruit_gfx.setCursor(padding_screen_left, padding_screen_top + character_height);
  u8g2_for_adafruit_gfx.print(material);

  u8g2_for_adafruit_gfx.setCursor(padding_screen_left,  padding_screen_top + (character_height*2));
  u8g2_for_adafruit_gfx.print(F("Rem wt: ")); u8g2_for_adafruit_gfx.print(remWeight);

  u8g2_for_adafruit_gfx.setCursor(padding_screen_left,  padding_screen_top + (character_height*3));
  u8g2_for_adafruit_gfx.print(F("Spool Id: ")); u8g2_for_adafruit_gfx.print(spoolId);

  displayArray[displayId].display();
  delay(10);
}

void spoolWeightDisplay() {

  screenMode = "spool_weight";

  for (int i=0; i<slots; i++){  

    displayArray[i].clearDisplay();

    int disp_w =  displayArray[i].width();
    int disp_h =  displayArray[i].height();
    int disp_padding_top = 20;
    int disp_center_x = disp_w/2;
    int disp_center_y = disp_h/2;
    int meter_w = displayArray[i].width() - 20;
    int meter_h = 20;
    int meter_x = (disp_w - meter_w) /2;
    // = (disp_h - meter_h) /2;
    int meter_y = disp_padding_top;
    int text_center_x;
    int text_y = (disp_padding_top + meter_h) + 20;
    int text_w;
    int text_h = 10;

    u8g2_for_adafruit_gfx.begin(displayArray[i]);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_crox2hb_tr); // 10px high  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    u8g2_for_adafruit_gfx.setFontMode(0);                 // use u8g2 transparent mode (this is default)
    u8g2_for_adafruit_gfx.setFontDirection(0);
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
   
    displayArray[i].clearDisplay();
    displayArray[i].drawRect(meter_x, disp_padding_top, meter_w, meter_h, SSD1306_WHITE);

    int remWeightInt = spoolsJson[i]["remaining_weight"];
    std::string remWeightStr = std::to_string(remWeightInt) + "g / 1000g";
    const char* remWeightPtr = remWeightStr.c_str();
    uint8_t weight_meter = map(remWeightInt, 0, 1000, 0, meter_w);
    uint16_t weight_perc = map(remWeightInt, 0, 1000, 0, 100);
    std::string weight_perc_str = std::to_string(weight_perc) + "%";
    const char* weight_perc_ptr = weight_perc_str.c_str();
    int weight_perc_y = (meter_y + meter_h) - 5;

    displayArray[i].fillRect(meter_x, disp_padding_top, weight_meter, meter_h, SSD1306_WHITE);

    // text_w = u8g2_for_adafruit_gfx.getUTF8Width("Weight Remaining");
    // text_center_x = (displayArray[i].width() - text_w) /2;
    // u8g2_for_adafruit_gfx.setCursor(text_center_x, (disp_padding_top+meter_h)+15);
    // u8g2_for_adafruit_gfx.print("Weight Remaining");

    text_w = u8g2_for_adafruit_gfx.getUTF8Width(remWeightPtr);
    text_center_x = (displayArray[i].width() - text_w) /2;
    u8g2_for_adafruit_gfx.setCursor(text_center_x, text_y);
    u8g2_for_adafruit_gfx.print(remWeightPtr);

    text_w = u8g2_for_adafruit_gfx.getUTF8Width(weight_perc_ptr);
    text_center_x = (displayArray[i].width() - text_w) /2;
    u8g2_for_adafruit_gfx.setCursor(text_center_x, weight_perc_y);
    u8g2_for_adafruit_gfx.setBackgroundColor(SSD1306_BLACK);
    u8g2_for_adafruit_gfx.print(weight_perc_ptr);

    displayArray[i].display();

  }

  delay(3000);

}

void updateDisplay(int displayId, int spoolId, int remWeight, const char * material, const char * name) {

  screenMode = "overview";

  displayArray[displayId].clearDisplay();
  u8g2_for_adafruit_gfx.begin(displayArray[displayId]);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_crox2hb_tr); // 10px high  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  u8g2_for_adafruit_gfx.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  u8g2_for_adafruit_gfx.setFontDirection(0);
  u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
  int16_t disp_center_x = displayArray[displayId].width()/2;
  int16_t disp_center_y = displayArray[displayId].height()/2;

  int8_t font_height = 10;
  int8_t padding_screen_top = 10;
  int8_t padding_screen_left = 10;
  int8_t padding_font_bottom = 7;
  int8_t character_height = font_height + padding_font_bottom;

  u8g2_for_adafruit_gfx.setCursor(padding_screen_left, padding_screen_top);
  u8g2_for_adafruit_gfx.print(name);

  u8g2_for_adafruit_gfx.setCursor(padding_screen_left, padding_screen_top + character_height);
  u8g2_for_adafruit_gfx.print(material);

  u8g2_for_adafruit_gfx.setCursor(padding_screen_left,  padding_screen_top + (character_height*2));
  u8g2_for_adafruit_gfx.print(F("Rem wt: ")); u8g2_for_adafruit_gfx.print(remWeight);

  u8g2_for_adafruit_gfx.setCursor(padding_screen_left,  padding_screen_top + (character_height*3));
  u8g2_for_adafruit_gfx.print(F("Spool Id: ")); u8g2_for_adafruit_gfx.print(spoolId);

  displayArray[displayId].display();
  delay(10);
}

void updateSpoolsJson (){

  http.useHTTP10(true);

  // Query spoolman to get only spools that are in the 'Drybox' location. Should be just4 spools.
  http.begin(wifiClient, String(baseAPI_URL + "spool?location=Drybox")); 
  http.GET();

  DeserializationError error;

  if (!useFilters) {

    error =  deserializeJson(spoolsJson, http.getStream());

  } else {

    JsonDocument filter;
    filter[0]["id"] = true;
    filter[0]["remaining_weight"] = true;
    filter[0]["filament"]["name"] = true;
    filter[0]["filament"]["material"] = true;

    error =  deserializeJson(spoolsJson, http.getStream(), DeserializationOption::Filter(filter));
  }
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  serializeJsonPretty(spoolsJson,Serial);

  http.end();
  
  delay(100);

}

void getSpoolLocations() {

  http.useHTTP10(true);

  // Query spoolman to get only spools that are in the 'Drybox' location. Should be just4 spools.
  http.begin(wifiClient, String(baseAPI_URL + "setting/locations_spoolorders")); 
  http.GET();

  JsonDocument doc;

  DeserializationError error =  deserializeJson(doc, http.getStream());
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

  serializeJsonPretty(doc,Serial);

}

// Get all current spools from spoolman
void getSpools() {

  // Serial.println("getSpools");

  // screenMode = "overview";

  http.useHTTP10(true);

  // Query spoolman to get only spools that are in the 'Drybox' location. Should be just4 spools.
  http.begin(wifiClient, String(baseAPI_URL + "spool?location=Drybox")); 
  http.GET();

  DeserializationError error;


  if (!useFilters) {

    error =  deserializeJson(spoolsJson, http.getStream());

  } else {

    JsonDocument filter;
    filter[0]["id"] = true;
    filter[0]["remaining_weight"] = true;
    filter[0]["filament"]["name"] = true;
    filter[0]["filament"]["material"] = true;

    error =  deserializeJson(spoolsJson, http.getStream(), DeserializationOption::Filter(filter));
  }
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  serializeJsonPretty(spoolsJson,Serial);

  for (int i=0; i<4; i++){

    int spoolId = spoolsJson[i]["id"];
    int remWeight = spoolsJson[i]["remaining_weight"];
    const char * material = spoolsJson[i]["filament"]["material"];
    const char * name = spoolsJson[i]["filament"]["name"];

    // String payload = name + "\n" + material + "\n" + "Rem Wt. " + remWeight + "g\n" + "Spool ID: " + spoolID;
  
    // Adafruit_SSD1306 currDisplay = displayArray[i];
    // int address = addressArray[i];
    overviewDisplay(i, spoolId, remWeight, material, name);
    
  }
  http.end();

  delay(3000);
  getSpoolLocations();
  spoolWeightDisplay();
}


void selectSpool(const char * spoolID){

  // Serial.println("Spool selected");

};

void getSpool(){

};

void updateSpool(int &spoolId, int &remWeight) {

  for (int i=0; i<4; i++){

    // JsonObject spoolObj = spoolsJson[i];
  
    if (spoolsJson[i]["id"] == spoolId) {

      spoolsJson[i]["remaining_weight"] = remWeight;
      serializeJsonPretty(spoolsJson, Serial);

      updateSpoolsJson();

      // int existingSpoolId = spoolsJson[i]["id"];

      // Serial.println("Updating spool" + existingSpoolId);

      //  Serial.println("Identified spool");

      // int spoolId = spoolsJson[i]["id"];
      // int remWeight = spoolsJson[i]["remaining_weight"];
      const char * material = spoolsJson[i]["filament"]["material"];
      const char * name = spoolsJson[i]["filament"]["name"];


      // updateDisplay = displayArray[i];

      displayArray[i].clearDisplay();
      u8g2_for_adafruit_gfx.begin(displayArray[i]);
      u8g2_for_adafruit_gfx.setFont(u8g2_font_crox2hb_tr);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
      u8g2_for_adafruit_gfx.setFontMode(0);                 // use u8g2 transparent mode (this is default)
      u8g2_for_adafruit_gfx.setFontDirection(0);
      // u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
      int16_t disp_center_x = displayArray[i].width()/2;
      int16_t disp_center_y = displayArray[i].height()/2;

      const char *  updateMsg = "UPDATED";

      int16_t text_width = u8g2_for_adafruit_gfx.getUTF8Width(updateMsg);

      int16_t x1, y1;
      uint16_t w, h;
      displayArray[i].getTextBounds(updateMsg, 0, 0, &x1, &y1, &w, &h);

      int16_t text_center_x = (displayArray[i].width() - text_width) /2;
      int16_t text_center_y = disp_center_y + h;

      u8g2_for_adafruit_gfx.setCursor(text_center_x,text_center_y);     // Start at top-left corner
      displayArray[i].invertDisplay(true);
      u8g2_for_adafruit_gfx.print(updateMsg); 

      // u8g2_for_adafruit_gfx.print(updateMsg);
      displayArray[i].display();

      delay(3000);

      // displayArray[i].clearDisplay();
      displayArray[i].invertDisplay(false);

      // screenMode = "overview";

      if (screenMode == "spool_weight") {

        spoolWeightDisplay();

      } else if (screenMode == "overview") {
        
        overviewDisplay(i, spoolId, remWeight, material, name);

      }

    }  
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  // Serial.println("MQTT Callback");

  JsonDocument doc;
  DeserializationError error =  deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  if (String(topic) == "octoPrint/event/plugin_Spoolman_spool_selected") {
    
    String selectedSpoolId = doc["spoolId"];
    
    for (int i=0; i<4; i++){
      Serial.print("selectedSpoolId: " + selectedSpoolId + "  Slot Spool ID: " + String(spoolsJson[i]["id"]) + "\n");

      if (selectedSpoolId == String(spoolsJson[i]["id"])) {

        displayArray[i].begin(SSD1306_SWITCHCAPVCC, addressArray[i]);
        displayArray[i].invertDisplay(true);

      } else {

        displayArray[i].invertDisplay(false);
      
      }
    }
  }

  JsonObject root = doc.as<JsonObject>();
   
  String spoolID = doc["spoolId"];
  Serial.println("Spool ID: " + spoolID);
  Serial.println(topic);


  display0.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS_0);
  display0.invertDisplay(true);
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

      DeserializationError error;
      JsonDocument doc;

      // useFilters = false;

      if (!useFilters) {

        error =  deserializeJson(doc, payload);

      } else {

        JsonDocument filter;

        // FOR UPDATES
        filter["type"] = true;
        filter["resource"] = true;

        // 
        filter["payload"]["id"] = true;
        filter["payload"]["remaining_weight"] = true;
        filter["payload"]["filament"]["name"] = true;
        filter["payload"]["filament"]["material"] = true;

        error =  deserializeJson(doc, payload, DeserializationOption::Filter(filter));
      }
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }

			serializeJsonPretty(doc,Serial);
      int spoolId = doc["payload"]["id"];
      int remWeight = doc["payload"]["remaining_weight"];

      Serial.println("\nResource: " + String(doc["resource"]));

      if (doc["type"] == "updated") {

        if (doc["resource"] == "spool") {

          updateSpool(spoolId, remWeight);


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