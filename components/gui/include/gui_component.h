#ifndef GUI_H
#define GUI_H

#include "component_interface.h"
#include <iostream>
#include <memory>

namespace gui {

class GuiComponent : public ComponentInterface {
  private:
    static std::unique_ptr<GuiComponent> instance;

    struct _cons {
        explicit _cons() = default;
    };

    void initDisplay();
    void initTouch();
    esp_err_t messageHandler(const Message &msg) override;

  public:
    GuiComponent(_cons);

    static std::unique_ptr<GuiComponent> instanceFactory() {
        return std::make_unique<GuiComponent>(_cons{});
    }

    static GuiComponent *getInstance() {
        /*
            If control enters the declaration concurrently while the variable is being initialized,
            the concurrent execution shall wait for completion of the initialization.
        */
        static std::unique_ptr<GuiComponent> instance{GuiComponent::instanceFactory()};
        return instance.get();
    }

    void init();
};

} // namespace gui

#endif // GUI_H