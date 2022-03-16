#include <VariableTimedAction.h>
#include <RTClib.h>
#include <SdFat.h>
#include <SPI.h>
#include <TMRpcm.h>
#include <BME280I2C.h>
#include <Wire.h>

#define SD_ChipSelectPin 53



BME280I2C bme;

float temp(NAN), hum(NAN), pres(NAN);

SdFat SD;
File file;


// Fehlermeldungen im Flash ablegen.
#define error(msg) SD.errorHalt(F(msg))


RTC_DS3231 rtc;
TMRpcm audio;
DateTime now;
char date [8] = "";
char the_time [10] = "";
String activeRecordingPath;
String date_string, time_string;

float freeSpace;





void logBME280Data() {

  if (!SD.exists("data/bme280.csv")) {
    Serial.println("No data file found. Trying to create");
  }

  file = SD.open("data/bme280.csv", FILE_WRITE);

  if (!file) {
    Serial.println("Error opening data/bme280.csv"); while (1);
  }

  now = rtc.now();
  bme.read(pres, temp, hum);
  file.print(now.day()); file.print("."); file.print(now.month()); file.print("."); file.print(now.year()); file.print(",");
  file.print(now.hour()); file.print(":"); file.print(now.minute()); file.print(":"); file.print(now.second());
  file.print(","); file.print(temp); file.print(","); file.print(hum); file.print(","); file.println(pres);
  // Filesync to avoid data loss
  if (!file.sync() || file.getWriteError()) {
    error("Write error!");
  }

  file.close();

  Serial.print("Logging ");
  Serial.print(now.day()); Serial.print("."); Serial.print(now.month()); Serial.print("."); Serial.print(now.year()); Serial.print(",");
  Serial.print(now.hour()); Serial.print(":"); Serial.print(now.minute()); Serial.print(":"); Serial.print(now.second());
  Serial.print(","); Serial.print(temp); Serial.print(","); Serial.print(hum); Serial.print(","); Serial.print(pres);
  Serial.println(" to /data/bme280.csv");


  

}


class DataLogger : public VariableTimedAction {
  private:
    unsigned long run() {
      logBME280Data(); return 0;
    }
  public:
};

class SdWarning : public VariableTimedAction {
  private:
    unsigned long run() {
      Serial.println("+++++++++++++++++++++++++++++++++++++");
      Serial.println("Do not remove sd card."); 
      return 0;
    }
  public:
};

class SdSafe : public VariableTimedAction {
  private:
    unsigned long run() {
      Serial.println("Sd card can be removed now"); 
      Serial.println("+++++++++++++++++++++++++++++++++++++");
      return 0;
    }
  public:
};


DataLogger tenSecondsCounter; 
SdWarning sdWarningCounter;
SdSafe sdSafeRemoveCounter;


void setup() {

  Serial.begin(115200); delay(500);
  audio.CSPin = SD_ChipSelectPin; // The audio library needs to know which CS pin to use for recording

  setupRTC();
  setupBME280();
  setupSD();

  printInfo();

  sdWarningCounter.start(55000); // warns before writing to card
  tenSecondsCounter.start(60000); // runs every minute
  sdSafeRemoveCounter.start(65000); // safe to remove sd card

  

}


void loop() {

  if (Serial.available()) {
    switch (Serial.read()) {
      case 'r': startRecording(); break;
      case 's': stopRecording(); break;
      case 'l': tenSecondsCounter.toggleRunning(); if (tenSecondsCounter.isRunning()) {
          Serial.println("Logging enabled!");
        } else {
          Serial.println("Logging disabled!");
        } break;
    }
  }

  VariableTimedAction::updateActions();



}




