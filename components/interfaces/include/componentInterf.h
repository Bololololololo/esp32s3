#ifndef COMPONENT_INTERFACE_H
#define COMPONENT_INTERFACE_H

#include "componentIds.h"
#include "esp_err.h"

struct Message {
    ComponentId id;
    char payload[256];
};

class ComponentInterface {
  private:
    ComponentId componentId;

  public:
    ComponentInterface(ComponentId id) : componentId(id) {
    }

    ComponentId getComponentId() const {
        return componentId;
    }

    virtual ~ComponentInterface() = default;
    virtual esp_err_t messageHandler(const Message &msg) = 0;
};

#endif // COMPONENT_INTERFACE_H