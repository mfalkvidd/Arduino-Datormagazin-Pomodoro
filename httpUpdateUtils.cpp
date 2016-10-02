#include <ESP8266httpUpdate.h>
#include "httpUpdateUtils.h"

httpUpdateUtils::httpUpdateUtils(char const* update_url, char const* fingerprint, char const* current_firmware_version, char const* current_spiffs_version, bool spiffs_update): update_url(update_url), fingerprint(fingerprint), current_firmware_version(current_firmware_version), current_spiffs_version(current_spiffs_version), spiffs_update(spiffs_update) {
};

void httpUpdateUtils::spiffsUpdate(bool sketchWasUpdated) {
  Serial.println("Update spiffs.");
  t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(update_url, current_spiffs_version, fingerprint);
  switch (ret) {
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("No spiffs update"));
      if (sketchWasUpdated) {
        delay(5000);
        ESP.reset();
      }
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("Spiffs update OK"));
      delay(5000);
      ESP.reset();
      break;

    case HTTP_UPDATE_FAILED:
      Serial.printf("Spiffs update failed. Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      if (sketchWasUpdated) {
        delay(5000);
        ESP.reset();
      }
      break;
  }
}

String httpUpdateUtils::sketchUpdate() {
  Serial.println("Update sketch.");
  t_httpUpdate_return ret = ESPhttpUpdate.update(update_url, current_firmware_version, fingerprint);
  switch (ret) {
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("No sketch update"));
      if (spiffs_update) {
        spiffsUpdate(false);
      }
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("Sketch update OK"));
      if (spiffs_update) {
        spiffsUpdate(true);
      }
      break;

    case HTTP_UPDATE_FAILED:
      Serial.printf("Sketch update failed. Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      return ESPhttpUpdate.getLastErrorString().c_str();
  }
  return "";
}

String httpUpdateUtils::httpUpdate() {
  Serial.print("url: ");
  Serial.println(update_url);
  Serial.print("fingerprint: ");
  Serial.println(fingerprint);
  Serial.print("current firmware version: ");
  Serial.println(current_firmware_version);
  Serial.print("current spiffs version: ");
  Serial.println(current_spiffs_version);
  ESPhttpUpdate.rebootOnUpdate(false);
  return sketchUpdate();
}

