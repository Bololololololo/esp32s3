#include "wifi_comp.h"
#include "message-router.h"

using namespace messageRouter;

namespace wifi {
WifiComponent::WifiComponent() : ComponentInterface(ComponentId::COMPONENT_ID_WIFI) {
    // Initialization code for the WiFi component can go here
    messageRouter::MessageRouter::getInstance()->subscribe(ComponentId::COMPONENT_ID_WIFI, std::make_shared<WifiComponent>(*this));
}

esp_err_t WifiComponent::messageHandler(const Message &msg) {
    // Handle incoming messages related to WiFi component
    if (msg.id == ComponentId::COMPONENT_ID_WIFI) {
        // Process the message payload
        std::printf("Received message for WiFi component: %s\n", msg.payload);
        // Add code to handle specific commands or data
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG; // Return error if message ID does not match
}
}