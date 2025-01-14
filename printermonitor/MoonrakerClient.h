/** The MIT License (MIT)

Copyright (c) 2018 David Payne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Additional Contributions:
/* 15 Jan 2019 : Owen Carter : Add psucontrol query via POST api call */

#pragma once
#include <ESP8266WiFi.h>
#include "libs/ArduinoJson/ArduinoJson.h"
#include "TimeLib.h"
#include <base64.h>

class MoonrakerClient {

private:
  char myServer[100];
  int myPort = 7125;
  String myApiKey = "";
  String encodedAuth = "";
  boolean pollPsu;
  String myETAMethod = "";
  const String printerType = "Moonraker";

  void resetPrintData();
  boolean validate();
  float avgCheck(float check, float val1, float val2);
  WiFiClient getRequest(String apiData, String apiPostBody);
  void handleParseError();
  String urlEncode(String url);
 
  String result;

  typedef struct {
    String averagePrintTime; // is this in the data?
    String estimatedPrintTime;
    String fileName;
    String fileSize;
    String lastPrintTime; // is this in the data?
    String progressCompletion;
    String progressFilepos;
    String progressPrintTime;
    String progressPrintTimeLeft;
    String state;
    String toolTemp;
    String toolTargetTemp;
    String currentLayer;
    String totalLayers;
    String estimatedEndTime;
    String filamentLength;
    String bedTemp;
    String bedTargetTemp;
    boolean isPrinting;
    boolean isPSUoff;
    String error;
    String printerName;
  } PrinterStruct;

  PrinterStruct printerData;

public:
  MoonrakerClient(String ApiKey, String server, int port, String user, String pass, boolean psu, String eta_method);
  void getPrinterJobResults();
  void getPrinterPsuState();
  void updatePrintClient(String ApiKey, String server, int port, String user, String pass, boolean psu, String eta_method);

  String getAveragePrintTime();
  String getEstimatedPrintTime();
  String getFileName();
  String getFileSize();
  String getLastPrintTime();
  String getProgressCompletion();
  String getProgressFilepos();
  String getProgressPrintTime();
  String getProgressPrintTimeLeft();
  String getState();
  boolean isPrinting();
  boolean isOperational();
  boolean isPSUoff();
  String getTempBedActual();
  String getTempBedTarget();
  String getTempToolActual();
  String getTempToolTarget();
  String getFilamentLength();
  String getEstimatedEndTime();
  String getCurrentLayer();
  String getTotalLayers();
  String getValueRounded(String value);
  String getError();
  String getPrinterType();
  int getPrinterPort();
  String getPrinterName();
  void setPrinterName(String printer);

};