void startRecording() {

  String recPath = "/recordings/";
  recPath += buildRtcString("-", "date");
  recPath += "/";

  Serial.println("Checking if folder for recording exists...");

  if (!SD.exists(recPath)) {
    Serial.print(recPath);
    Serial.print(" not found. Trying to create... ");
    if (!SD.mkdir(recPath)) {
      Serial.println("Failed"); while (1);
    } else {
      Serial.print("Folder ");
      Serial.print(recPath);
      Serial.println(" Success");
    }

  } else {
    Serial.print("Found recording folder for today: ");
    Serial.println(recPath);
  }


  Serial.println("Start recording!");

  String fileName = buildRtcString("-", "time");
  fileName += ".wav";
  recPath += fileName;

  audio.startRecording(recPath.c_str(), 16000, A0);  //Record at 16khz sample rate on pin A0

  activeRecordingPath = recPath;

}


void printBME280Data() {
  bme.read(pres, temp, hum);
  Serial.print("Temperature: "); Serial.print(temp); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(hum); Serial.println(" % RH");
  Serial.print("Pressure: "); Serial.print(pres); Serial.println(" Pa");
}



void printInfo() {
  Serial.println("");
  Serial.println("#####################################");

  printRTCData();
  printBME280Data();
  //printSdData();

  Serial.println("#####################################");
  Serial.println("");
}

void printSdData() {
  Serial.print("Free disk space: ");
  Serial.print(freeSpace);
  Serial.println(" Gb");
}




void printRTCData() {
  Serial.print("Date: "); Serial.println(buildRtcString("/", "date"));
  Serial.print("Time: "); Serial.println(buildRtcString(":", "time"));
}






void stopRecording() {
  Serial.println("Stop recording!");
  Serial.print("File saved at ");
  Serial.println(activeRecordingPath);
  audio.stopRecording(activeRecordingPath.c_str());
}




String buildRtcString(String seperator, String mode) {

  now = rtc.now();

  if (mode == "date") {
    char dateBuffer[30];
    sprintf(dateBuffer, "%02dS%02dS%04d", now.day(), now.month(), now.year());
    String dateString(dateBuffer);
    dateString.replace("S", seperator);
    return dateString;
  } else if (mode == "time") {
    char timeBuffer[30];
    sprintf(timeBuffer, "%02dS%02dS%02d", now.hour(), now.minute(), now.second());
    String timeString(timeBuffer);
    timeString.replace("S", seperator);
    return timeString;
  }

}




void setupBME280() {
  Wire.begin();
  while (!bme.begin()) {
    Serial.println("Could not find BME280 sensor!"); while (1);
  }
  Serial.println("Found BME280 sensor!");
}


void setupRTC() {
  if (rtc.begin()) {
    // February 14, 2022 at 11:50am you would call: rtc.adjust(DateTime(2022, 2, 14, 11, 50, 0));
    Serial.println("RTC found");
  } else {
    Serial.println("RTC not found check wiring"); while (1);
  }
}


void setupSD() {
  if (!SD.begin(SD_ChipSelectPin, SPI_HALF_SPEED)) {
    Serial.print("SD not found check wiring"); while (1);
  } else {

    if (!SD.exists("recordings")) {
      Serial.println("No recordings folder found. Trying to create folder.");
      if (!SD.mkdir("recordings")) {
        Serial.println("Create recordings folder failed"); while (1);
      } else {
        Serial.println("Folder recordings successfully created.");
      }
    } else {
      Serial.println("Found recordings folder.");
    }


    if (!SD.exists("data")) {
      Serial.println("No data folder found. Trying to create folder.");
      if (!SD.mkdir("data")) {
        Serial.println("Create data folder failed"); while (1);
      } else {
        Serial.println("Folder data successfully created.");
      }
    } else {
      Serial.println("Found data folder.");
    }

  }

  //calculateFreeSpace();

  Serial.println("SD setup done");

}

void calculateFreeSpace() {
  // calculate free disk space (takes some time)
  long freeKB = SD.vol()->freeClusterCount();
  freeKB *= SD.vol()->sectorsPerCluster() / 2;
  freeSpace = float(freeKB) / (float)1024 / (float)1000;
}
