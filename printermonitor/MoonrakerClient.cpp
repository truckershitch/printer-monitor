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

#include "MoonrakerClient.h"

MoonrakerClient::MoonrakerClient(String ApiKey, String server, int port, String user, String pass, boolean psu) {
  updatePrintClient(ApiKey, server, port, user, pass, psu);
}

void MoonrakerClient::updatePrintClient(String ApiKey, String server, int port, String user, String pass, boolean psu) {
  server.toCharArray(myServer, 100);
  myApiKey = ApiKey;
  myPort = port;
  encodedAuth = "";
  if (user != "") {
    String userpass = user + ":" + pass;
    base64 b64;
    encodedAuth = b64.encode(userpass, true);
  }
  pollPsu = psu;
}

boolean MoonrakerClient::validate() {
  boolean rtnValue = false;
  printerData.error = "";
  if (String(myServer) == "") {
    printerData.error += "Server address is required; ";
  }
  //**** Moonraker API Key does not require an API Key by default
  // if (myApiKey == "") {
  //   printerData.error += "ApiKey is required; ";
  // }
  if (printerData.error == "") {
    rtnValue = true;
  }
  return rtnValue;
}

WiFiClient MoonrakerClient::getRequest(String apiData, String apiPostBody="") {
  WiFiClient printClient;
  printClient.setTimeout(5000);

  Serial.print("Getting Moonraker Data via ");
  if (apiPostBody != "") {
    Serial.println("POST");
  }
  else {
    Serial.println("GET");
  }
  Serial.println(apiData);
  result = "";
  if (printClient.connect(myServer, myPort)) {  //starts client connection, checks for connection
    printClient.println(apiData);
    printClient.println("Host: " + String(myServer) + ":" + String(myPort));
    if (myApiKey != "") {
      printClient.println("X-Api-Key: " + myApiKey);
    }
    if (encodedAuth != "") {
      printClient.print("Authorization: ");
      printClient.println("Basic " + encodedAuth);
    }
    printClient.println("User-Agent: ArduinoWiFi/1.1");
    printClient.println("Connection: close");
    if (apiPostBody != "") {
      printClient.println("Content-Type: application/json");
      printClient.print("Content-Length: ");
      printClient.println(apiPostBody.length());
      printClient.println();
      printClient.println(apiPostBody);
    }
    if (printClient.println() == 0) {
      Serial.println("Connection to " + String(myServer) + ":" + String(myPort) + " failed.");
      Serial.println();
      resetPrintData();
      printerData.error = "Connection to " + String(myServer) + ":" + String(myPort) + " failed.";
      return printClient;
    }
  } 
  else {
    Serial.println("Connection to Moonraker failed: " + String(myServer) + ":" + String(myPort)); //error message if no client connect
    Serial.println();
    resetPrintData();
    printerData.error = "Connection to Moonraker failed: " + String(myServer) + ":" + String(myPort);
    return printClient;
  }

  // Check HTTP status
  char status[32] = {0};
  printClient.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0 && strcmp(status, "HTTP/1.1 409 CONFLICT") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    printerData.state = "";
    printerData.error = "Response: " + String(status);
    return printClient;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!printClient.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    printerData.error = "Invalid response from " + String(myServer) + ":" + String(myPort);
    printerData.state = "";
  }

  return printClient;
}

void MoonrakerClient::handleParseError() {
  Serial.println("Moonraker Data Parsing failed: " + String(myServer) + ":" + String(myPort));
  printerData.error = "Moonraker Data Parsing failed: " + String(myServer) + ":" + String(myPort);
}

String MoonrakerClient::urlEncode(String url) {
  int c;
	char *hex = "0123456789abcdef";
  String encoded = "";

	for (int i=0; i < url.length(); i++) {
    c = int(url[i]);
		if( ('a' <= c && c <= 'z')
      || ('A' <= c && c <= 'Z')
      || ('0' <= c && c <= '9') ) {
      encoded += url[i];
		}
    else {
			encoded += "%";
			encoded += char(hex[c >> 4]);
			encoded += char(hex[c & 15]);
		}
	}
  return encoded;
}

