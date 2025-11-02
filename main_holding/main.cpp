#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
// #include "EspMQTTClient.h"

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

TwoWire  I2C_Bus0 = TwoWire(0);
TwoWire  I2C_Bus1 = TwoWire(1);

const char* ssid = "GL-SFT1200-887";
const char* password = "goodlife";
int slotCount = 0;

WiFiClient client; 
HTTPClient http;

Adafruit_SSD1306 display0(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus0, OLED_RESET);
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus0, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus1, OLED_RESET);
Adafruit_SSD1306 display3(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_Bus1, OLED_RESET);
Adafruit_SSD1306 dual_display = display0;
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

Adafruit_SSD1306 displayArray[] = { display0, display1, display2, display3};

int addressArray[] = {SCREEN_ADDRESS_0, SCREEN_ADDRESS_1, SCREEN_ADDRESS_2, SCREEN_ADDRESS_3};



void testdrawchar(Adafruit_SSD1306 display, String payload, int address) {

   if(!display.begin(SSD1306_SWITCHCAPVCC, address)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_9x15_tf);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  u8g2_for_adafruit_gfx.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  u8g2_for_adafruit_gfx.setFontDirection(0);
  u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
  int16_t disp_center_x = display.width()/2;
  int16_t disp_center_y = display.height()/2;

  int16_t text_width = u8g2_for_adafruit_gfx.getUTF8Width(payload.c_str());

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(payload, 0, 0, &x1, &y1, &w, &h);
  // Serial.printf("Text W: %d ", w);  Serial.printf("Text H: %d\n", h); Serial.printf("Text Y: %d\n", y1);
  
  int16_t text_center_x = (display.width() - text_width) /2;
  int16_t text_center_y = disp_center_y + h;
  
  // display.invertDisplay(true);
  
  u8g2_for_adafruit_gfx.setCursor(0, 15);     // Start at top-left corner
  
  u8g2_for_adafruit_gfx.print(payload);
  display.display();
  delay(10);
}

void getSpools() {

  http.useHTTP10(true);
  http.begin(client, "http://192.168.8.226:7912/api/v1/spool");
  http.GET();

  JsonDocument doc;

  deserializeJson(doc, http.getStream());

  JsonArray root = doc.as<JsonArray>();

  for (int i=0; i<root.size(); i++){

    JsonObject spoolObj = root[i];
    JsonObject extraObj = spoolObj["extra"];
    JsonObject filamentObj = spoolObj["filament"];
    int remWeight = spoolObj["remaining_weight"];
    String status = extraObj["active"];
    String slot = extraObj["slot"];
    String material = filamentObj["material"];
    const char* name = filamentObj["name"];
    
    if (status == "true") {

      Serial.printf("Name: %s\n", name);
      Serial.printf("Material: %s\n", material);
      Serial.printf("Slot: %s\n", slot); 
      Serial.printf("Status: %s\n\n", status); 

      String info = "Name: " + String(name) + "\n" + "Material: " + material + "\n" + "Rem Wt." + remWeight;
      dual_display = displayArray[slotCount];
      int address = addressArray[slotCount];
      testdrawchar(dual_display, info, address);
      if (slotCount<4){
        slotCount++;
      } else {
        slotCount=0;
      }
    }

  }
  http.end();
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

  Serial.println("Connected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);

  getSpools();
  
}

void loop() {
  
}