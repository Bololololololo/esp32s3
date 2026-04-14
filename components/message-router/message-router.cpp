#include "message-router.h"
#include <esp_log.h>

/* check the queue every 10 ms */
#define tickTICK_PERIOD_MS 10 / portTICK_PERIOD_MS

static const char *TAG = "message-router";

namespace messageRouter {

struct TaskParameters_t {
    QueueHandle_t *xRouterQueue;
    std::unordered_map<ComponentId, std::vector<std::shared_ptr<ComponentInterface>>> *subscribersDB;
};

static void routerTask(void *pvParameters) {
    Message receivedMsg;
    TaskParameters_t *params = (TaskParameters_t *)pvParameters;
    QueueHandle_t *xRouterQueue = params->xRouterQueue;
    std::unordered_map<ComponentId, std::vector<std::shared_ptr<ComponentInterface>>> *subscribersDB =
        params->subscribersDB;

    for (;;) {
        // Wait for a message to arrive in the queue
        if (xQueueReceive(*xRouterQueue, &receivedMsg, tickTICK_PERIOD_MS) == pdPASS) {
            ESP_LOGI(TAG, "Received message %s: id %s", receivedMsg.payload,
                     componentIdToString[receivedMsg.id].c_str());

            if (subscribersDB->find(receivedMsg.id) != subscribersDB->end()) {
                for (const auto &subscriber : (*subscribersDB)[receivedMsg.id]) {
                    if (subscriber) {
                        subscriber->messageHandler(receivedMsg);
                    }
                }
            } else {
                ESP_LOGW(TAG, "No subscribers found for message id: %d", receivedMsg.id);
                ESP_LOGW(TAG, "Subscriber list size : %d", subscribersDB->size());
            }
        }
    }
}

MessageRouter::MessageRouter(_cons) {
    // Create a FreeRTOS queue for message routing
    this->xRouterQueue = xQueueCreate(10, sizeof(Message));
    // Create the router task
    TaskParameters_t taskParams = {.xRouterQueue = &this->xRouterQueue, .subscribersDB = &this->subscribersDB};

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

esp_err_t MessageRouter::subscribe(ComponentId componentId, std::shared_ptr<ComponentInterface> subscriber) {
    ESP_LOGI(TAG, "Subscribed to component: %s", componentIdToString[componentId].c_str());

    if (subscriber != nullptr) {
        subscribersDB[componentId].push_back(subscriber);
    }

    return ESP_OK;
}

esp_err_t MessageRouter::unsubscribe(ComponentId componentId, std::shared_ptr<ComponentInterface> subscriber) {
    ESP_LOGI(TAG, "Unsubscribed from component: %s", componentIdToString[componentId].c_str());

    if (subscriber != nullptr && subscribersDB.find(componentId) != subscribersDB.end()) {
        auto &subscribers = subscribersDB[componentId];
        subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber), subscribers.end());
    }

    return ESP_OK;
}
} // namespace messageRouter