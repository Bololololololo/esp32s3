#ifndef WIFI_COMP_H
#define WIFI_COMP_H

#include "componentInterface.h"

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

} // namespace wifis

#endif // WIFI_COMP_H