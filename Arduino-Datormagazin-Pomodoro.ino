#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"
#include <FS.h>
#include <Bounce2.h>

FASTLED_USING_NAMESPACE
#define SPEEDUP 60
#define DATA_PIN    5
#define BUTTON_PIN 2
#define BUZZER_PIN 4
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
boolean running = false;

ESP8266WebServer server(80);
WiFiManager wifiManager;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(20);

  Serial.begin(115200);
  Serial.println("Booting");

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  for (byte led = 0; led < NUM_LEDS; led++) {
    leds[led] = CRGB::DarkBlue;
  }
  FastLED.show();

  wifiManager.setConfigPortalTimeout(5 * 60); // Start without wifi if wifi isn't configured within 5 minutes

  wifiManager.autoConnect(esp_ssid, esp_password);
  Serial.println("wifiManager started");

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  MDNS.begin("pomodoro"); // Makes the device accessible through http://pomodoro.local
  MDNS.addService("http", "tcp", 80);
  SPIFFS.begin();

  server.on ("/gettime", web_get_time);
  server.serveStatic("/", SPIFFS, "/h/", "max-age=3600");
  server.begin();
}

void loop() {
  server.handleClient();
  debouncer.update();
  boolean button_up = debouncer.read();
  if (!button_up) {
    start_pomodoro();
  }

  if (!running) {
    fill_rainbow(leds, NUM_LEDS, gHue, 7);
    FastLED.delay(1000 / FRAMES_PER_SECOND);
  } else {
    byte led;
    for (led = 0; led < (elapsed_time_millis() * NUM_LEDS / POMODORO_LENGTH); led++) {
      leds[led] = 0x32FF32;
    }
    byte level = abs(128 - gHue) * 1.33;
    leds[led].setRGB(40 + level, 255 - level, 40);
    for (led++ ; led < NUM_LEDS; led++) {
      leds[led] = CRGB::DarkRed;
    }
  }
  FastLED.show();

  EVERY_N_MILLISECONDS( 5 ) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  if (running) {
    if (elapsed_time_millis() > POMODORO_LENGTH) {
      stop_pomodoro();
    }
  }
}

unsigned long elapsed_time_millis() {
  return (millis() - start_time);
}

void web_get_time() {
  String json;
  if (running) {
    json = "{\"procent\":\"" + String(elapsed_time_millis() * 100.0 / POMODORO_LENGTH) + "\"}";
  } else {
    json = "{\"procent\":\"0\"}";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "application/json", json);
}

void start_pomodoro() {
  start_time = millis();
  running = true;
}

void stop_pomodoro() {
  running = false;
  tone(BUZZER_PIN, 1000, 100);
  FastLED.delay(100);
  noTone(BUZZER_PIN);
}

