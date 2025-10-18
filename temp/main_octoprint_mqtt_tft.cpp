

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "EspMQTTClient.h"
#include <ArduinoJson.h>
// #include <PubSubClient.h>
// #include <WiFi.h>
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


bool setMaxPacketSize(3000);

void setup()
{
  Serial.begin(115200);
  tft.begin();
  tft.setTextColor(ILI9341_WHITE); 
  tft.setFont(&FreeSans9pt7b);
  tft.setRotation(1);  // HORZ TOP 1; 
  tft.fillScreen(ILI9341_BLACK);

  // Optional functionalities of EspMQTTClient
  // client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  // // client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  // client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  // client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

void onTestMessageReceived(const String& payload) {
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }


  JsonObject printer_data = doc["printer_data"];

  const char* printer_data_state_text = printer_data["state"]["text"]; // "Printing"
  const char* printer_data_state_error = printer_data["state"]["error"]; // nullptr

  JsonObject printer_data_job = printer_data["job"];
  double printer_data_job_estimatedPrintTime = printer_data_job["estimatedPrintTime"];
  double printer_data_job_averagePrintTime = printer_data_job["averagePrintTime"]; // 9576.348718455003
  double printer_data_job_lastPrintTime = printer_data_job["lastPrintTime"]; // 9576.348718455003
  const char* printer_data_job_user = printer_data_job["user"]; // "gbdineen"

  JsonObject printer_data_progress = printer_data["progress"];
  double printer_data_progress_completion = printer_data_progress["completion"]; // 61.80070861393104
  long printer_data_progress_filepos = printer_data_progress["filepos"]; // 4918842
  double printer_data_progress_printTime = printer_data_progress["printTime"]; // 5862
  double printer_data_progress_printTimeLeft = printer_data_progress["printTimeLeft"]; // 3510
  const char* printer_data_progress_printTimeLeftOrigin = printer_data_progress["printTimeLeftOrigin"];

  // Tine left
  double decimal_hours = printer_data_progress_printTimeLeft / 3600;
  int hours = floor(decimal_hours);
  double total_minutes = decimal_hours * 60;
  int minutes = static_cast<int>(floor(total_minutes)) % 60;

  // Elapsed time
  double decimal_hours_elapsed = printer_data_progress_printTime / 3600;
  int hours_elapsed = floor(decimal_hours_elapsed);
  double total_minutes_elapsed = decimal_hours_elapsed * 60;
  int minutes_elapsed = static_cast<int>(floor(total_minutes_elapsed)) % 60;


  // Estimated print time
  double decimal_hours_estimated_print_time = printer_data_job_estimatedPrintTime / 3600;
  int hours_estimated_print_time = floor(decimal_hours_estimated_print_time);
  double total_minutes_estimated_print_time = decimal_hours_estimated_print_time * 60;
  int minutes_estimated_print_time = static_cast<int>(floor(total_minutes_estimated_print_time)) % 60;

  // % complete
  int completion = floor(printer_data_progress_completion);


  Serial.printf("Print Time Hours Left: %d:%d \n", hours, minutes);
  Serial.printf("Print Time Elapsed: %d:%d \n", hours_elapsed, minutes_elapsed);
  Serial.printf("Estimated Print Time: %d:%.2d \n", hours_estimated_print_time, minutes_estimated_print_time);

  Serial.printf("Completion: %d%%\n\n", completion);

  
  int16_t line_height = 20;

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, line_height);
  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(1);
  tft.printf("Estimated Print Time: %d:%.2d", hours_estimated_print_time, minutes_estimated_print_time);
  tft.setCursor(10, line_height*2);
  tft.printf("Print Time Elapsed: %d:%.2d", hours_elapsed, minutes_elapsed);
  tft.setCursor(10, line_height*3);
  tft.printf("Print Time Left: %d:%.2d", hours, minutes);
  tft.setCursor(10, line_height*4);
  tft.printf("Complete: %d%%\n\n", completion);
  

}

void onConnectionEstablished()
{


  client.setMaxPacketSize(3000);


  client.subscribe("octoPrint/progress/printing", onTestMessageReceived);
  // client.subscribe("octoPrint/progress/printing/#", onTestMessageReceived) {
    // Serial.println("Topic: " + topic);
    // Serial.println("Payload: " + payload);

    // JsonDocument doc;

    // DeserializationError error = deserializeJson(doc, payload);

    // // Test if parsing succeeds.
    // if (error) {
    //   Serial.print(F("deserializeJson() failed: "));
    //   Serial.println(error.f_str());
    //   return;
    // }

    // const char* location = doc["location"]; // "local"
    // const char* path = doc["path"]; // "filament_dryer/rollers/hex_roller_v1.gcode"
    // int progress = doc["progress"]; // 63

    // JsonObject printer_data = doc["printer_data"];

    // const char* printer_data_state_text = printer_data["state"]["text"]; // "Printing"
    // const char* printer_data_state_error = printer_data["state"]["error"]; // nullptr

    // JsonObject printer_data_job = printer_data["job"];
    // double printer_data_job_estimatedPrintTime = printer_data_job["estimatedPrintTime"];
    // double printer_data_job_averagePrintTime = printer_data_job["averagePrintTime"]; // 9576.348718455003
    // double printer_data_job_lastPrintTime = printer_data_job["lastPrintTime"]; // 9576.348718455003
    // const char* printer_data_job_user = printer_data_job["user"]; // "gbdineen"

    // float printer_data_currentZ = printer_data["currentZ"]; // 49.95

    // JsonObject printer_data_progress = printer_data["progress"];
    // double printer_data_progress_completion = printer_data_progress["completion"]; // 61.80070861393104
    // long printer_data_progress_filepos = printer_data_progress["filepos"]; // 4918842
    // int printer_data_progress_printTime = printer_data_progress["printTime"]; // 5862
    // int printer_data_progress_printTimeLeft = printer_data_progress["printTimeLeft"]; // 3510
    // const char* printer_data_progress_printTimeLeftOrigin = printer_data_progress["printTimeLeftOrigin"];

    // JsonObject printer_data_resends = printer_data["resends"];
    // int printer_data_resends_count = printer_data_resends["count"]; // 0
    // long printer_data_resends_transmitted = printer_data_resends["transmitted"]; // 189840
    // int printer_data_resends_ratio = printer_data_resends["ratio"]; // 0

    // long timestamp = doc["_timestamp"]; // 1759880795 

    // tft.fillScreen(ILI9341_BLACK);
    // tft.setCursor(10, 20);
    // tft.setTextColor(ILI9341_WHITE); 
    // tft.setTextSize(1);
    // tft.println("Actual Temp: " + String(actual));
    // tft.setCursor(10, 40);
    // tft.println("Target Temp: " + String(target));


  // }
}

  // Publish a message to "mytopic/test"
  // client.publish("mytopic/test", "This is a message"); // You can activate the retain flag by setting the third parameter to true

  // Execute delayed instructions
  // client.executeDelayed(5 * 1000, []() {
  //   client.publish("mytopic/wildcardtest/test123", "This is a message sent 5 seconds later");
  // });


void loop()
{
  client.loop();
}
