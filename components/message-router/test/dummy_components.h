#include "../include/message_router.h"
#include "componentIds.h"
#include "component_interface.h"
#include "esp_log.h"

class DummyComponent : public ComponentInterface {
  public:
    Message lastMessage;

    DummyComponent(ComponentId id) : ComponentInterface(id) {
    }

    esp_err_t messageHandler(const Message &message) override {
        ESP_LOGI(componentIdToString[getComponentId()].c_str(), "Received message %s: id %s", message.payload,
                 componentIdToString[message.id].c_str());
        lastMessage = message; // Store the last received message for testing purposes
        return ESP_OK;
    }
};
