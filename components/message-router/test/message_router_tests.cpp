#include "../include/message-router.h"
#include "dummy_components.h"
#include "esp_log.h"
#include <catch2/catch_test_macros.hpp>

static const char *TAG = "MessageRouterTests";

using namespace messageRouter;

TEST_CASE("MessageRouter: Basic Operations", "[message-router]") {
    ESP_LOGI(TAG, "Running MessageRouter: Basic Operations");
    MessageRouter *router = MessageRouter::getInstance();
    esp_err_t ret = ESP_OK;

    auto dummySubscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    auto dummyPublisher = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_PUB);

    MessageRouter::getInstance()->subscribe(ComponentId::COMPONENT_ID_TEST_PUB, dummySubscriber);
    MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, "Banana"});

    sleep(1); // Wait for the message to be processed

    CHECK(dummySubscriber->lastMessage.id == ComponentId::COMPONENT_ID_TEST_PUB);
    CHECK(std::string(dummySubscriber->lastMessage.payload) == "Banana");

    MessageRouter::getInstance()->unsubscribe(ComponentId::COMPONENT_ID_TEST_PUB, dummySubscriber);
    MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, "Apple"});
    CHECK(std::string(dummySubscriber->lastMessage.payload) ==
          "Banana"); // Should still be "Banana" since we unsubscribed
}
