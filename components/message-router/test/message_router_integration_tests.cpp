#include "../../gui/include/gui_service.h"
#include "../../interfaces/include/componentIds.h"
#include "../../interfaces/include/component_interface.h"
#include "../../interfaces/include/service_interface.h"
#include "../../service_manager/include/service_manager.h"
#include "../../wifi/include/wifi_service.h"
#include "../include/message_router.h"
#include "../include/message_router_service.h"
#include "esp_log.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

static const char *TAG = "MsgRouterIntegrationTests";

using namespace messageRouter;
using namespace services;

// Mock service component for testing inter-service communication
class MockServiceComponent : public ComponentInterface {
  public:
    std::atomic<int> messageCount{0};
    Message lastMessage;
    std::mutex messageMutex;

    MockServiceComponent(ComponentId id) : ComponentInterface(id) {
    }

    esp_err_t messageHandler(const Message &msg) override {
        ESP_LOGI(componentIdToString[getComponentId()].c_str(), "Service received message %s: id %s", msg.payload,
                 componentIdToString[msg.id].c_str());
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            lastMessage = msg;
            messageCount++;
        }
        return ESP_OK;
    }
};

// ============================================================================
// SERVICE INITIALIZATION TESTS
// ============================================================================

TEST_CASE("ServiceManager: Initialize All Services", "[integration][service-init]") {
    ESP_LOGI(TAG, "Testing ServiceManager initialization of all services");

    ServiceManager *serviceManager = ServiceManager::getInstance();
    CHECK(serviceManager != nullptr);

    // Verify all services are in stopped state initially
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_MSGR) != ESP_OK);
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_WIFI) != ESP_OK);
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_GUI) != ESP_OK);
}

TEST_CASE("ServiceManager: Start Individual Services", "[integration][service-init]") {
    ESP_LOGI(TAG, "Testing individual service startup");

    ServiceManager *serviceManager = ServiceManager::getInstance();

    // Start message router service first (it's a dependency)
    esp_err_t ret = serviceManager->startService(ServiceType::SERVICE_MSGR);
    CHECK(ret == ESP_OK);
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_MSGR) == ESP_OK);

    // Give service time to initialize
    vTaskDelay(pdMS_TO_TICKS(100));

    // Start WiFi service
    ret = serviceManager->startService(ServiceType::SERVICE_WIFI);
    CHECK(ret == ESP_OK);
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_WIFI) == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "All services started successfully");
}

TEST_CASE("ServiceManager: Stop Services", "[integration][service-init]") {
    ESP_LOGI(TAG, "Testing service shutdown");

    ServiceManager *serviceManager = ServiceManager::getInstance();

    // Start services
    serviceManager->startService(ServiceType::SERVICE_MSGR);
    vTaskDelay(pdMS_TO_TICKS(100));
    serviceManager->startService(ServiceType::SERVICE_WIFI);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Stop services
    esp_err_t ret = serviceManager->stopService(ServiceType::SERVICE_WIFI);
    CHECK(ret == ESP_OK);
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_WIFI) != ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(100));

    ret = serviceManager->stopService(ServiceType::SERVICE_MSGR);
    CHECK(ret == ESP_OK);
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_MSGR) != ESP_OK);

    ESP_LOGI(TAG, "All services stopped successfully");
}

// ============================================================================
// MESSAGE ROUTING WITH SERVICES TESTS
// ============================================================================

TEST_CASE("MessageRouter: Route Messages Between Service Components", "[integration][message-routing]") {
    ESP_LOGI(TAG, "Testing message routing between service components");

    // Initialize message router
    MessageRouter *router = MessageRouter::getInstance();
    CHECK(router != nullptr);

    // Create mock service components
    auto wifiMockComponent = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_WIFI);
    auto guiMockComponent = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_LVGL_DISPLAY);

    // Subscribe both components
    esp_err_t ret = router->subscribe(wifiMockComponent);
    CHECK(ret == ESP_OK);
    ret = router->subscribe(guiMockComponent);
    CHECK(ret == ESP_OK);

    // WiFi component publishes a message
    Message wifiMsg{ComponentId::COMPONENT_ID_WIFI, "WiFi connected to network"};
    ret = router->publish(wifiMsg);
    CHECK(ret == ESP_OK);

    // Give router time to process
    vTaskDelay(pdMS_TO_TICKS(200));

    // GUI should have received the WiFi message
    CHECK(guiMockComponent->messageCount.load() == 1);
    CHECK(std::string(guiMockComponent->lastMessage.payload) == "WiFi connected to network");

    // GUI component publishes a message
    Message guiMsg{ComponentId::COMPONENT_ID_LVGL_DISPLAY, "Display updated"};
    ret = router->publish(guiMsg);
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(200));

    // WiFi should have received the GUI message
    CHECK(wifiMockComponent->messageCount.load() == 1);
    CHECK(std::string(wifiMockComponent->lastMessage.payload) == "Display updated");

    // Clean up
    router->unsubscribe(wifiMockComponent);
    router->unsubscribe(guiMockComponent);
}

