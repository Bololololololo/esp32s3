#ifndef MESSAGE_ROUTER_H
#define MESSAGE_ROUTER_H

#include "componentIds.h"
#include "component_interface.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h" // Usually needed to manage the tasks using the queue
#include <memory>
#include <stdio.h>
#include <unordered_map>
#include <vector>

namespace messageRouter {

class MessageRouter {
  private:
    static std::unique_ptr<MessageRouter> instance;
    struct _cons {
        explicit _cons() = default;
    };

    std::unordered_map<ComponentId, std::vector<std::shared_ptr<ComponentInterface>>> subscribersDB;

    TaskHandle_t xRouterTask;
    QueueHandle_t xRouterQueue;

  public:
    MessageRouter(_cons);

    static std::unique_ptr<MessageRouter> instanceFactory() {
        return std::make_unique<MessageRouter>(_cons{});
    }

    static MessageRouter *getInstance() {
        /*
            If control enters the declaration concurrently while the variable is being initialized,
            the concurrent execution shall wait for completion of the initialization.
        */
        static std::unique_ptr<MessageRouter> instance{MessageRouter::instanceFactory()};
        return instance.get();
    }

    esp_err_t subscribe(std::shared_ptr<ComponentInterface> subscriber);
    esp_err_t unsubscribe(std::shared_ptr<ComponentInterface> subscriber);
    esp_err_t publish(const Message &message);
};
} // namespace messageRouter

#endif // MESSAGE_ROUTER_H