void MoonrakerClient::getPrinterJobResults(float utcOffset) {
  if (!validate()) {
    return;
  }
  // progressCompletion, progressFilePos,
  // fileName, progressPrintTime, filamentLength
  String apiGetData = "GET /printer/objects/query?";
  apiGetData += "webhooks";
  apiGetData += "&extruder=temperature,target";
  apiGetData += "&heater_bed=temperature,target";
  apiGetData += "&virtual_sdcard=progress,file_position";
  apiGetData += "&print_stats=state,filename,print_duration,total_duration,filament_used";
  apiGetData += "&gcode_move=gcode_position";
  apiGetData += " HTTP/1.1";
  WiFiClient printClient = getRequest(apiGetData);
  if (printerData.error != "") {
    return;
  }
  const size_t bufferSizeMainData = 2*JSON_OBJECT_SIZE(1) + 5*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(6) + 766;
  DynamicJsonBuffer jsonBufferMainData(bufferSizeMainData);

  // Parse JSON object
  JsonObject& mainDataRoot = jsonBufferMainData.parseObject(printClient)["result"]["status"];
  if (!mainDataRoot.success()) {
    handleParseError();
    printerData.state = "";
    printerData.isPrinting = false;
    printerData.toolTemp = "";
    printerData.toolTargetTemp = "";
    printerData.bedTemp = "";
    printerData.bedTargetTemp = "";
    printerData.progressCompletion = "";
    printerData.progressFilepos = "";
    printerData.fileName = "";
    printerData.progressPrintTime = "";
    printerData.estimatedEndTime = "";
    printerData.filamentLength = "";
    printerData.averagePrintTime = "";
    printerData.lastPrintTime = "";
    return;
  }

  Serial.print("mainDataRoot: ");
  mainDataRoot.printTo(Serial);
  Serial.println();

  printerData.state = (const char*)mainDataRoot["webhooks"]["state"];
  String webhooksStateMsg = (const char*)mainDataRoot["webhooks"]["state_message"];

  if (isOperational()) {
    Serial.println("Status: " + webhooksStateMsg);
  } else {
    Serial.println("Printer Not Operational");
  }

  String printStatsState = (const char*)mainDataRoot["print_stats"]["state"];
  if (printStatsState == "printing") {
    printerData.isPrinting = true;
  }
  else {
    printerData.isPrinting = false;
  }
  printerData.toolTemp = (const char*)mainDataRoot["extruder"]["temperature"];
  printerData.toolTargetTemp = (const char*)mainDataRoot["extruder"]["target"];
  printerData.bedTemp = (const char*)mainDataRoot["heater_bed"]["temperature"];
  printerData.bedTargetTemp = (const char*)mainDataRoot["heater_bed"]["target"];
  float progressCompletion = mainDataRoot["virtual_sdcard"]["progress"].as<float>();
  printerData.progressCompletion = String(progressCompletion * 100.0);
  printerData.progressFilepos = (const char*)mainDataRoot["virtual_sdcard"]["file_position"];
  printerData.fileName = (const char*)mainDataRoot["print_stats"]["filename"];
  long progressPrintTime = mainDataRoot["print_stats"]["print_duration"].as<long>();
  printerData.progressPrintTime = String(progressPrintTime);
  long totalDuration = mainDataRoot["print_stats"]["total_duration"].as<long>();
  printerData.filamentLength = (const char*)mainDataRoot["print_stats"]["filament_used"];
  // need for layers
  float gcode_z_pos = mainDataRoot["gcode_move"]["gcode_position"][2].as<float>();
  
  // NOT USED RIGHT NOW
  printerData.averagePrintTime = "";
  printerData.lastPrintTime = "";

  if (printerData.fileName != "") {
    // fileSize, estimatedPrintTime
    // progressprintTimeLeft, estimatedEndTime,
    // totalLayers, currentLayer
    apiGetData = "GET /server/files/metadata?filename=" + urlEncode(printerData.fileName) + " HTTP/1.1";
    printClient = getRequest(apiGetData);
    if (printerData.error != "") {
      return;
    }
    const size_t BufferSizeMeta = JSON_ARRAY_SIZE(2) + 2*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(17) + 620;
    DynamicJsonBuffer jsonMetaData(BufferSizeMeta);

    // Parse JSON object
    JsonObject& metaDataRoot = jsonMetaData.parseObject(printClient);
    if (!metaDataRoot.success()) {
      handleParseError();
      printerData.fileSize = "";
      printerData.estimatedPrintTime = "";
      printerData.progressPrintTimeLeft = "";
      printerData.estimatedEndTime = "Unknown";
      printerData.totalLayers = "";
      printerData.currentLayer = "";
      return;
    }
  
    Serial.print("metaDataRoot: ");
    metaDataRoot.printTo(Serial);
    Serial.println();

    printerData.fileSize = (const char*)metaDataRoot["result"]["size"];
    // times
    long estimatedTime = metaDataRoot["result"]["estimated_time"].as<long>();
    printerData.estimatedPrintTime = String(estimatedTime);
    long progressPrintTimeLeft = ceil((1.0 - progressCompletion) * estimatedTime);
    printerData.progressPrintTimeLeft = String(progressPrintTimeLeft);
    long printStartTime = metaDataRoot["result"]["print_start_time"].as<long>();

    if (progressPrintTime > 0) {
      long eta = now() + utcOffset * 3600 +  progressPrintTimeLeft;

      char buff[32];
      int etaHour = hourFormat12(eta); // 12 hour format

      if (progressPrintTimeLeft >= 86400) {
        sprintf(buff, "%02d.%02d.%02d %02d:%02d ", month(eta), day(eta), year(eta) % 100, etaHour, minute(eta));
      }
      else {
        sprintf(buff, "%02d:%02d ", etaHour, minute(eta));
      }
      
      Serial.println("utcOffset: "  + String(utcOffset));
      Serial.println("progressPrintTimeLeft: " + String(progressPrintTimeLeft));
      Serial.println("raw eta: " + String(eta));
      Serial.println("friendly eta: ") + String(buff);
      Serial.println("printStartTime: " + String(printStartTime));
      Serial.println("totalDuration: " + String(totalDuration));
      
      printerData.estimatedEndTime = buff;
      if (isAM(eta)) {
        printerData.estimatedEndTime += "AM";
      }
      else {
        printerData.estimatedEndTime += "PM";
      }

      Serial.println("ETA: " + printerData.estimatedEndTime);
    }
    else {
      printerData.estimatedEndTime = "Unknown";
    }

    // layers
    float h = metaDataRoot["result"]["object_height"].as<float>();
    float flh = metaDataRoot["result"]["first_layer_height"].as<float>();
    float lh = metaDataRoot["result"]["layer_height"].as<float>();

    printerData.totalLayers = String((int)ceil(((h - flh) / lh) + 1));
    if (progressPrintTime > 0) {
      printerData.currentLayer = String((int)ceil((gcode_z_pos - flh) / lh + 1));
    }
    else {
      printerData.currentLayer = "0";
    }
    if (printerData.currentLayer > printerData.totalLayers) {
      printerData.currentLayer = printerData.totalLayers;
    }
  }  

  if (isPrinting()) {
    Serial.println("Status: " + printerData.state + " " + printerData.fileName + "(" + printerData.progressCompletion + "%)");
  }
}