TEST_CASE("MessageRouter: Broadcast to Multiple Service Subscribers", "[integration][message-routing]") {
    ESP_LOGI(TAG, "Testing broadcast message routing to multiple subscribers");

    MessageRouter *router = MessageRouter::getInstance();

    // Create multiple mock service components
    std::vector<std::shared_ptr<MockServiceComponent>> subscribers;
    for (int i = 0; i < 4; i++) {
        auto component = std::make_shared<MockServiceComponent>(
            static_cast<ComponentId>(static_cast<int>(ComponentId::COMPONENT_ID_WIFI) + i));
        subscribers.push_back(component);
        router->subscribe(component);
    }

    // Publish a broadcast message
    Message broadcastMsg{ComponentId::COMPONENT_ID_TEST_PUB, "System wide broadcast"};
    esp_err_t ret = router->publish(broadcastMsg);
    CHECK(ret == ESP_OK);

    // Give router time to process and deliver to all
    vTaskDelay(pdMS_TO_TICKS(300));

    // All subscribers should have received the message
    for (size_t i = 0; i < subscribers.size(); i++) {
        CHECK(subscribers[i]->messageCount.load() == 1);
        CHECK(std::string(subscribers[i]->lastMessage.payload) == "System wide broadcast");
    }

    // Clean up
    for (const auto &sub : subscribers) {
        router->unsubscribe(sub);
    }
}

// ============================================================================
// SERVICE COMMUNICATION TESTS
// ============================================================================

TEST_CASE("ServiceIntegration: Services Communicate Through MessageRouter", "[integration][service-communication]") {
    ESP_LOGI(TAG, "Testing full service integration with message routing");

    ServiceManager *serviceManager = ServiceManager::getInstance();
    MessageRouter *router = MessageRouter::getInstance();

    // Start all services
    ESP_LOGI(TAG, "Starting all services");
    esp_err_t ret = serviceManager->startService(ServiceType::SERVICE_MSGR);
    CHECK(ret == ESP_OK);
    vTaskDelay(pdMS_TO_TICKS(150));

    ret = serviceManager->startService(ServiceType::SERVICE_WIFI);
    CHECK(ret == ESP_OK);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Verify services are running
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_MSGR) == ESP_OK);
    CHECK(serviceManager->isServiceRunning(ServiceType::SERVICE_WIFI) == ESP_OK);

    // Create a mock component and subscribe to router
    auto testComponent = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    router->subscribe(testComponent);

    // Publish a message that services would use
    Message statusMsg{ComponentId::COMPONENT_ID_WIFI, "Network status update"};
    ret = router->publish(statusMsg);
    CHECK(ret == ESP_OK);

    vTaskDelay(pdMS_TO_TICKS(300));

    // Test component should have received the message
    CHECK(testComponent->messageCount.load() >= 1);

    // Clean up
    router->unsubscribe(testComponent);
    serviceManager->stopService(ServiceType::SERVICE_WIFI);
    vTaskDelay(pdMS_TO_TICKS(100));
    serviceManager->stopService(ServiceType::SERVICE_MSGR);
}

