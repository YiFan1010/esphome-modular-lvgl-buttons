#pragma once

#include "esphome.h"
#include "esphome/core/component.h"

#ifdef USE_esp32
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#endif

#include <vector>
#include <string>

namespace esphome {
namespace config_manager {

static const char *TAG = "config_manager";

struct ButtonConfig {
  std::string entity_id;
  std::string label;
  std::string icon;
  std::string type;  // "switch" | "light" | "sensor"
};

class ConfigManagerComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override {
    return setup_priority::AFTER_CONNECTION;
  }

  const std::vector<ButtonConfig> &get_buttons() const { return buttons_; }
  std::string get_icon_hex(const std::string &name) const;

#ifdef USE_esp32
  void handle_root(AsyncWebServerRequest *request);
  void handle_json(AsyncWebServerRequest *request);
  void handle_save(AsyncWebServerRequest *request, uint8_t *data, size_t len,
                   size_t index, size_t total);
#endif

 protected:
  std::string read_config();
  bool write_config(const std::string &json);
  bool parse_config(const std::string &json);

  std::vector<ButtonConfig> buttons_;

#ifdef USE_esp32
  std::unique_ptr<AsyncWebServer> server_;
#endif
};

}  // namespace config_manager
}  // namespace esphome
