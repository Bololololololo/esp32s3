#include "componentIds.h"

class ComponentInterface {
  private:
    ComponentId componentId;

  public:
    virtual void initialize() = default;
    virtual void messageHandler(const Message &msg) = default;
    virtual ComponentInterface(ComponentId componentId) = default;
    virtual ~ComponentInterface() = default;
};