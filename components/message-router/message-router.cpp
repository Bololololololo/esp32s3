#include "message-router.h"
#include <esp_log.h>

/* check the queue every 10 ms */
#define tickTICK_PERIOD_MS 10 / portTICK_PERIOD_MS

namespace messageRouter {

static void routerTask(void *pvParameters) {
    Message receivedMsg;
    QueueHandle_t *xRouterQueue = (QueueHandle_t *)pvParameters;

    for (;;) {
        // Wait for a message to arrive in the queue
        if (xQueueReceive(*xRouterQueue, &receivedMsg, tickTICK_PERIOD_MS) == pdPASS) {

            switch (receivedMsg.id) {
            case COMPONENT_ID_LVGL_DISPLAY:
                ESP_LOGI("Router", "Received LVGL Display message: %s", receivedMsg.payload);
                break;
            case COMPONENT_ID_BLE:
                ESP_LOGI("Router", "Received BLE message: %s", receivedMsg.payload);
                break;
            default:
                ESP_LOGW("Router", "Unknown message id received: %d", receivedMsg.id);
                break;
            }
        }
    }
}

esp_err_t MessageRouter::initialize() {
    // Create a FreeRTOS queue for message routing
    this->xRouterQueue = xQueueCreate(10, sizeof(Message));

    // Create the router task
    xTaskCreate(routerTask, "MessageRouterTask", 2048, &this->xRouterQueue, 5, &this->xRouterTask);
    return ESP_OK;
}

esp_err_t MessageRouter::publish(char *topic, const Message &message) {
    ESP_LOGI("Router", "Publishing message to topic %s: id %s", topic, componentIdToString[message.id].c_str());

    if (xQueueSend(xRouterQueue, &message, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("Router", "Failed to send message to router queue");
        return ESP_FAIL;
    }
    return ESP_OK;
}
} // namespace messageRouter