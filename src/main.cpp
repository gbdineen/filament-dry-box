/**********************************

FILAMENT DRYBOX DISPLAY & LEDS
V 1.2
10/15/25

**********************************/

#include <iostream>
#include <string>
#include <cstring>
#include "SPI.h"
#include <TFT_eSPI.h>
#include "OpenFontRender.h"
#include "EspMQTTClient.h"
#include <ArduinoJson.h>
#include "NotoSans_Bold.h"

// #define TFT_DC 2
// #define TFT_CS 5
// #define TFT_CLK 18
// #define TFT_MISO 19
// #define TFT_MOSI 23
// #define TFT_RST 0
#define LOOP_DELAY 0 // This controls how frequently the meter is updateD. For test purposes this is set to 0
#define DARKER_GREY 0x18E3
#define TTF_FONT NotoSans_Bold
#include "Free_Fonts.h"


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
String oldVal;
bool firstTime = true;

// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
EspMQTTClient client(
  "GL-SFT1200-887",
  "goodlife",
  "192.168.8.165",  // MQTT Broker server ip
  "gbdineen",   // Can be omitted if not needed
  "N1mbl3Sh@rk",   // Can be omitted if not needed
  "drybox_client",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

OpenFontRender ofr;
TFT_eSPI tft = TFT_eSPI();            // Invoke custom library with default width and height
TFT_eSprite spr = TFT_eSprite(&tft);  // Declare Sprite object "spr" with pointer to "tft" object

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

  // TEXT STUFF
  ofr.setDrawer(spr); 
  ofr.setFontSize((6 * r) / 4);
  ofr.setFontColor(TFT_WHITE, DARKER_GREY);

  uint8_t w = ofr.getTextWidth("444");
  uint8_t h = ofr.getTextHeight("4") + 4;
  spr.createSprite(w, h + 2);
  spr.fillSprite(DARKER_GREY); // (TFT_BLUE); // (DARKER_GREY);
  char str_buf[8];         // Buffed for string
  itoa (val, str_buf, 10); // Convert value to string (null terminated)
  uint8_t ptr = 0;         // Pointer to a digit character
  uint8_t dx = 4;          // x offset for cursor position
  if (val < 100) dx = ofr.getTextWidth("4") / 2; // Adjust cursor x for 2 digits
  if (val < 10) dx = ofr.getTextWidth("4");      // Adjust cursor x for 1 digit
  while ((uint8_t)str_buf[ptr] != 0) ptr++;      // Count the characters
  while (ptr) {
    ofr.setCursor(w - dx - w / 20, -h / 2.5);    // Offset cursor position in sprite
    ofr.rprintf(str_buf + ptr - 1);              // Draw a character
    str_buf[ptr - 1] = 0;                        // Replace character with a null
    dx += 1 + w / 3;                             // Adjust cursor for next character
    ptr--;                                       // Decrement character pointer
  }
  spr.pushSprite(x - w / 2, y - h / 2); // Push sprite containing the val number
  spr.deleteSprite();                   // Recover used memory

  // Make the TFT the print destination, print the units label direct to the TFT
  ofr.setDrawer(tft);
  ofr.setFontColor(TFT_GOLD, DARKER_GREY);
  ofr.setFontSize(r / 2.0);
  ofr.setCursor(x, y + (r * 0.4));
  ofr.cprintf("%");
  // Draw the meter value arc
  uint8_t thickness = r / 5;
  int16_t angleStart = 30; // Start angle of meter arc
  int16_t angleEnd = 330;   // End angle of meter arc
  int16_t angleRange = angleEnd - angleStart;
  int16_t angleVal = angleStart + (angleRange * val) / 100;

  // Draw the filled arc for the value
  tft.drawArc(x, y, r, r - thickness, angleStart, angleVal, TFT_MAGENTA, DARKER_GREY);

  
}

