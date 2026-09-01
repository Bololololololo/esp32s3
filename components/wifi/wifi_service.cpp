#include "wifi_service.h"
#include "esp_log.h"
#include <cstdio>

static const char *TAG = "wifi-service";

namespace services {

void WifiService::start() {
    // Start the WiFi service
    ESP_LOGI(TAG, "WiFi service starting");
    this->setServiceState(SERVICE_RUNNING);
    this->wifiComponent = std::make_unique<WifiComponent>();
    ESP_LOGI(TAG, "WiFi service started successfully");
    // Add code to initialize WiFi hardware and connect to network
}

void WifiService::stop() {
    // Stop the WiFi service
    this->setServiceState(SERVICE_STOPPED);
    // Add code to disconnect from network and cleanup WiFi resources
}

} // namespace services
