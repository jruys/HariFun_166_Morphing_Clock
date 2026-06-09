// =======================================================================================
// Morphing Clock with Web Configuration Server - Persistent Memory Optimized Edition
// Prevents dynamic heap fragmentation, handles double buffering, and syncs configs with Flash.
// =======================================================================================

#define PxMATRIX_DOUBLE_BUFFER // Enable showBuffer() in PxMatrix
#include <PxMatrix.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "Digit.h"
#include "NTPClient.h"
#include "TinyFont.h"

// --- Direct Linkage to Unified Persistent Configuration Engine Variables inside NTPClient.cpp ---
extern char timezone[5];
extern bool military;
extern int displayColorMode;
extern int morphFade;
extern int nightModeStart;
extern int nightModeEnd;
extern bool saveConfig();

// --- Hardware Matrix Wiring Pins (ESP8266 Configuration) ---
#define P_LAT 16
#define P_A   5
#define P_B   4
#define P_C   15
#define P_D   12
#define P_E   0
#define P_OE  2

PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);
Ticker display_ticker;

// Interrupt Service Routine for Matrix Redraws
void display_updater() {
  display.display(70);
}

#define NUM_DIGITS 6
Digit* Digits[NUM_DIGITS];

ESP8266WebServer server(80);
NTPClient ntpClient;

unsigned long prevEpoch = 0;
byte prevhh = 99, prevmm = 99, prevss = 99;

// Helper to convert selection mode to RGB565 color format
uint16_t getDisplayColor(int mode) {
  switch (mode) {
    case 0: return display.color565(255, 0, 0);     // Red
    case 1: return display.color565(0, 255, 0);     // Green
    case 2: return display.color565(0, 0, 255);     // Blue
    case 3: return display.color565(255, 255, 255); // White
    case 4: return display.color565(255, 255, 0);   // Yellow
    case 5: return display.color565(0, 255, 255);   // Cyan
    case 6: return display.color565(255, 0, 255);   // Purple
    default: return display.color565(0, 0, 255);    // Default to Blue
  }
}

// Helper to instantly push color/time updates across both canvas buffers
void updateDisplayColors() {
  uint16_t newColor = getDisplayColor(displayColorMode);
  for (int i = 0; i < NUM_DIGITS; i++) {
    Digits[i]->SetColor(newColor);
  }
  
  if (prevEpoch != 0) {
    int hh = ntpClient.GetHours();
    int mm = ntpClient.GetMinutes();
    int ss = ntpClient.GetSeconds();
    
    // Force immediate draw across front buffer channel
    display.clearDisplay();
    Digits[0]->Draw(ss % 10); Digits[1]->Draw(ss / 10);
    Digits[2]->Draw(mm % 10); Digits[3]->Draw(mm / 10);
    Digits[4]->Draw(hh % 10); Digits[5]->Draw(hh / 10);
    Digits[1]->DrawColon(newColor); Digits[3]->DrawColon(newColor);
    display.showBuffer();
    
    // Force immediate draw across back buffer channel
    display.clearDisplay();
    Digits[0]->Draw(ss % 10); Digits[1]->Draw(ss / 10);
    Digits[2]->Draw(mm % 10); Digits[3]->Draw(mm / 10);
    Digits[4]->Draw(hh % 10); Digits[5]->Draw(hh / 10);
    Digits[1]->DrawColon(newColor); Digits[3]->DrawColon(newColor);
    display.showBuffer();
  }
}