void barMeter (int x, int y, int val) {

    int fillVal = map(val, 0, 100, 0, 280);
    
    String valStr = "Print Progress " + String(val) + "%";
  
   

    tft.setCursor(0, 0);
    int r = 5;
    int t = 21;

    tft.drawRect(x, y, barWidth+2, barHeight+2, TFT_SILVER);
    tft.fillRect(x+1, y+1, fillVal, barHeight, TFT_GREEN);
    // tft.setTextSize(2);
 
 
    // spr.setTextColor(TFT_WHITE, TFT_DARKGREY);
    // spr.setTextPadding(spr.textWidth(oldVal));
    // Serial.println(spr.textWidth(oldVal));
    
    spr.drawString("                                   ", x + (barWidth /2) - 30, y + (barHeight / 2));
    // spr.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    spr.setTextPadding(spr.textWidth(oldVal));
    spr.drawString(valStr, x + (barWidth /2) - 30, y + (barHeight / 2));

    // spr.setTextPadding(spr.textWidth(valStr));
    // spr.drawString(oldVal, x + (barWidth /2) - 30, y + (barHeight / 2));
    // Serial.println(oldVal);
    oldVal = valStr;
    // if ( firstTime ) {
    //   firstTime = false;
      spr.pushSprite(x , y, TFT_BLACK);
    // };

    // spr.deleteSprite();
    // spr.fillSprite(TFT_BLACK);
    // spr.setSwapBytes(false);
    // spr.setColorDepth(8);
    // spr.createSprite(200, 60);
    // spr.setTextSize(2);
    // spr.setTextDatum(MC_DATUM);
    // spr.setTextColor(TFT_WHITE, TFT_BLACK);
    // spr.fillSprite(TFT_BLACK);
    // spr.drawString("PROGRESS", 100, 10);
    // // spr.println(valStr);
    // spr.printf("%d %%", val);
    // spr.fillSprite(TFT_TRANSPARENT);
    
    // spr.pushSprite(x , y);
    // // spr.setSwapBytes(true);
    // spr.deleteSprite();

    // spr.createSprite(30, 30);
    // // spr.fillSprite(TFT_TRANSPARENT);
    // // spr.setTextColor(TFT_WHITE, TFT_BLACK);
    // spr.fillSprite(TFT_GREEN);
    // // spr.drawString("Drying...", 0, 0, 6);
    // spr.printf("%d %%", val);
    // spr.pushSprite(x + (barWidth/2), y + (barHeight/2)); 
    // spr.
    // spr.deleteSprite();



    // // TEXT STUFF
    // ofr.setDrawer(spr); 
    // ofr.setFontSize((6 * r) / 
    // ofr.setFontColor(TFT_WHITE, TFT_TRANSPARENT);

    // uint8_t w = ofr.getTextWidth("444");
    // uint8_t h = ofr.getTextHeight("4") + 4;
    // spr.createSprite(barWidth, barHeight);
    // // spr.fillSprite(DARKER_GREY); // (TFT_BLUE); // (DARKER_GREY);
    // // spr.fillSprite(TFT_TRANSPARENT);
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
    // spr.pushSprite(x, y); // Push sprite containing the val number
    // spr.deleteSprite();                   // Recover used memory

    // // Make the TFT the print destination, print the units label direct to the TFT
    // ofr.setDrawer(tft);
    // ofr.setFontColor(TFT_GOLD, DARKER_GREY);
    // ofr.setFontSize(r / 2.0);
    // ofr.setCursor(x, y);
    // ofr.printf("%d %%", val);

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

  static int16_t xpos = tft.width() / 2;
  static int16_t ypos = tft.height() / 2;

  int barX = (tft.width() - barWidth) / 2;
  barMeter(barX, 10, progress);


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

  // JsonDocument wled;
  // JsonArray seg = wled["seg"].to<JsonArray>();
  // JsonObject segDetails = seg.add<JsonObject>();
  // JsonArray col = segDetails["col"].to<JsonArray>();
  // JsonArray rgb = col.add<JsonArray>();

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

      serializeJson(wled, wledSerialized);
      client.publish("wled/drybox/api", wledSerialized); // You can activate the retain flag by setting the third parameter to true

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

      serializeJson(wled, wledSerialized);
      client.publish("wled/drybox/api", wledSerialized); // You can activate the retain flag by setting the third parameter to true

    }
  }  
}



void onConnectionEstablished()
{
  client.setMaxPacketSize(3000);
  client.subscribe("octoPrint/event/plugin_spoolmanager_spool_selected", onSpoolInfoReceived);
  client.subscribe("octoPrint/event/DisplayLayerProgress_timerTrigger", onPrintProgressReceived);

  // client.publish("mytopic/test", "This is a message"); // You can activate the retain flag by setting the third parameter to true

  // Execute delayed instructions
  // client.executeDelayed(5 * 1000, []() {
  //   client.publish("mytopic/wildcardtest/test123", "This is a message sent 5 seconds later");
  // });
   
}

