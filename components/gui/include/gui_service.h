#ifndef _GUI_SERVICE_H_
#define _GUI_SERVICE_H_

#include "gui_component.h"
#include "service_interface.h"
#include <memory>

using namespace gui;

namespace services {
class GuiService : public ServiceInterface {
  public:
    GuiService() : ServiceInterface(ServiceType::SERVICE_GUI) {
    }
    ~GuiService() override = default;

    void start() override;
    void stop() override;

  private:
    std::unique_ptr<GuiComponent> guiComponent;
};

} // namespace services

#endif // _GUI_SERVICE_H_