void MoonrakerClient::getPrinterPsuState() {
  // TODO: define name of printer power object in config
  // for now, hardcode to "printer"

  /*
  GET /machine/device_power/device?device=printer
  returns {"printer": "off"}
  */

  //**** get the PSU state (if enabled and printer operational)
  if (pollPsu && isOperational()) {
    if (!validate()) {
      printerData.isPSUoff = false; // we do not know PSU state, so assume on.
      return;
    }
    String apiGetData = "GET /machine/device_power/device?device=printer HTTP/1.1";
    WiFiClient printClient = getRequest(apiGetData);
    if (printerData.error != "") {
      printerData.isPSUoff = false; // we do not know PSU state, so assume on.
      return;
    }
    const size_t bufferSizePSU = JSON_OBJECT_SIZE(2) + 300;
    DynamicJsonBuffer jsonBufferPSU(bufferSizePSU);
  
    // Parse JSON object
    JsonObject& psuRoot = jsonBufferPSU.parseObject(printClient);
    if (!psuRoot.success()) {
      printerData.isPSUoff = false; // we do not know PSU state, so assume on
      return;
    }

    Serial.print("psuRoot: ");
    psuRoot.printTo(Serial);
    Serial.println();
  
    String psu = (const char*)psuRoot["printer"];
    if (psu == "on") {
      printerData.isPSUoff = false; // PSU checked and is on
    }
    else {
      printerData.isPSUoff = true; // PSU checked and is off, set flag
    }
    printClient.stop(); //stop client
  }
  else {
    printerData.isPSUoff = false; // we are not checking PSU state, so assume on
  }
}