void setup()
{
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  if (ofr.loadFont(TTF_FONT, sizeof(TTF_FONT))) {
    Serial.println("Render initialize error");
    return;
  }
  spr.createSprite(barWidth, 60);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  // spr.fillSprite(TFT_BLACK);
  // spr.pushSprite(x , y, TFT_TRANSPARENT);
  spr.fillSprite(TFT_BLACK);
  // spr.fillRect(0, 0, spr.textWidth(oldVal), 50,  TFT_BLUE);
  spr.setTextDatum((MC_DATUM));
  spr.setFreeFont(FM9);


  // Optional functionalities of EspMQTTClient
  // client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  // // client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  // client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  // client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

void loop()
{
  client.loop();
  
}

void dump() 
{

  /* PRINT STATS IN TEXT ON TFT */
  // tft.setCursor(10, line_height);
  // tft.setTextColor(ILI9341_WHITE); 
  // tft.setTextSize(1);
  // tft.printf("Estimated Print Time: %d:%.2d", hours_estimated_print_time, minutes_estimated_print_time);
  // tft.setCursor(10, line_height*2);
  // tft.printf("Print Time Elapsed: %d:%.2d", hours_elapsed, minutes_elapsed);
  // tft.setCursor(10, line_height*3);
  // tft.printf("Print Time Left: %d:%.2d", hours, minutes);
  // tft.setCursor(10, line_height*4);
  // tft.printf("Complete: %d%%\n\n", completion);


  /* UNPACK AND REFORMAT ALL THE DATA FROM PRIUNT PROGRESS - DEPRICATED */
  // JsonObject printer_data = doc["printer_data"];

  // const char* printer_data_state_text = printer_data["state"]["text"]; // "Printing"
  // const char* printer_data_state_error = printer_data["state"]["error"]; // nullptr

  // JsonObject printer_data_job = printer_data["job"];
  // double printer_data_job_estimatedPrintTime = printer_data_job["estimatedPrintTime"];
  // double printer_data_job_averagePrintTime = printer_data_job["averagePrintTime"]; // 9576.348718455003
  // double printer_data_job_lastPrintTime = printer_data_job["lastPrintTime"]; // 9576.348718455003
  // const char* printer_data_job_user = printer_data_job["user"]; // "gbdineen"

  // JsonObject printer_data_progress = printer_data["progress"];
  // double printer_data_progress_completion = printer_data_progress["completion"]; // 61.80070861393104
  // long printer_data_progress_filepos = printer_data_progress["filepos"]; // 4918842
  // double printer_data_progress_printTime = printer_data_progress["printTime"]; // 5862
  // double printer_data_progress_printTimeLeft = printer_data_progress["printTimeLeft"]; // 3510
  // const char* printer_data_progress_printTimeLeftOrigin = printer_data_progress["printTimeLeftOrigin"];

  // // Tine left
  // double decimal_hours = printer_data_progress_printTimeLeft / 3600;
  // int hours = floor(decimal_hours);
  // double total_minutes = decimal_hours * 60;
  // int minutes = static_cast<int>(floor(total_minutes)) % 60;

  // // Elapsed time
  // double decimal_hours_elapsed = printer_data_progress_printTime / 3600;
  // int hours_elapsed = floor(decimal_hours_elapsed);
  // double total_minutes_elapsed = decimal_hours_elapsed * 60;
  // int minutes_elapsed = static_cast<int>(floor(total_minutes_elapsed)) % 60;


  // // Estimated print time
  // double decimal_hours_estimated_print_time = printer_data_job_estimatedPrintTime / 3600;
  // int hours_estimated_print_time = floor(decimal_hours_estimated_print_time);
  // double total_minutes_estimated_print_time = decimal_hours_estimated_print_time * 60;
  // int minutes_estimated_print_time = static_cast<int>(floor(total_minutes_estimated_print_time)) % 60;

  // // % complete
  // int completion = floor(printer_data_progress_completion);


  // Serial.printf("Print Time Hours Left: %d:%d \n", hours, minutes);
  // Serial.printf("Print Time Elapsed: %d:%d \n", hours_elapsed, minutes_elapsed);
  // Serial.printf("Estimated Print Time: %d:%.2d \n", hours_estimated_print_time, minutes_estimated_print_time);

  // Serial.printf("Completion: %d%%\n\n", completion);

  
  // int16_t line_height = 20;

  // tft.fillScreen(ILI9341_BLACK);
}