// --- Web Server Response Engine ---
void page_out(void) {
  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader(F("Pragma"), F("no-cache"));
  server.sendHeader(F("Expires"), F("0"));
  
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("text/html"), F(""));

  server.sendContent(F("<!DOCTYPE html><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\"><title>Clock</title>"));
  server.sendContent(F("<style>body{font-family:sans-serif;background:#121212;color:#e0e0e0;padding:20px;max-width:400px;margin:0 auto}h2{color:#3498db}label{display:block;margin:15px 0 5px;font-weight:bold}input[type=number],select{background:#1e1e1e;border:1px solid #333;color:#fff;padding:8px;border-radius:4px;width:100%;box-sizing:border-box}input[type=checkbox]{transform:scale(1.3);margin-right:10px;vertical-align:middle}.btn{padding:10px 20px;border:0;border-radius:4px;color:#fff;font-weight:bold;cursor:pointer;margin-top:15px}.send{background:#3498db}</style>"));
  server.sendContent(F("<h2>MorphingClock Configuration</h2><form method=get>"));

  server.sendContent(F("<label><input type=checkbox name=HMOD24"));
  if (military) server.sendContent(F(" checked"));
  server.sendContent(F("> 24h Mode</label>"));
  
  char buf[64];

  server.sendContent(F("<label>GMT Timezone Offset (e.g. -5, 0, 1):</label>"));
  snprintf(buf, sizeof(buf), "<input type=number name=TZ min=-12 max=14 value=\"%s\">", timezone);
  server.sendContent(buf);

  server.sendContent(F("<label>Night Mode Start Hour (0-23):</label>"));
  snprintf(buf, sizeof(buf), "<input type=number name=NMSTART min=0 max=23 value=\"%02d\">", nightModeStart);
  server.sendContent(buf);

  server.sendContent(F("<label>Night Mode End Hour (0-23):</label>"));
  snprintf(buf, sizeof(buf), "<input type=number name=NMEND min=0 max=23 value=\"%02d\">", nightModeEnd);
  server.sendContent(buf);

  server.sendContent(F("<label>Morph Speed / Delay (ms):</label>"));
  snprintf(buf, sizeof(buf), "<input type=number name=FADE min=5 max=500 value=\"%d\">", morphFade);
  server.sendContent(buf);

  server.sendContent(F("<label>Display Color:</label>"));
  server.sendContent(F("<select name=COLOR>"));
  
  server.sendContent(F("<option value=\"red\"")); if (displayColorMode == 0) server.sendContent(F(" selected")); server.sendContent(F(">Red</option>"));
  server.sendContent(F("<option value=\"green\"")); if (displayColorMode == 1) server.sendContent(F(" selected")); server.sendContent(F(">Green</option>"));
  server.sendContent(F("<option value=\"blue\"")); if (displayColorMode == 2) server.sendContent(F(" selected")); server.sendContent(F(">Blue</option>"));
  server.sendContent(F("<option value=\"white\"")); if (displayColorMode == 3) server.sendContent(F(" selected")); server.sendContent(F(">White</option>"));
  server.sendContent(F("<option value=\"yellow\"")); if (displayColorMode == 4) server.sendContent(F(" selected")); server.sendContent(F(">Yellow</option>"));
  server.sendContent(F("<option value=\"cyan\"")); if (displayColorMode == 5) server.sendContent(F(" selected")); server.sendContent(F(">Cyan</option>"));
  server.sendContent(F("<option value=\"purple\"")); if (displayColorMode == 6) server.sendContent(F(" selected")); server.sendContent(F(">Purple</option>"));
  
  server.sendContent(F("</select>"));

  server.sendContent(F("<br><input type=submit class=\"btn send\" value=\"Save Settings\"></form>"));
  server.sendContent(F("")); 
}

void handle_args() {
  bool coreConfigChanged = false;

  if (server.hasArg("HMOD24")) {
    if (!military) { military = true; coreConfigChanged = true; }
  } else if (server.args() > 0) {
    if (military) { military = false; coreConfigChanged = true; }
  }
  
  if (server.hasArg("NMSTART")) {
    int val = server.arg("NMSTART").toInt();
    if (val != nightModeStart) { nightModeStart = val; coreConfigChanged = true; }
  }
  if (server.hasArg("NMEND")) {
    int val = server.arg("NMEND").toInt();
    if (val != nightModeEnd) { nightModeEnd = val; coreConfigChanged = true; }
  }
  if (server.hasArg("FADE")) {
    int val = server.arg("FADE").toInt();
    if (val != morphFade) { morphFade = val; coreConfigChanged = true; }
  }
  
  if (server.hasArg("TZ") || server.hasArg("tz")) {
    String tzArg = server.hasArg("TZ") ? server.arg("TZ") : server.arg("tz");
    if (tzArg != String(timezone)) {
      strncpy(timezone, tzArg.c_str(), sizeof(timezone) - 1);
      timezone[sizeof(timezone) - 1] = '\0';
      coreConfigChanged = true;
    }
  }

  if (server.hasArg("COLOR") || server.hasArg("color")) {
    String cArg = server.hasArg("COLOR") ? server.arg("COLOR") : server.arg("color");
    cArg.toLowerCase();
    int oldColor = displayColorMode;
    
    if (cArg == "red" || cArg == "0") displayColorMode = 0;
    else if (cArg == "green" || cArg == "1") displayColorMode = 1;
    else if (cArg == "blue" || cArg == "2") displayColorMode = 2;
    else if (cArg == "white" || cArg == "3") displayColorMode = 3;
    else if (cArg == "yellow" || cArg == "4") displayColorMode = 4;
    else if (cArg == "cyan" || cArg == "5") displayColorMode = 5;
    else if (cArg == "purple" || cArg == "6") displayColorMode = 6;
    
    if (displayColorMode != oldColor) coreConfigChanged = true;
  }

  // If any values changed, save the configurations to storage cleanly
  if (coreConfigChanged) {
    saveConfig();
    ntpClient.GetCurrentTime(true); // Forces timezone offset recalculations instantly
  }
  
  updateDisplayColors();
  page_out();
}

