#ifndef WIFI_COMP_H
#define WIFI_COMP_H

#include "component_interface.h"

namespace wifi {

enum class WifiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
};

class WifiComponent : public ComponentInterface {
  public:
    WifiComponent();

  private:
    esp_err_t messageHandler(const Message &msg) override;
    void connectToNetwork();
    void disconnectFromNetwork();
};

} // namespace wifi

#endif // WIFI_COMP_H