#include "service_manager.h"
#include "esp_log.h"
#include "wifi_service.h"
#include <stdio.h>

static const char *TAG = "message-router";

namespace services {
ServiceManager::ServiceManager(_cons) {
    // Constructor implementation
    this->servicesMap.emplace(ServiceType::SERVICE_WIFI, std::make_unique<WifiService>());
}

esp_err_t ServiceManager::startService(const ServiceType &serviceType) {
    auto it = this->servicesMap.find(serviceType);
    if (it != this->servicesMap.end()) {
        it->second->start();
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Service %d not found", static_cast<int>(serviceType));
    return ESP_ERR_NOT_FOUND;
}
} // namespace services