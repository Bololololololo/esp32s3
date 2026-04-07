#ifndef MESSAGE_ROUTER_H
#define MESSAGE_ROUTER_H

#include "componentIds.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h" // Usually needed to manage the tasks using the queue
#include <memory>
#include <stdio.h>

namespace messageRouter {

struct Message {
    ComponentId id;
    char payload[256];
};

class MessageRouter {
  private:
    static std::unique_ptr<MessageRouter> instance;
    struct _cons {
        explicit _cons() = default;
    };

    TaskHandle_t xRouterTask;
    QueueHandle_t xRouterQueue;

  public:
    MessageRouter(_cons) {
    }

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

    esp_err_t initialize();

    esp_err_t subscribe(char *topic);
    esp_err_t publish(char *topic, const Message &message);
};
} // namespace messageRouter

#endif // MESSAGE_ROUTER_H