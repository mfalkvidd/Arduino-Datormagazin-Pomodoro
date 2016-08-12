#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"
#include <FS.h>
#include <Bounce2.h>
#include <time.h>

FASTLED_USING_NAMESPACE
#define SPEEDUP 1
#define DATA_PIN 5 // D1
#define BUTTON_PIN 16 // D0
#define BUZZER_PIN 4 //D2
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    24
CRGB leds[NUM_LEDS];

#define BRIGHTNESS 20
#define FRAMES_PER_SECOND 120
#define POMODORO_LENGTH (25*60*1000/SPEEDUP)
uint8_t gHue = 0;

Bounce debouncer = Bounce();

const char* esp_ssid = "Pomodoro";
const char* esp_password = "pomodoro";

unsigned long start_time;
bool running = false;
bool first = true; // Workaround for the weird behavior the touch button experience at first start.

ESP8266WebServer server(80);
WiFiManager wifiManager;

void setup() {
  Serial.begin(115200);
  printSystemInfo();

  pinMode(BUTTON_PIN, INPUT);
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(20);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  paintAllLeds(CRGB::DarkBlue);

  wifiManager.setConfigPortalTimeout(5 * 60); // Start without wifi if wifi isn't configured within 5 minutes

  wifiManager.autoConnect(esp_ssid, esp_password);
  paintLeds(1, CRGB::DarkGreen, CRGB::DarkBlue, CRGB::DarkBlue);
  Serial.println("wifiManager started");
  setClockFromNTP();
  printSystemInfo();
  paintLeds(2, CRGB::DarkGreen, CRGB::DarkBlue, CRGB::DarkBlue);

  MDNS.begin("pomodoro"); // Makes the device accessible through http://pomodoro.local
  MDNS.addService("http", "tcp", 80);
  paintLeds(3, CRGB::DarkGreen, CRGB::DarkBlue, CRGB::DarkBlue);
  SPIFFS.begin();
  paintLeds(4, CRGB::DarkGreen, CRGB::DarkBlue, CRGB::DarkBlue);

  server.on ("/gettime", web_get_time);
  server.serveStatic("/", SPIFFS, "/h/", "max-age=3600");
  server.begin();
  paintLeds(5, CRGB::DarkGreen, CRGB::DarkBlue, CRGB::DarkBlue);
}

void loop() {
  server.handleClient();
  if (debouncer.update()) { // Only read if the state changed from last time
    if (debouncer.rose() && !first) {
      Serial.println("Rose");
      start_pomodoro();
    } else {
      first = false;
      Serial.println("Did not rise");
    }
  }

  if (!running) {
    fill_rainbow(leds, NUM_LEDS, gHue, 7);
    FastLED.delay(1000 / FRAMES_PER_SECOND);
    FastLED.show();
  } else {
    byte level = abs(128 - gHue) * 1.33;
    paintLeds((elapsed_time_millis() * NUM_LEDS / POMODORO_LENGTH), 0x32FF32, CRGB(40 + level, 255 - level, 40), CRGB::DarkRed);
  }

  EVERY_N_MILLISECONDS( 5 ) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  if (running) {
    if (elapsed_time_millis() > POMODORO_LENGTH) {
      stop_pomodoro();
    }
  }
}

void paintAllLeds(CRGB color) {
  paintLeds(0, color, color, color);
}

void paintLeds(byte divider, CRGB firstColor, CRGB secondColor, CRGB thirdColor) {
  byte led;
  for (led = 0; led < divider; led++) {
    leds[led] = firstColor;
  }
  leds[led] = secondColor;
  for (led++ ; led < NUM_LEDS; led++) {
    leds[led] = thirdColor;
  }
  FastLED.show();
}

unsigned long elapsed_time_millis() {
  return (millis() - start_time);
}

void web_get_time() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  if (running) {
    server.send(200, "application/json", "{\"procent\":\"" + String(elapsed_time_millis() * 100.0 / POMODORO_LENGTH) + "\"}");
  } else {
    server.send(200, "application/json", "{\"procent\":\"0\"}");
  }
}

void start_pomodoro() {
  tone(BUZZER_PIN, 30, 50);
  start_time = millis();
  running = true;
  Serial.print(F("Pomodoro started at "));
  time_t now = time(nullptr);
  Serial.print(ctime(&now));
  FastLED.delay(50);
  noTone(BUZZER_PIN);
}

void stop_pomodoro() {
  running = false;
  tone(BUZZER_PIN, 1000, 100);
  Serial.print(F("Pomodoro stopped at "));
  time_t now = time(nullptr);
  Serial.print(ctime(&now));
  FastLED.delay(100);
  noTone(BUZZER_PIN);
}

void printSystemInfo() {
  Serial.println(F("----- System details -----------------------------------------"));
  Serial.print(F("Vcc                   : "));
  Serial.println(ESP.getVcc());
  Serial.print(F("FreeHeap              : "));
  Serial.println(ESP.getFreeHeap());
  Serial.print(F("ChipId                : "));
  Serial.println(ESP.getChipId());
  Serial.print(F("BootVersion           : "));
  Serial.println(ESP.getBootVersion());
  Serial.print(F("BootMode              : "));
  Serial.println(ESP.getBootMode());
  Serial.print(F("CpuFreqMHz            : "));
  Serial.println(ESP.getCpuFreqMHz());
  Serial.print(F("FlashChipId           : "));
  Serial.println(ESP.getFlashChipId());
  Serial.print(F("FlashChipSize         : "));
  Serial.println(ESP.getFlashChipSize());
  Serial.print(F("FlashChipRealSize     : "));
  Serial.println(ESP.getFlashChipRealSize());
  Serial.print(F("FlashChipSizeByChipId : "));
  Serial.println(ESP.getFlashChipSizeByChipId());
  Serial.print(F("SdkVersion            : "));
  Serial.println(ESP.getSdkVersion());
  Serial.print(F("FlashChipSpeed        : "));
  Serial.println(ESP.getFlashChipSpeed());
  Serial.print(F("FlashChipMode         : "));
  Serial.println(ESP.getFlashChipMode());
  Serial.print(F("SketchSize            : "));
  Serial.println(ESP.getSketchSize());
  Serial.print(F("FreeSketchSpace       : "));
  Serial.println(ESP.getFreeSketchSpace());
  Serial.print(F("ResetReason           : "));
  Serial.println(ESP.getResetReason());
  Serial.print(F("ResetInfo             : "));
  Serial.println(ESP.getResetInfo());
  Serial.println(F("----- IP details ---------------------------------------------"));
  Serial.print(F("IP Address            : "));
  Serial.println(WiFi.localIP());
  Serial.print(F("Subnet                : "));
  Serial.println(WiFi.subnetMask());
  Serial.print(F("Gateway               : "));
  Serial.println(WiFi.gatewayIP());
#ifdef _TIME_H_
  Serial.println(F("----- Time details -------------------------------------------"));
  Serial.print(F("Time                  : "));
  time_t now = time(nullptr);
  Serial.print(ctime(&now));
#endif
  Serial.println(F("+++++ System ready +++++++++++++++++++++++++++++++++++++++++++"));
}

void setClockFromNTP() {
  configTime(2 * 3600, 0, "ntp.se"); // will direct the client to the closest available server (in Sweden)
  while (!time(nullptr)) {
    delay(10);
  }
}
