#include "../include/message_router.h"
#include "dummy_components.h"
#include "esp_log.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

static const char *TAG = "MessageRouterSubscriptionTests";

using namespace messageRouter;

// ============================================================================
// SUBSCRIPTION FILTERING TESTS
// ============================================================================

TEST_CASE("MessageRouter: Subscribe to Specific Component Messages", "[unit][message-router-filtering]") {
    ESP_LOGI(TAG, "Testing subscription to specific component messages");

    auto wifiSubscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_LVGL_DISPLAY);
    auto storageSubscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_STORAGE);

    // Subscribe to only WiFi messages
    MessageRouter::getInstance()->subscribe(wifiSubscriber, ComponentId::COMPONENT_ID_WIFI);
    // Subscribe to only Storage messages
    MessageRouter::getInstance()->subscribe(storageSubscriber, ComponentId::COMPONENT_ID_STORAGE);

    // Publish WiFi message
    esp_err_t ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_WIFI, "WiFi connected"});
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    // Only wifiSubscriber should have received it
    CHECK(wifiSubscriber->lastMessage.id == ComponentId::COMPONENT_ID_WIFI);
    CHECK(std::string(wifiSubscriber->lastMessage.payload) == "WiFi connected");
    CHECK(storageSubscriber->messageCount == 0); // Should have received nothing

    // Publish Storage message
    ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_STORAGE, "Data saved"});
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    // Only storageSubscriber should have received it
    CHECK(storageSubscriber->lastMessage.id == ComponentId::COMPONENT_ID_STORAGE);
    CHECK(std::string(storageSubscriber->lastMessage.payload) == "Data saved");
    // wifiSubscriber should still only have the WiFi message
    CHECK(std::string(wifiSubscriber->lastMessage.payload) == "WiFi connected");

    // Clean up
    MessageRouter::getInstance()->unsubscribe(wifiSubscriber, ComponentId::COMPONENT_ID_WIFI);
    MessageRouter::getInstance()->unsubscribe(storageSubscriber, ComponentId::COMPONENT_ID_STORAGE);
}

TEST_CASE("MessageRouter: Wildcard Subscription Receives All Messages", "[unit][message-router-filtering]") {
    ESP_LOGI(TAG, "Testing wildcard subscription to all messages");

    auto wildcardSubscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_LVGL_DISPLAY);
    auto specificSubscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_STORAGE);

    // Subscribe to ALL messages (default/wildcard)
    MessageRouter::getInstance()->subscribe(wildcardSubscriber); // No sourceComponentId = COMPONENT_ID_ALL
    // Subscribe to only WiFi messages
    MessageRouter::getInstance()->subscribe(specificSubscriber, ComponentId::COMPONENT_ID_WIFI);

    // Publish WiFi message
    esp_err_t ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_WIFI, "WiFi msg"});
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    // Both should receive WiFi message
    CHECK(wildcardSubscriber->lastMessage.id == ComponentId::COMPONENT_ID_WIFI);
    CHECK(specificSubscriber->lastMessage.id == ComponentId::COMPONENT_ID_WIFI);

    // Publish Storage message
    ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_STORAGE, "Storage msg"});
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    // Only wildcardSubscriber should receive Storage message
    CHECK(wildcardSubscriber->lastMessage.id == ComponentId::COMPONENT_ID_STORAGE);
    CHECK(std::string(wildcardSubscriber->lastMessage.payload) == "Storage msg");
    // specificSubscriber should still have the WiFi message
    CHECK(std::string(specificSubscriber->lastMessage.payload) == "WiFi msg");

    // Clean up
    MessageRouter::getInstance()->unsubscribe(wildcardSubscriber);
    MessageRouter::getInstance()->unsubscribe(specificSubscriber, ComponentId::COMPONENT_ID_WIFI);
}

TEST_CASE("MessageRouter: Mixed Specific and Wildcard Subscriptions", "[unit][message-router-filtering]") {
    ESP_LOGI(TAG, "Testing mixed specific and wildcard subscriptions");

    auto wifiSpecific = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_LVGL_DISPLAY);
    auto allMessages = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_STORAGE);

    // Subscribe to WiFi messages specifically
    MessageRouter::getInstance()->subscribe(wifiSpecific, ComponentId::COMPONENT_ID_WIFI);
    // Subscribe to all messages
    MessageRouter::getInstance()->subscribe(allMessages, ComponentId::COMPONENT_ID_ALL);

    // Publish BLE message
    esp_err_t ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_BLE, "BLE data"});
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    // Only allMessages subscriber should receive it
    CHECK(allMessages->lastMessage.id == ComponentId::COMPONENT_ID_BLE);
    CHECK(std::string(allMessages->lastMessage.payload) == "BLE data");
    CHECK(wifiSpecific->messageCount == 0);

    // Publish WiFi message
    ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_WIFI, "WiFi event"});
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    // Both should receive WiFi message
    CHECK(wifiSpecific->lastMessage.id == ComponentId::COMPONENT_ID_WIFI);
    CHECK(std::string(wifiSpecific->lastMessage.payload) == "WiFi event");
    CHECK(allMessages->messageCount == 2); // BLE + WiFi

    // Clean up
    MessageRouter::getInstance()->unsubscribe(wifiSpecific, ComponentId::COMPONENT_ID_WIFI);
    MessageRouter::getInstance()->unsubscribe(allMessages, ComponentId::COMPONENT_ID_ALL);
}

TEST_CASE("MessageRouter: Unsubscribe from Specific Source", "[unit][message-router-filtering]") {
    ESP_LOGI(TAG, "Testing unsubscribe from specific source");

    auto wifiSubscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_LVGL_DISPLAY);
    auto allSubscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_STORAGE);

    // wifiSubscriber subscribes to WiFi messages
    MessageRouter::getInstance()->subscribe(wifiSubscriber, ComponentId::COMPONENT_ID_WIFI);
    // allSubscriber subscribes to all messages
    MessageRouter::getInstance()->subscribe(allSubscriber, ComponentId::COMPONENT_ID_ALL);

    // Publish WiFi message
    esp_err_t ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_WIFI, "WiFi 1"});
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    CHECK(wifiSubscriber->messageCount == 1);
    CHECK(allSubscriber->messageCount == 1);

    // Unsubscribe wifiSubscriber from WiFi messages
    MessageRouter::getInstance()->unsubscribe(wifiSubscriber, ComponentId::COMPONENT_ID_WIFI);

    // Publish another WiFi message
    ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_WIFI, "WiFi 2"});
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    // wifiSubscriber should still have only 1 message
    CHECK(wifiSubscriber->messageCount == 1);
    // allSubscriber should have 2 messages
    CHECK(allSubscriber->messageCount == 2);

    // Clean up
    MessageRouter::getInstance()->unsubscribe(allSubscriber, ComponentId::COMPONENT_ID_ALL);
}