// =======================================================================================
// System Initialization
// =======================================================================================
void setup() {
  Serial.begin(115200);
  
  display.begin(16);
  display_ticker.attach(0.002, display_updater);
  
  // This executes loadConfig() internally, reading saved parameters out of SPIFFS Flash
  ntpClient.Setup(&display);
  
  uint16_t activeColor = getDisplayColor(displayColorMode);
  Digits[0] = new Digit(&display, 0, 63 - 1 - 9*1, 8, activeColor); // Seconds Units
  Digits[1] = new Digit(&display, 0, 63 - 1 - 9*2, 8, activeColor); // Seconds Tens
  Digits[2] = new Digit(&display, 0, 63 - 4 - 9*3, 8, activeColor); // Minutes Units
  Digits[3] = new Digit(&display, 0, 63 - 4 - 9*4, 8, activeColor); // Minutes Tens
  Digits[4] = new Digit(&display, 0, 63 - 7 - 9*5, 8, activeColor); // Hours Units
  Digits[5] = new Digit(&display, 0, 63 - 7 - 9*6, 8, activeColor); // Hours Tens
  
  // Purge lingering configuration text artifacts fully from both hardware buffers
  display.clearDisplay(); display.showBuffer();
  display.clearDisplay(); display.showBuffer();
  
  server.on("/", handle_args);
  server.begin();
}

// =======================================================================================
// Core Asynchronous Processing Loop
// =======================================================================================
void loop() {
  server.handleClient();
  
  unsigned long epoch = ntpClient.GetCurrentTime(false);
  if (epoch != prevEpoch && epoch != 0) {
    int hh = ntpClient.GetHours();
    int mm = ntpClient.GetMinutes();
    int ss = ntpClient.GetSeconds();
    
    // First hardware synchronization point
    if (prevEpoch == 0) {
      uint16_t colonColor = getDisplayColor(displayColorMode);

      Digits[0]->Draw(ss % 10); Digits[1]->Draw(ss / 10);
      Digits[2]->Draw(mm % 10); Digits[3]->Draw(mm / 10);
      Digits[4]->Draw(hh % 10); Digits[5]->Draw(hh / 10);
      Digits[1]->DrawColon(colonColor); Digits[3]->DrawColon(colonColor);
      display.showBuffer();
      
      Digits[0]->Draw(ss % 10); Digits[1]->Draw(ss / 10);
      Digits[2]->Draw(mm % 10); Digits[3]->Draw(mm / 10);
      Digits[4]->Draw(hh % 10); Digits[5]->Draw(hh / 10);
      Digits[1]->DrawColon(colonColor); Digits[3]->DrawColon(colonColor);
      display.showBuffer();
      
      prevss = ss; prevmm = mm; prevhh = hh;
    } else {
      if (ss != prevss) { Digits[0]->SetValue(ss % 10); Digits[1]->SetValue(ss / 10); prevss = ss; }
      if (mm != prevmm) { Digits[2]->SetValue(mm % 10); Digits[3]->SetValue(mm / 10); prevmm = mm; }
      if (hh != prevhh) { Digits[4]->SetValue(hh % 10); Digits[5]->SetValue(hh / 10); prevhh = hh; }
    }
    
    prevEpoch = epoch;
  }
  
  // Dynamic refresh animation throttling
  static unsigned long lastMorphTime = 0;
  if (millis() - lastMorphTime >= (unsigned long)morphFade) {
    lastMorphTime = millis();
    
    for (int i = 0; i < NUM_DIGITS; i++) {
      Digits[i]->Morph();
    }
    
    uint16_t colonColor = getDisplayColor(displayColorMode);
    Digits[1]->DrawColon(colonColor);
    Digits[3]->DrawColon(colonColor);
    
    display.showBuffer();
  }
}