#include <driver/adc.h>
#include "esp_adc_cal.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESPUI.h>
#include <SPI.h>
#include <SD.h>


bool sd_is_mounted = false;

const char* ssid = "Netzwerk";
const char* password = "PtK!B68%DL$rH%^5%292";

#define SD_MISO     2
#define SD_MOSI     15
#define SD_SCLK     14
#define SD_CS       13


int statusLabelId, batteryVoltageId, batteryPercentageId;
bool isRecording;

uint16_t button1, updateButton;

int calc_battery_percentage() {

  //4,2 = 100
  //3,3 = 0
  //0,9 = 100

  float voltage = get_battery_voltage();

  int percentage = float(voltage - 3.3f) * 111.11f;

  return percentage;

}


void updateButtonCallback(Control* sender, int type){

  switch (type)
  {
    case B_DOWN:
      Serial.println("Button DOWN");
      ESPUI.updateText(batteryVoltageId, String(get_battery_voltage()));
      ESPUI.updateText(batteryPercentageId, String(calc_battery_percentage()));
      break;

    case B_UP:
      Serial.println("Button UP");
      break;
  }
}


void buttonCallback(Control* sender, int type){

  switch (type)
  {
    case B_DOWN:
      if (!isRecording) {
        isRecording = true;
        ESPUI.getControl(button1)->color = ControlColor::Alizarin;
        ESPUI.updateControl(button1);
        ESPUI.updateButton(button1, "Stop recording");
        ESPUI.updateText(statusLabelId, "Recording");
      } else {
        isRecording = false;
        ESPUI.getControl(button1)->color = ControlColor::Emerald;
        ESPUI.updateControl(button1);
        ESPUI.updateButton(button1, "Start recording");
        ESPUI.updateText(statusLabelId, "Idle");
      }
      Serial.println("Button DOWN");
      break;

    case B_UP:
      Serial.println("Button UP");
      break;
  }
}



void setup_sd_card() {
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS)) {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    String str = "SDCard Size: " + String(cardSize) + "MB";
    Serial.println(str);
    sd_is_mounted = true;
  }
}



float get_battery_voltage() {

  uint16_t v = analogRead(35);
  Serial.println(v);
  float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (1100 / 1000.0);

  return battery_voltage;

}

void setup_network() {
  WiFi.mode(WIFI_STA);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

}


void setup_ui() {

  ESPUI.setVerbosity(Verbosity::VerboseJSON);

  statusLabelId = ESPUI.label("Status:", ControlColor::Turquoise, "Idle");
  batteryVoltageId = ESPUI.label("Voltage:", ControlColor::Turquoise, String(get_battery_voltage()));
  batteryPercentageId = ESPUI.label("Battery:", ControlColor::Turquoise, String(calc_battery_percentage()));
  
  button1 = ESPUI.button("Recording", &buttonCallback, ControlColor::Emerald, "Start recording");
  updateButton = ESPUI.button("Battery values", &updateButtonCallback, ControlColor::Emerald, "Update");

  ESPUI.begin("AMU Control");


}




void setup() {
  //Serial.begin(115200);

  setup_sd_card();

  //  esp_adc_cal_characteristics_t adc_chars;
  //  esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC_ATTEN_DB_2_5, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);

  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);

  setup_network();


  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();




  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected. IP address: " + WiFi.localIP());


  setup_ui();



}

void loop() {

  ArduinoOTA.handle();




}
