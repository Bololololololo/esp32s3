#include "gui_service.h"
#include "esp_log.h"
#include <cstdio>

static const char *TAG = "gui-service";

namespace services {

void GuiService::start() {
    // Start the GUI service
    ESP_LOGI(TAG, "GUI service starting");
    this->setServiceState(SERVICE_RUNNING);
    this->guiComponent = GuiComponent::instanceFactory();
    // Initialize the GUI component
    this->guiComponent->init();
    ESP_LOGI(TAG, "GUI service started successfully");
}

void GuiService::stop() {
    // Stop the GUI service
    this->setServiceState(SERVICE_STOPPED);
    // Clean up the GUI component
    this->guiComponent.reset();
}

} // namespace services
