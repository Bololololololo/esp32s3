#include "settings_demo.h"

SettingsScreen::SettingsScreen() {
    // 1. Create root screen object (240x320)
    m_screen = lv_obj_create(nullptr);
    lv_obj_set_size(m_screen, 240, 320);
    lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x121212), 0);
    lv_obj_set_flex_flow(m_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(m_screen, 8, 0);
    lv_obj_set_style_pad_row(m_screen, 10, 0);
}

void SettingsScreen::init() {
    // Title Header
    lv_obj_t *title = lv_label_create(m_screen);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    // -------------------------------------------------------------
    // Wi-Fi Section
    // -------------------------------------------------------------
    lv_obj_t *wifiLabel = lv_label_create(m_screen);
    lv_label_set_text(wifiLabel, "Wi-Fi Mode");
    lv_obj_set_style_text_color(wifiLabel, lv_color_hex(0x8E8E93), 0);

    m_wifiGroup = lv_obj_create(m_screen);
    lv_obj_set_width(m_wifiGroup, lv_pct(100));
    lv_obj_set_height(m_wifiGroup, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(m_wifiGroup, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(m_wifiGroup, 4, 0);
    lv_obj_set_style_pad_row(m_wifiGroup, 4, 0);
    lv_obj_set_style_bg_color(m_wifiGroup, lv_color_hex(0x1C1C1E), 0);
    lv_obj_set_style_border_width(m_wifiGroup, 0, 0);
    lv_obj_set_style_radius(m_wifiGroup, 10, 0);

    createRadioOption(m_wifiGroup, " Disabled", false, static_cast<uint32_t>(WifiMode::DISABLED));
    createRadioOption(m_wifiGroup, " Station (STA)", true, static_cast<uint32_t>(WifiMode::STATION));
    createRadioOption(m_wifiGroup, " Access Point (AP)", false, static_cast<uint32_t>(WifiMode::ACCESS_POINT));

    // -------------------------------------------------------------
    // Bluetooth Section
    // -------------------------------------------------------------
    lv_obj_t *btLabel = lv_label_create(m_screen);
    lv_label_set_text(btLabel, "Bluetooth Radio");
    lv_obj_set_style_text_color(btLabel, lv_color_hex(0x8E8E93), 0);

    m_btGroup = lv_obj_create(m_screen);
    lv_obj_set_width(m_btGroup, lv_pct(100));
    lv_obj_set_height(m_btGroup, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(m_btGroup, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(m_btGroup, 4, 0);
    lv_obj_set_style_pad_row(m_btGroup, 4, 0);
    lv_obj_set_style_bg_color(m_btGroup, lv_color_hex(0x1C1C1E), 0);
    lv_obj_set_style_border_width(m_btGroup, 0, 0);
    lv_obj_set_style_radius(m_btGroup, 10, 0);

    createRadioOption(m_btGroup, " Off", true, static_cast<uint32_t>(BluetoothMode::OFF));
    createRadioOption(m_btGroup, " BLE Advertising", false, static_cast<uint32_t>(BluetoothMode::BLE_ADVERTISING));
    createRadioOption(m_btGroup, " SPP Classic", false, static_cast<uint32_t>(BluetoothMode::SPP_CLASSIC));

    // Load active screen in LVGL 9
    lv_screen_load(m_screen);
}

lv_obj_t *SettingsScreen::createRadioOption(lv_obj_t *parent, const char *labelText, bool checked, uint32_t optionId) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 36);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2C2C2E), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0A84FF), LV_STATE_CHECKED);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_pad_hor(btn, 10, 0);

    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Circle Indicator
    lv_obj_t *circle = lv_obj_create(btn);
    lv_obj_set_size(circle, 16, 16);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(circle, 2, 0);
    lv_obj_set_style_border_color(circle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFFFFF), LV_STATE_CHECKED);

    // Option Text
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, labelText);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);

    // Store ID inside user_data pointer for lookup
    lv_obj_set_user_data(btn, reinterpret_cast<void *>(static_cast<uintptr_t>(optionId)));

    if (checked) {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    }

    // Attach C++ callback passing `this` pointer as user_data
    lv_obj_add_event_cb(btn, SettingsScreen::radioEventHandler, LV_EVENT_CLICKED, this);

    return btn;
}

void SettingsScreen::radioEventHandler(lv_event_t *e) {
    auto *instance = static_cast<SettingsScreen *>(lv_event_get_user_data(e));
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));

    if (instance && target) {
        instance->handleRadioSelection(target);
    }
}

void SettingsScreen::handleRadioSelection(lv_obj_t *target) {
    lv_obj_t *parent = lv_obj_get_parent(target);
    uint32_t optionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(target)));

    // Ensure target stays checked
    lv_obj_add_state(target, LV_STATE_CHECKED);

    // Uncheck all other radio siblings
    uint32_t count = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if (child != target && lv_obj_has_state(child, LV_STATE_CHECKED)) {
            lv_obj_remove_state(child, LV_STATE_CHECKED);
        }
    }

    // Update internal state depending on parent container
    if (parent == m_wifiGroup) {
        m_wifiMode = static_cast<WifiMode>(optionId);
    } else if (parent == m_btGroup) {
        m_btMode = static_cast<BluetoothMode>(optionId);
    }
}