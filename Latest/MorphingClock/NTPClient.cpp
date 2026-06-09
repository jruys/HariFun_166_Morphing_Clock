#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <DoubleResetDetector.h>
#include "FS.h"
#include <ArduinoJson.h> // Ensure you are using ArduinoJson V6 or higher
#include <WiFiUdp.h>
#include "NTPClient.h"
#include "TinyFont.h"
#include <time.h>        // Required for dynamic Unix time breakdown struct mappings

#define DEBUG 0
#define DRD_TIMEOUT 10 
#define DRD_ADDRESS 0 

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
char wifiManagerAPName[] = "MorphClk";
char wifiManagerAPPassword[] = "MorphClk";
bool shouldSaveConfig = false; 

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

const unsigned long askFrequency = 60 * 60 * 1000; 
unsigned long timeToAsk;
unsigned long timeToRead;
unsigned long lastEpoch; 
unsigned long lastEpochTimeStamp; 
unsigned long nextEpochTimeStamp; 
unsigned long currentTime;

char homeWifiName[] = ""; 
char homeWifiPassword[] = ""; 
char timezone[5] = "0"; 
bool military; 

// Persistent System Configuration Variables
int displayColorMode = 2; // Default Blue
int morphFade = 30;       // Default 30ms
int nightModeStart = 22;  // Default 22
int nightModeEnd = 7;     // Default 7

const char* ntpServerName = "pool.ntp.org";
IPAddress timeServerIP; 
const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[NTP_PACKET_SIZE]; 
WiFiUDP udp; 
unsigned int localPort = 2390;      

char configFilename[] = "/config.json";
bool error_getTime = false;

// =======================================================================================
// Daylight Saving Time Helper Engine (European Rules Implementation)
// Change rules if in US: 2nd Sunday of March to 1st Sunday of November
// =======================================================================================
bool isEuropeanDST(time_t utcTime) {
  struct tm *timeinfo = gmtime(&utcTime);
  int month = timeinfo->tm_mon + 1; // tm_mon is 0-11
  int day = timeinfo->tm_mday;
  int hour = timeinfo->tm_hour;     // Raw UTC Hour
  int wday = timeinfo->tm_wday;     // 0 = Sunday, 1 = Monday...

  // Jan, Feb, Nov, Dec are always Standard/Winter Time
  if (month < 3 || month > 10) return false;
  // Apr, May, Jun, Jul, Aug, Sep are always Summer/DST Time
  if (month > 3 && month < 10) return true;

  // March and October require finding the absolute last Sunday of the month
  // The last Sunday of March/October always falls between the 25th and 31st
  int lastSunday = day - wday; 
  while (lastSunday + 7 <= 31) {
    lastSunday += 7;
  }

  if (month == 3) { // March transition (Clocks go forward)
    if (day > lastSunday) return true;
    if (day < lastSunday) return false;
    return hour >= 1; // Starts at 01:00 UTC
  } else {          // October transition (Clocks go back)
    if (day > lastSunday) return false;
    if (day < lastSunday) return true;
    return hour < 1;  // Ends at 01:00 UTC
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  drd.stop();
}

bool loadConfig() {
  Serial.println("=== Loading Config ===");
  File configFile = SPIFFS.open(configFilename, "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, configFile);

  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }
  
  if (doc.containsKey("timezone")) strncpy(timezone, doc["timezone"], sizeof(timezone));
  if (doc.containsKey("military")) military = doc["military"];
  if (doc.containsKey("colorMode")) displayColorMode = doc["colorMode"];
  if (doc.containsKey("fade")) morphFade = doc["fade"];
  if (doc.containsKey("nm_start")) nightModeStart = doc["nm_start"];
  if (doc.containsKey("nm_end")) nightModeEnd = doc["nm_end"];
  
  return true;
}

bool saveConfig() {
  Serial.println("=== Saving Config ===");
  File configFile = SPIFFS.open(configFilename, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  StaticJsonDocument<512> doc;
  doc["timezone"] = timezone;
  doc["military"] = military;
  doc["colorMode"] = displayColorMode;
  doc["fade"] = morphFade;
  doc["nm_start"] = nightModeStart;
  doc["nm_end"] = nightModeEnd;

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write to file");
    return false;
  }
  return true;
}

NTPClient::NTPClient() {}

void NTPClient::Setup(PxMATRIX* d) {
  char tstr[32];
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount FS");
    return;
  }

  loadConfig();
  _display = d;
  _display->fillScreen(_display->color565(0, 0, 0));
  _display->setTextColor(_display->color565(0, 255, 0));
  
  const byte row0 = 2;
  const byte row1 = 12;
  const byte row2 = 22;

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  int connectionStatus = WL_IDLE_STATUS;

  if (strlen(homeWifiName) > 0) {
    _display->setCursor(2, row1);
    _display->print("Connecting");
    connectionStatus = WiFi.begin(homeWifiName, homeWifiPassword);
  } else {
    if (drd.detectDoubleReset()) {
      Serial.println("DOUBLE Reset Detected");
      _display->setCursor(1, row0); _display->print("AP : "); _display->print(wifiManagerAPName);
      _display->setCursor(1, row1); _display->print("Pw : "); _display->print(wifiManagerAPPassword);
      _display->setCursor(1, row2); _display->print("192.168.4.1");

      WiFi.disconnect();
      connectionStatus = wifiManager.startConfigPortal(wifiManagerAPName, wifiManagerAPPassword);
      _display->fillScreen(_display->color565(0, 0, 0));
    } else {
      Serial.println("SINGLE reset Detected");
      _display->setCursor(2, row1);
      _display->print("Connecting");
      connectionStatus = wifiManager.autoConnect();
    }
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  _display->fillScreen(_display->color565(0, 0, 0));
  _display->setCursor(2, row0);
  _display->print("Connected!");

  udp.begin(localPort);

  _display->setCursor(2, row1);
  _display->print("TZ:");
  if(strlen(timezone) < 5) _display->print(timezone);

  sprintf(tstr, "IP : %s", WiFi.localIP().toString().c_str());
  TFDrawText(_display, tstr, 1, 22, _display->color565(0, 255, 0));

  if (shouldSaveConfig) {
    saveConfig();
  }
  drd.stop();
  delay(3000);
}

unsigned long NTPClient::sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   
  packetBuffer[1] = 0;     
  packetBuffer[2] = 6;     
  packetBuffer[3] = 0xEC;  
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  udp.beginPacket(address, 123); 
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  return 0;
}

