#ifndef HTTP_UPDATE_UTILS_H
#define HTTP_UPDATE_UTILS_H
class httpUpdateUtils {
  public:
    httpUpdateUtils(const char* updurl, const char* fp, const char* current_firmware_version, const char* current_spiffs_version, bool spiffs_update = true);
    String httpUpdate();

  private:
    void spiffsUpdate(bool sketchWasUpdated);
    String sketchUpdate();
    const char* update_url;
    const char* fingerprint;
    const char* current_firmware_version;
    const char* current_spiffs_version;
    bool spiffs_update;
};

#endif
