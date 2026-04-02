#include <iostream>

#include "esp_log.h"
#include "gui.h"

static const char *TAG = "gui";

void Gui::event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        const char *txt = lv_checkbox_get_text(obj);
        const char *state = lv_obj_get_state(obj) & LV_STATE_CHECKED ? "Checked" : "Unchecked";
        LV_LOG_USER("%s: %s", txt, state);
        ESP_LOGI(TAG, "Button pressed txt : %s  state : %s", txt, state);
    }
}

void Gui::init() {
    font_normal = LV_FONT_DEFAULT;
    font_large = LV_FONT_DEFAULT;

    /* disp_size == DISP_SMALL */
    lv_coord_t tab_h = 45;

#if LV_FONT_MONTSERRAT_12
    font_normal = &lv_font_montserrat_12;
#else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif

#if LV_FONT_MONTSERRAT_24
    font_large = &lv_font_montserrat_24;
#else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif

#if LV_USE_THEME_DEFAULT
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                          LV_THEME_DEFAULT_DARK, font_normal);
#endif

    lv_style_init(&style_text_muted);
    lv_style_set_text_opa(&style_text_muted, LV_OPA_50);

    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, font_large);

    lv_style_init(&style_icon);
    lv_style_set_text_color(&style_icon, lv_theme_get_color_primary(NULL));
    lv_style_set_text_font(&style_icon, font_large);

    lv_style_init(&style_bullet);
    lv_style_set_border_width(&style_bullet, 0);
    lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

    lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);

    tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, tab_h);

    lv_obj_t *t1 = lv_tabview_add_tab(tv, "Main");
    lv_obj_t *t2 = lv_tabview_add_tab(tv, "Debug");
    lv_obj_t *t3 = lv_tabview_add_tab(tv, "Settings");

    mainTabCreate(t1);
    debugTabCreate(t2);
    settingsTabCreate(t3);
}

void Gui::mainTabCreate(lv_obj_t *parent) {
    lv_obj_t *panel1 = lv_obj_create(parent);
    lv_obj_set_height(panel1, LV_SIZE_CONTENT);

    lv_obj_t *menu_btn1 = lv_btn_create(panel1);
    lv_obj_set_height(menu_btn1, LV_SIZE_CONTENT);

    lv_obj_t *label = lv_label_create(menu_btn1);
    lv_label_set_text(label, "Menu 1");
    lv_obj_center(label);

    lv_obj_t *menu_btn2 = lv_btn_create(panel1);
    lv_obj_set_height(menu_btn2, LV_SIZE_CONTENT);

    lv_obj_t *label1 = lv_label_create(menu_btn2);
    lv_label_set_text(label1, "Menu 2");
    lv_obj_center(label1);

    lv_obj_t *menu_btn3 = lv_btn_create(panel1);
    lv_obj_set_height(menu_btn3, LV_SIZE_CONTENT);

    lv_obj_t *label2 = lv_label_create(menu_btn3);
    lv_label_set_text(label2, "Menu 3");
    lv_obj_center(label2);

    static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

    /*Create the top panel*/
    static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT, /*Button1*/
                                          LV_GRID_CONTENT, /*Button2*/
                                          LV_GRID_CONTENT, /*Button3*/
                                          LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);

    lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(menu_btn1, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(menu_btn2, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(menu_btn3, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 2, 1);
}

void Gui::settingsTabCreate(lv_obj_t *parent) {
    lv_obj_t *panel1 = lv_obj_create(parent);
    lv_obj_add_flag(panel1, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_set_height(panel1, LV_PCT(100));

    lv_obj_set_flex_flow(panel1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(panel1, 1);

    lv_obj_t *cb;
    cb = lv_checkbox_create(panel1);
    lv_checkbox_set_text(cb, "Seting 1");
    lv_obj_add_state(cb, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(cb, event_handler, LV_EVENT_ALL, NULL);

    cb = lv_checkbox_create(panel1);
    lv_checkbox_set_text(cb, "Setting 2");
    lv_obj_add_state(cb, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(cb, event_handler, LV_EVENT_ALL, NULL);

    cb = lv_checkbox_create(panel1);
    lv_checkbox_set_text(cb, "Setting 3");
    lv_obj_add_state(cb, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(cb, event_handler, LV_EVENT_ALL, NULL);

    cb = lv_checkbox_create(panel1);
    lv_obj_add_state(cb, LV_STATE_CHECKED | LV_STATE_DISABLED);
    lv_checkbox_set_text(cb, "Setting 4");
    lv_obj_add_event_cb(cb, event_handler, LV_EVENT_ALL, NULL);

    lv_obj_update_layout(cb);
}

void Gui::debugTabCreate(lv_obj_t *parent) {
    lv_obj_t *panel1 = lv_obj_create(parent);
    lv_obj_set_height(panel1, LV_SIZE_CONTENT);

    // Get information about CPU, RAM and storage usage
    lv_obj_t *cpu_freq_label = lv_label_create(panel1);
    lv_label_set_text_fmt(cpu_freq_label, "CPU Frequency: %" LV_PRId32 " MHz", sys_info.getCpuFreqMHz());

    lv_obj_t *total_free_dram_label = lv_label_create(panel1);
    lv_label_set_text_fmt(total_free_dram_label, "Total free DRAM available:  %u KB",
                          sys_info.getTotalFreeDRAMSizeKB());

    lv_obj_t *total_free_iram_label = lv_label_create(panel1);
    lv_label_set_text_fmt(total_free_iram_label, "Total free Internal DRAM available:  %u KB",
                          sys_info.getFreeInternalDRAMSizeKB());

    lv_obj_t *total_internal_mem_pool = lv_label_create(panel1);
    lv_label_set_text_fmt(total_internal_mem_pool, "Total Internal RAM size:  %u KB",
                          sys_info.getTotalInternalSizeKB());

    lv_obj_t *total_external_mem_pool = lv_label_create(panel1);
    lv_label_set_text_fmt(total_external_mem_pool, "Total External PSRAM available:  %u KB",
                          sys_info.getTotalPsramSize());

    lv_obj_t *storage_size_label = lv_label_create(panel1);
    lv_label_set_text(storage_size_label, "Storage Size: 16GB");

    static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

    /*Create the top panel*/
    static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT, /*CPU freq*/
                                          LV_GRID_CONTENT, /*RAM size*/
                                          LV_GRID_CONTENT, /*IRAM size*/
                                          LV_GRID_CONTENT, /*PSRAM size*/
                                          LV_GRID_CONTENT, /*Memory pool size*/
                                          LV_GRID_CONTENT, /*Storage size*/
                                          LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);

    lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(cpu_freq_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(total_free_dram_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(total_free_iram_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
    lv_obj_set_grid_cell(total_internal_mem_pool, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_grid_cell(total_external_mem_pool, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 5, 1);
    lv_obj_set_grid_cell(storage_size_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
}
