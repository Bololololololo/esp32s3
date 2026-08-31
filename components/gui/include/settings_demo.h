#pragma once

#include "lvgl.h"
#include <cstdint>

enum class WifiMode {
    DISABLED = 0,
    STATION,
    ACCESS_POINT
};

enum class BluetoothMode {
    OFF = 0,
    BLE_ADVERTISING,
    SPP_CLASSIC
};

class SettingsScreen {
  public:
    SettingsScreen();
    ~SettingsScreen() = default;

    // Initialize and build the LVGL 9 UI
    void init();

    // Getters to retrieve selected states from external code/tasks
    WifiMode getWifiMode() const {
        return m_wifiMode;
    }
    BluetoothMode getBluetoothMode() const {
        return m_btMode;
    }

  private:
    lv_obj_t *m_screen{nullptr};
    lv_obj_t *m_wifiGroup{nullptr};
    lv_obj_t *m_btGroup{nullptr};

    WifiMode m_wifiMode{WifiMode::STATION};
    BluetoothMode m_btMode{BluetoothMode::OFF};

    // Helper to build radio option buttons
    lv_obj_t *createRadioOption(lv_obj_t *parent, const char *labelText, bool checked, uint32_t optionId);

    // Static event trampoline for LVGL C callback compatibility
    static void radioEventHandler(lv_event_t *e);

    // Instance method that handles radio mutual exclusion
    void handleRadioSelection(lv_obj_t *target);
};