void NTPClient::AskCurrentEpoch() {
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP); 
}

unsigned long NTPClient::ReadCurrentEpoch() {
  int cb = udp.parsePacket();
  if (!cb) {
    error_getTime = false;
    return 0;
  } else {
    error_getTime = true;
    udp.read(packetBuffer, NTP_PACKET_SIZE); 

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    
    const unsigned long seventyYears = 2208988800UL;
    lastEpoch = secsSince1900 - seventyYears; 
    lastEpochTimeStamp = nextEpochTimeStamp; 

    return lastEpoch;
  }
}

unsigned long NTPClient::GetCurrentTime(bool force) {
  unsigned long timeNow = millis();
  if (force || (timeNow > timeToAsk || !error_getTime)) { 
    timeToAsk = timeNow + askFrequency; 
    if (force || (timeToRead == 0)) { 
      timeToRead = timeNow + 1000; 
      AskCurrentEpoch(); 
      nextEpochTimeStamp = millis(); 
    }
  }

  if (force || (timeToRead > 0 && timeNow > timeToRead)) {
    ReadCurrentEpoch(); 
    timeToRead = 0; 
  }

  if (force || (lastEpoch != 0)) {  
    // Calculate raw UTC timestamp first
    unsigned long elapsedMillis = millis() - lastEpochTimeStamp;
    unsigned long rawUtcTime = lastEpoch + (elapsedMillis / 1000);
    
    // Core baseline timezone translation offset from Web configuration profile
    unsigned long zoneOffset = String(timezone).toInt() * 3600;
    
    // Evaluate and execute dynamic DST shifting modifications automatically
    if (isEuropeanDST((time_t)rawUtcTime)) {
      zoneOffset += 3600; // Shift ahead 1 hour during summer months
    }
    
    currentTime = rawUtcTime + zoneOffset;
  }
  return currentTime;
}

byte NTPClient::GetHours() {
  int hours = (currentTime % 86400L) / 3600;
  if (!military) {
    if (hours == 0) hours = 12; 
    if (hours > 12) hours -= 12; 
  }
  return hours;
}

byte NTPClient::GetMinutes() { return (currentTime % 3600) / 60; }
byte NTPClient::GetSeconds() { return currentTime % 60; }

void NTPClient::PrintTime() {
  if (DEBUG) {
    Serial.print("The local time is ");      
    byte hh = GetHours();
    byte mm = GetMinutes();
    byte ss = GetSeconds();
    Serial.print(hh); Serial.print(':');
    if (mm < 10) Serial.print('0');
    Serial.print(mm); Serial.print(':');
    if (ss < 10) Serial.print('0');
    Serial.println(ss);
  }
}