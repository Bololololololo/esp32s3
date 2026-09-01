#include "../include/message_router.h"
#include "dummy_components.h"
#include "esp_log.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

static const char *TAG = "MessageRouterTests";

using namespace messageRouter;

TEST_CASE("MessageRouter: Basic Operations", "[message-router]") {
    ESP_LOGI(TAG, "Running MessageRouter: Basic Operations");

    auto dummySubscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    auto dummyPublisher = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_PUB);

    MessageRouter::getInstance()->subscribe(dummySubscriber);
    MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, "Banana"});

    sleep(1); // Wait for the message to be processed

    CHECK(dummySubscriber->lastMessage.id == ComponentId::COMPONENT_ID_TEST_PUB);
    CHECK(std::string(dummySubscriber->lastMessage.payload) == "Banana");

    MessageRouter::getInstance()->unsubscribe(dummySubscriber);
    MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, "Apple"});
    CHECK(std::string(dummySubscriber->lastMessage.payload) ==
          "Banana"); // Should still be "Banana" since we unsubscribed
}

// ============================================================================
// MULTITHREADING TESTS
// ============================================================================

TEST_CASE("MessageRouter: Concurrent Publishing from Multiple Threads", "[message-router-multithreading]") {
    ESP_LOGI(TAG, "Testing concurrent publishing from multiple threads");

    auto subscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    MessageRouter::getInstance()->subscribe(subscriber);

    const int NUM_THREADS = 4;
    const int MESSAGES_PER_THREAD = 10;
    std::atomic<int> messageCount(0);
    std::mutex countMutex;

    auto publishMessages = [&](int threadId) {
        for (int i = 0; i < MESSAGES_PER_THREAD; i++) {
            std::string payload = "Thread" + std::to_string(threadId) + "_Msg" + std::to_string(i);
            esp_err_t ret =
                MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, payload.c_str()});
            CHECK(ret == ESP_OK);
            messageCount++;
        }
    };

    // Launch multiple publishing threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(publishMessages, i);
    }

    // Wait for all threads to complete
    for (auto &t : threads) {
        t.join();
    }

    // Allow time for router task to process all messages
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Published %d total messages from %d threads", messageCount.load(), NUM_THREADS);
    CHECK(messageCount.load() == NUM_THREADS * MESSAGES_PER_THREAD);

    MessageRouter::getInstance()->unsubscribe(subscriber);
}

TEST_CASE("MessageRouter: Concurrent Subscribe/Unsubscribe Operations", "[message-router-multithreading]") {
    ESP_LOGI(TAG, "Testing concurrent subscribe/unsubscribe operations");

    const int NUM_SUBSCRIBERS = 5;
    const int NUM_OPERATIONS = 20;
    std::vector<std::shared_ptr<DummyComponent>> subscribers;

    // Pre-create subscribers
    for (int i = 0; i < NUM_SUBSCRIBERS; i++) {
        subscribers.emplace_back(std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB));
    }

    std::atomic<int> subscriptionCount(0);

    auto subscribeThread = [&]() {
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            int subIndex = i % NUM_SUBSCRIBERS;
            esp_err_t ret = MessageRouter::getInstance()->subscribe(subscribers[subIndex]);
            CHECK(ret == ESP_OK);
            subscriptionCount++;
        }
    };

    // Launch concurrent subscription threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; i++) {
        threads.emplace_back(subscribeThread);
    }

    // Wait for subscriptions to complete
    for (auto &t : threads) {
        t.join();
    }

    ESP_LOGI(TAG, "Performed %d subscription operations", subscriptionCount.load());
    CHECK(subscriptionCount.load() == 3 * NUM_OPERATIONS);

    // Clean up
    for (const auto &sub : subscribers) {
        MessageRouter::getInstance()->unsubscribe(sub);
    }
}