TEST_CASE("ServiceIntegration: High Volume Message Routing Between Services", "[integration][service-communication]") {
    ESP_LOGI(TAG, "Testing high volume message routing between services");

    MessageRouter *router = MessageRouter::getInstance();

    // Create producer and consumer components
    auto producer = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_TEST_PUB);
    auto consumer1 = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    auto consumer2 = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_WIFI);

    router->subscribe(consumer1);
    router->subscribe(consumer2);

    const int NUM_MESSAGES = 25;
    ESP_LOGI(TAG, "Publishing %d messages through router", NUM_MESSAGES);

    // Publish multiple messages
    for (int i = 0; i < NUM_MESSAGES; i++) {
        std::string payload = "ServiceMsg_" + std::to_string(i);
        Message msg{ComponentId::COMPONENT_ID_TEST_PUB, payload.c_str()};
        esp_err_t ret = router->publish(msg);
        CHECK(ret == ESP_OK);
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay between messages
    }

    // Give router time to process all messages
    vTaskDelay(pdMS_TO_TICKS(500));

    // Both consumers should have received all messages
    ESP_LOGI(TAG, "Consumer1 received %d messages, Consumer2 received %d messages", consumer1->messageCount.load(),
             consumer2->messageCount.load());
    CHECK(consumer1->messageCount.load() == NUM_MESSAGES);
    CHECK(consumer2->messageCount.load() == NUM_MESSAGES);

    // Clean up
    router->unsubscribe(consumer1);
    router->unsubscribe(consumer2);
}

TEST_CASE("ServiceIntegration: Dynamic Subscribe/Unsubscribe During Message Flow",
          "[integration][service-communication]") {
    ESP_LOGI(TAG, "Testing dynamic service subscription changes");

    MessageRouter *router = MessageRouter::getInstance();

    auto component1 = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    auto component2 = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_WIFI);
    auto publisher = std::make_shared<MockServiceComponent>(ComponentId::COMPONENT_ID_TEST_PUB);

    // Phase 1: Only component1 subscribed
    ESP_LOGI(TAG, "Phase 1: component1 subscribed");
    router->subscribe(component1);

    Message msg1{ComponentId::COMPONENT_ID_TEST_PUB, "Message 1"};
    router->publish(msg1);
    vTaskDelay(pdMS_TO_TICKS(200));

    CHECK(component1->messageCount.load() == 1);
    CHECK(component2->messageCount.load() == 0);

    // Phase 2: Add component2
    ESP_LOGI(TAG, "Phase 2: component2 subscribed");
    router->subscribe(component2);

    Message msg2{ComponentId::COMPONENT_ID_TEST_PUB, "Message 2"};
    router->publish(msg2);
    vTaskDelay(pdMS_TO_TICKS(200));

    CHECK(component1->messageCount.load() == 2);
    CHECK(component2->messageCount.load() == 1);

    // Phase 3: Remove component1
    ESP_LOGI(TAG, "Phase 3: component1 unsubscribed");
    router->unsubscribe(component1);

    Message msg3{ComponentId::COMPONENT_ID_TEST_PUB, "Message 3"};
    router->publish(msg3);
    vTaskDelay(pdMS_TO_TICKS(200));

    CHECK(component1->messageCount.load() == 2); // Should still be 2
    CHECK(component2->messageCount.load() == 2); // Should now be 2

    // Clean up
    router->unsubscribe(component2);
}

TEST_CASE("ServiceIntegration: Service Error Recovery", "[integration][service-communication]") {
    ESP_LOGI(TAG, "Testing service error handling and recovery");

    ServiceManager *serviceManager = ServiceManager::getInstance();

    // Try to stop a service that's not running
    esp_err_t ret = serviceManager->stopService(ServiceType::SERVICE_GUI);
    CHECK(ret == ESP_OK); // Service may not be started
    esp_err_t ret = serviceManager->stopService(ServiceType::SERVICE_GUI);
    CHECK(ret == ESP_ERR_NOT_FOUND); // Service may not be started

    // Try to check if non-existent service is running
    ret = serviceManager->isServiceRunning(ServiceType::SERVICE_MAX);
    CHECK(ret != ESP_OK); // Should fail

    // Start a service
    ret = serviceManager->startService(ServiceType::SERVICE_MSGR);
    CHECK(ret == ESP_OK);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Try to start it again (should still return OK)
    ret = serviceManager->startService(ServiceType::SERVICE_MSGR);
    CHECK(ret == ESP_OK);

    // Clean up
    serviceManager->stopService(ServiceType::SERVICE_MSGR);
}
