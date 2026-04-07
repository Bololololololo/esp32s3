#include <map>
#include <string>

enum ComponentId {
    COMPONENT_ID_LVGL_DISPLAY = 0,
    COMPONENT_ID_LVGL_INPUT,
    COMPONENT_ID_WIFI,
    COMPONENT_ID_BLE,
    COMPONENT_ID_MAX
};

std::map<ComponentId, std::string> componentIdToString = {{COMPONENT_ID_LVGL_DISPLAY, "LVGL Display"},
                                                          {COMPONENT_ID_LVGL_INPUT, "LVGL Input"},
                                                          {COMPONENT_ID_WIFI, "WiFi"},
                                                          {COMPONENT_ID_BLE, "BLE"}};