TEST_CASE("MessageRouter: Concurrent Publish and Subscribe", "[message-router-multithreading]") {
    ESP_LOGI(TAG, "Testing concurrent publish and subscribe operations");

    auto subscriber1 = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    auto subscriber2 = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);

    std::atomic<bool> stopPublishing(false);
    std::atomic<int> publishCount(0);

    auto publisherThread = [&]() {
        int count = 0;
        while (!stopPublishing && count < 50) {
            std::string payload = "ConcurrentMsg" + std::to_string(count);
            esp_err_t ret =
                MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, payload.c_str()});
            CHECK(ret == ESP_OK);
            publishCount++;
            count++;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    };

    auto subscriberThread = [&]() {
        vTaskDelay(pdMS_TO_TICKS(10));
        MessageRouter::getInstance()->subscribe(subscriber1);
        vTaskDelay(pdMS_TO_TICKS(15));
        MessageRouter::getInstance()->subscribe(subscriber2);
        vTaskDelay(pdMS_TO_TICKS(20));
        MessageRouter::getInstance()->unsubscribe(subscriber1);
    };

    std::thread pubThread(publisherThread);
    std::thread subThread(subscriberThread);

    pubThread.join();
    stopPublishing = true;
    subThread.join();

    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Published %d messages while subscribing/unsubscribing", publishCount.load());
    CHECK(publishCount.load() > 0);

    // Verify subscriber2 still has messages
    CHECK(subscriber2->lastMessage.id == ComponentId::COMPONENT_ID_TEST_PUB);

    MessageRouter::getInstance()->unsubscribe(subscriber2);
}

TEST_CASE("MessageRouter: Multiple Subscribers Receive Messages Concurrently", "[message-router-multithreading]") {
    ESP_LOGI(TAG, "Testing multiple subscribers receiving messages concurrently");

    const int NUM_SUBSCRIBERS = 6;
    std::vector<std::shared_ptr<DummyComponent>> subscribers;

    for (int i = 0; i < NUM_SUBSCRIBERS; i++) {
        auto sub = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
        subscribers.push_back(sub);
        MessageRouter::getInstance()->subscribe(sub);
    }

    // Publish a single message
    esp_err_t ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, "BroadcastMsg"});
    CHECK(ret == ESP_OK);

    // Wait for message delivery
    vTaskDelay(pdMS_TO_TICKS(500));

    // Verify all subscribers received the message
    for (int i = 0; i < NUM_SUBSCRIBERS; i++) {
        CHECK(subscribers[i]->lastMessage.id == ComponentId::COMPONENT_ID_TEST_PUB);
        CHECK(std::string(subscribers[i]->lastMessage.payload) == "BroadcastMsg");
    }

    // Clean up
    for (const auto &sub : subscribers) {
        MessageRouter::getInstance()->unsubscribe(sub);
    }
}

TEST_CASE("MessageRouter: High Volume Message Stress Test", "[message-router-multithreading]") {
    ESP_LOGI(TAG, "Running high volume message stress test");

    auto subscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    MessageRouter::getInstance()->subscribe(subscriber);

    const int NUM_PUBLISHER_THREADS = 3;
    const int MESSAGES_PER_THREAD = 50;
    std::atomic<int> totalMessages(0);

    auto publishMessages = [&](int threadId) {
        for (int i = 0; i < MESSAGES_PER_THREAD; i++) {
            std::string payload = "Stress" + std::to_string(threadId) + "_" + std::to_string(i);
            esp_err_t ret =
                MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, payload.c_str()});
            if (ret == ESP_OK) {
                totalMessages++;
            } else {
                ESP_LOGW(TAG, "Failed to publish message from thread %d", threadId);
            }
        }
    };

    auto startTime = std::chrono::high_resolution_clock::now();

    // Launch publisher threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_PUBLISHER_THREADS; i++) {
        threads.emplace_back(publishMessages, i);
    }

    // Wait for all threads
    for (auto &t : threads) {
        t.join();
    }

    // Wait for all messages to be processed
    vTaskDelay(pdMS_TO_TICKS(2000));

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    ESP_LOGI(TAG, "Stress test: Published %d messages in %lld ms", totalMessages.load(), duration.count());
    CHECK(totalMessages.load() == NUM_PUBLISHER_THREADS * MESSAGES_PER_THREAD);

    MessageRouter::getInstance()->unsubscribe(subscriber);
}