// Reset all PrinterData
void MoonrakerClient::resetPrintData() {
  printerData.averagePrintTime = "";
  printerData.estimatedPrintTime = "";
  printerData.fileName = "";
  printerData.fileSize = "";
  printerData.lastPrintTime = "";
  printerData.progressCompletion = "";
  printerData.progressFilepos = "";
  printerData.progressPrintTime = "";
  printerData.progressPrintTimeLeft = "";
  printerData.state = "";
  printerData.toolTemp = "";
  printerData.toolTargetTemp = "";
  printerData.filamentLength = "";
  printerData.bedTemp = "";
  printerData.bedTargetTemp = "";
  printerData.isPrinting = false;
  printerData.isPSUoff = false;
  printerData.error = "";
  printerData.currentLayer = "";
  printerData.totalLayers = "";
  printerData.estimatedEndTime = "";
}

String MoonrakerClient::getCurrentLayer(){
  return printerData.currentLayer;
}

String MoonrakerClient::getTotalLayers(){
  return printerData.totalLayers;
}

String MoonrakerClient::getEstimatedEndTime(){
  return printerData.estimatedEndTime;
}

String MoonrakerClient::getAveragePrintTime(){
  return printerData.averagePrintTime;
}

String MoonrakerClient::getEstimatedPrintTime() {
  return printerData.estimatedPrintTime;
}

String MoonrakerClient::getFileName() {
  return printerData.fileName;
}

String MoonrakerClient::getFileSize() {
  return printerData.fileSize;
}

String MoonrakerClient::getLastPrintTime(){
  return printerData.lastPrintTime;
}

String MoonrakerClient::getProgressCompletion() {
  return String(printerData.progressCompletion.toInt());
}

String MoonrakerClient::getProgressFilepos() {
  return printerData.progressFilepos;  
}

String MoonrakerClient::getProgressPrintTime() {
  return printerData.progressPrintTime;
}

String MoonrakerClient::getProgressPrintTimeLeft() {
  String rtnValue = printerData.progressPrintTimeLeft;
  // if (getProgressCompletion() == "100") {
  //   rtnValue = "0"; // Print is done so this should be 0 this is a fix for Octoprint
  // }
  return rtnValue;
}

String MoonrakerClient::getState() {
  String state;
  if (printerData.state == "ready") {
    state = "Operational";
  }
  else {
    state = printerData.state;
  }
  return state;
}

boolean MoonrakerClient::isPrinting() {
  return printerData.isPrinting;
}

boolean MoonrakerClient::isPSUoff() {
  return printerData.isPSUoff;
}

boolean MoonrakerClient::isOperational() {
  boolean operational = false;
  if (printerData.state == "startup" || printerData.state == "ready" || isPrinting()) {
    operational = true;
  }
  return operational;
}

String MoonrakerClient::getTempBedActual() {
  return printerData.bedTemp;
}

String MoonrakerClient::getTempBedTarget() {
  return printerData.bedTargetTemp;
}

String MoonrakerClient::getTempToolActual() {
  return printerData.toolTemp;
}

String MoonrakerClient::getTempToolTarget() {
  return printerData.toolTargetTemp;
}

String MoonrakerClient::getFilamentLength() {
  return printerData.filamentLength;
}

String MoonrakerClient::getError() {
  return printerData.error;
}

String MoonrakerClient::getValueRounded(String value) {
  float f = value.toFloat();
  int rounded = (int)(f+0.5f);
  return String(rounded);
}

String MoonrakerClient::getPrinterType() {
  return printerType;
}

int MoonrakerClient::getPrinterPort() {
  return myPort;
}

String MoonrakerClient::getPrinterName() {
  return printerData.printerName;
}

void MoonrakerClient::setPrinterName(String printer) {
  printerData.printerName = printer;
}
