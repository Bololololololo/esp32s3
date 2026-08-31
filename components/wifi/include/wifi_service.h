#ifndef _WIFI_SERVICE_H_
#define _WIFI_SERVICE_H_

#include "serviceInterface.h"
#include "wifi_comp.h"
#include <memory>

using namespace wifi;

namespace services {
class WifiService : public ServiceInterface {
  public:
    WifiService() : ServiceInterface(ServiceType::SERVICE_WIFI) {
    }
    ~WifiService() override = default;

    void start() override;
    void stop() override;

  private:
    std::unique_ptr<WifiComponent> wifiComponent;
};

} // namespace services

#endif // _WIFI_SERVICE_H_
