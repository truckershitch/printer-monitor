/*
  NTPuClient.cpp
  Baseed on code by Andreas Spiess
  See his YouTube video #299
  https://youtu.be/r2UAmBLBBRM

  Created July 13, 2022
*/

#include "TimeHandler.h"

TimeHandler::TimeHandler(float utcOffset, String TZ, String ntpServer) {
  myUtcOffset = utcOffset;
  myNtpServer = String(ntpServer);
  myTZ = TZ;
}

void TimeHandler::updateTime() {
  Serial.println("Fetching NTP Time from NTP Server " + myNtpServer + "\n");

  // TEMPORARILY DISABLING TIME ZONE -- USING myUtcOffset
  // configTime(0, 0, NTP_SERVER);
  // See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  // setenv("TZ", tz.c_str(), 1);
  configTime(myUtcOffset * 3600, 0, myNtpServer.c_str());  

  if (TimeHandler::getNTPtime(10)) {  // wait up to 10sec to sync
  } else {
    Serial.println("\nTime not set");
    ESP.restart();
  }
  setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year - 100);
  adjustTime(1); // add one second to allow for lag
  TimeHandler::showTime(timeinfo);
  lastNTPtime = time(&now);
  lastEntryTime = millis();
}

bool TimeHandler::getNTPtime(int sec) {

  {
    uint32_t start = millis();
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      Serial.print(".");
      delay(10);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
    Serial.print("now ");  Serial.println(now);
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
    Serial.println(time_output);
    Serial.println();
  }
  return true;
}

// void TimeHandler::setTZ(String tz_info) {
//   tz = tz_info;
// }

void TimeHandler::setUtcOffset(float utcOffset) {
  myUtcOffset = utcOffset;
}

// This function is obsolete because the time() function only calls the NTP server every hour. So you can always use getNTPtime()
// It can be deleted and only stays here for the video

/*
  void getTimeReducedTraffic(int sec) {
  tm *ptm;
  if ((millis() - lastEntryTime) < (1000 * sec)) {
    now = lastNTPtime + (int)(millis() - lastEntryTime) / 1000;
  } else {
    lastEntryTime = millis();
    lastNTPtime = time(&now);
    now = lastNTPtime;
    Serial.println("Get NTP time");
  }
  ptm = localtime(&now);
  timeinfo = *ptm;
  }
*/

void TimeHandler::showTime(tm localTime) {
  Serial.print(localTime.tm_mday);
  Serial.print('/');
  Serial.print(localTime.tm_mon + 1);
  Serial.print('/');
  Serial.print(localTime.tm_year - 100);
  Serial.print('-');
  Serial.print(localTime.tm_hour);
  Serial.print(':');
  Serial.print(localTime.tm_min);
  Serial.print(':');
  Serial.print(localTime.tm_sec);
  Serial.print(" Day of Week ");
  if (localTime.tm_wday == 0)   Serial.println(7);
  else Serial.println(localTime.tm_wday);
}

String TimeHandler::getDisplayTime(long timeinfo, bool is24Hr, bool withSecs) {
  String dispHours = is24Hr ? String(hour(timeinfo)) : String(hourFormat12(timeinfo));
  String dispMins = minute(timeinfo) < 10 ? "0" + String(minute(timeinfo)) : String(minute(timeinfo));
  String dispSecs = second(timeinfo) < 10 ? "0" + String(second(timeinfo)) : String(second(timeinfo));
  String displayTime = withSecs ? dispHours + ":" + dispMins + ":" + dispSecs : dispHours + ":" + dispMins;

  return displayTime;
}

String TimeHandler::getAMPM(long timeinfo) {
  String result = isAM(timeinfo) ? "AM" : "PM";
  return result;
}