TEST_CASE("MessageRouter: Singleton Thread Safety", "[message-router-multithreading]") {
    ESP_LOGI(TAG, "Testing singleton thread safety with concurrent getInstance calls");

    const int NUM_THREADS = 10;
    std::vector<MessageRouter *> instances;
    std::mutex instancesMutex;

    auto getInstanceThread = [&]() {
        MessageRouter *router = MessageRouter::getInstance();
        {
            std::lock_guard<std::mutex> lock(instancesMutex);
            instances.push_back(router);
        }
    };

    // Launch multiple threads calling getInstance concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(getInstanceThread);
    }

    for (auto &t : threads) {
        t.join();
    }

    // Verify all threads got the same singleton instance
    CHECK(instances.size() == NUM_THREADS);
    for (int i = 1; i < NUM_THREADS; i++) {
        CHECK(instances[i] == instances[0]);
    }

    ESP_LOGI(TAG, "All %d threads received the same singleton instance", NUM_THREADS);
}

TEST_CASE("MessageRouter: Rapid Subscribe/Unsubscribe Cycles", "[message-router-multithreading]") {
    ESP_LOGI(TAG, "Testing rapid subscribe/unsubscribe cycles");

    auto subscriber = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
    const int CYCLES = 20;

    for (int cycle = 0; cycle < CYCLES; cycle++) {
        esp_err_t ret = MessageRouter::getInstance()->subscribe(subscriber);
        CHECK(ret == ESP_OK);

        // Publish while subscribed
        ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, "CycleMsg"});
        CHECK(ret == ESP_OK);

        vTaskDelay(pdMS_TO_TICKS(10));

        ret = MessageRouter::getInstance()->unsubscribe(subscriber);
        CHECK(ret == ESP_OK);

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGI(TAG, "Completed %d rapid subscribe/unsubscribe cycles", CYCLES);
}

TEST_CASE("MessageRouter: Publish While Modifying Subscriber List", "[message-router-multithreading]") {
    ESP_LOGI(TAG, "Testing message publishing while subscriber list is being modified");

    std::vector<std::shared_ptr<DummyComponent>> subscribers;
    const int NUM_SUBSCRIBERS = 8;
    const int NUM_ITERATIONS = 15;

    // Create initial subscribers
    for (int i = 0; i < NUM_SUBSCRIBERS; i++) {
        auto sub = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
        subscribers.push_back(sub);
        MessageRouter::getInstance()->subscribe(sub);
    }

    std::atomic<bool> stopPublishing(false);
    std::atomic<int> publishCount(0);

    auto publisherThread = [&]() {
        int count = 0;
        while (!stopPublishing && count < 100) {
            std::string payload = "ModifyMsg" + std::to_string(count);
            esp_err_t ret = MessageRouter::getInstance()->publish({ComponentId::COMPONENT_ID_TEST_PUB, payload});
            if (ret == ESP_OK) {
                publishCount++;
            }
            count++;
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    };

    auto modifierThread = [&]() {
        for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
            // Add a new subscriber
            auto newSub = std::make_shared<DummyComponent>(ComponentId::COMPONENT_ID_TEST_SUB);
            MessageRouter::getInstance()->subscribe(newSub);
            subscribers.push_back(newSub);

            vTaskDelay(pdMS_TO_TICKS(15));

            // Remove a random subscriber
            if (subscribers.size() > 1) {
                int removeIdx = (iter % (subscribers.size() - 1));
                MessageRouter::getInstance()->unsubscribe(subscribers[removeIdx]);
                subscribers.erase(subscribers.begin() + removeIdx);
            }

            vTaskDelay(pdMS_TO_TICKS(15));
        }
    };

    std::thread pubThread(publisherThread);
    std::thread modThread(modifierThread);

    modThread.join();
    stopPublishing = true;
    pubThread.join();

    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Published %d messages while modifying subscriber list", publishCount.load());
    CHECK(publishCount.load() > 0);

    // Clean up
    for (const auto &sub : subscribers) {
        MessageRouter::getInstance()->unsubscribe(sub);
    }
}
