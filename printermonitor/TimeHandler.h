#pragma once

#include <ESP8266WiFi.h>
#include <time.h>
#include "TimeLib.h"

class TimeHandler {
    private:
        String myNtpServer = "pool.ntp.org";
        float myUtcOffset = 0; // UTC
        String myTZ = "EST5EDT,M3.2.0,M11.1.0"; // US Eastern Time / New York

        tm timeinfo;
        time_t now;
        long unsigned lastNTPtime;
        unsigned long lastEntryTime;

        bool getNTPtime(int sec);
        void showTime(tm localTime);

    public:
        TimeHandler(float utcOffset, String TZ, String ntpServer);
        void updateTime();
        void setUtcOffset(float utcOffset);
        String getDisplayTime(long timeinfo, bool is24Hr, bool withSecs);
        String getAMPM(long timeinfo);
};