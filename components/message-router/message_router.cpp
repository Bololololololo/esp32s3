#include "message_router.h"
#include <esp_log.h>

static const char *TAG = "message-router";

namespace messageRouter {

struct TaskParameters_t {
    QueueHandle_t *xRouterQueue;
    std::unordered_map<ComponentId, std::vector<std::shared_ptr<ComponentInterface>>> *subscribersDB;
};

static void routerTask(void *pvParameters) {
    Message receivedMsg;
    TaskParameters_t *params = (TaskParameters_t *)pvParameters;
    QueueHandle_t &xRouterQueue = *params->xRouterQueue;
    auto &subscribersDB = *params->subscribersDB;

    for (;;) {
        // Block indefinitely until a message arrives (no unnecessary wakeups)
        if (xQueueReceive(xRouterQueue, &receivedMsg, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Received message %s: id %s", receivedMsg.payload,
                     componentIdToString[receivedMsg.id].c_str());

            // Single lookup: use find() iterator instead of find() + operator[]
            auto it = subscribersDB.find(receivedMsg.id);
            if (it != subscribersDB.end()) {
                for (const auto &subscriber : it->second) {
                    subscriber->messageHandler(receivedMsg);
                }
            } else {
                ESP_LOGW(TAG, "No subscribers found for message id: %d", receivedMsg.id);
            }
        }
    }
}

MessageRouter::MessageRouter(_cons) {
    // Create a FreeRTOS queue for message routing
    this->xRouterQueue = xQueueCreate(10, sizeof(Message));
    // Create the router task
    TaskParameters_t taskParams = {.xRouterQueue = &this->xRouterQueue, .subscribersDB = &this->subscribersDB};

    ESP_LOGI(TAG, "Message router task created");
    xTaskCreate(routerTask, "MessageRouterTask", 8192, &taskParams, 5, &this->xRouterTask);
}

esp_err_t MessageRouter::publish(const Message &message) {
    ESP_LOGI(TAG, "Publishing message %s: id %s", message.payload, componentIdToString[message.id].c_str());
    if (xQueueSend(xRouterQueue, &message, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send message to router queue");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t MessageRouter::subscribe(std::shared_ptr<ComponentInterface> subscriber) {
    ESP_LOGI(TAG, "Subscribed to component: %s", componentIdToString[subscriber->getComponentId()].c_str());

    if (subscriber != nullptr) {
        subscribersDB[subscriber->getComponentId()].push_back(subscriber);
    }

    return ESP_OK;
}

esp_err_t MessageRouter::unsubscribe(std::shared_ptr<ComponentInterface> subscriber) {
    ESP_LOGI(TAG, "Unsubscribed from component: %s", componentIdToString[subscriber->getComponentId()].c_str());

    if (subscriber != nullptr && subscribersDB.find(subscriber->getComponentId()) != subscribersDB.end()) {
        auto &subscribers = subscribersDB[subscriber->getComponentId()];
        subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber), subscribers.end());
    }

    return ESP_OK;
}
} // namespace messageRouter