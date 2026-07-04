#include <iostream>

#include "esp_log.h"
#include "gui.h"

#include "esp_timer.h"
#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

static const char *TAG = "gui";

////////
#include <inttypes.h>


#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_ops.h"

#include "esp_lcd_ili9341.h"
///////////

#define TEST_LCD_HOST SPI2_HOST
#define TEST_LCD_H_RES (320)
#define TEST_LCD_V_RES (240)
#define TEST_LCD_BIT_PER_PIXEL (16)

#define TEST_PIN_NUM_LCD_CS (GPIO_NUM_10)
#define TEST_PIN_NUM_LCD_PCLK (GPIO_NUM_12)
#define TEST_PIN_NUM_LCD_DATA0 (GPIO_NUM_11)
#define TEST_PIN_NUM_LCD_DC (GPIO_NUM_46)
#if CONFIG_IDF_TARGET_ESP32S3
#define TEST_PIN_NUM_LCD_RST (GPIO_NUM_18)
#else
#define TEST_PIN_NUM_LCD_RST (GPIO_NUM_1)
#endif
#if CONFIG_IDF_TARGET_ESP32S3
#define TEST_PIN_NUM_LCD_BL (GPIO_NUM_45)
#else
#define TEST_PIN_NUM_LCD_BL (GPIO_NUM_0)
#endif

// Task created for GUI to run in separate core
void vTaskGui(void *pvParameters) {
    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void inc_lvgl_tick(void *arg) {
    lv_tick_inc(1);
}

static void test_draw_bitmap(esp_lcd_panel_handle_t panel_handle) {
    uint16_t row_line = TEST_LCD_V_RES / TEST_LCD_BIT_PER_PIXEL;
    uint8_t byte_per_pixel = TEST_LCD_BIT_PER_PIXEL / 8;
    uint8_t *color = (uint8_t *)heap_caps_calloc(1, row_line * TEST_LCD_H_RES * byte_per_pixel, MALLOC_CAP_DMA);

    // Red in RGB565 = 0xF800; panel is BGR-order so 0xF800 maps red to the R channel
    uint16_t red_pixel = SPI_SWAP_DATA_TX(0x00F8, TEST_LCD_BIT_PER_PIXEL);
    for (int i = 0; i < row_line * TEST_LCD_H_RES; i++) {
        color[i * byte_per_pixel + 0] = (red_pixel >> 0) & 0xFF;
        color[i * byte_per_pixel + 1] = (red_pixel >> 8) & 0xFF;
    }

    for (int j = 0; j < TEST_LCD_BIT_PER_PIXEL; j++) {
        esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, TEST_LCD_H_RES, (j + 1) * row_line, color);
    }
    free(color);
}

void Gui::initDisplay(void) {
    ESP_LOGI(TAG, "Turn on the backlight");
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(TEST_PIN_NUM_LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(TEST_PIN_NUM_LCD_BL, 1);

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .mosi_io_num = TEST_PIN_NUM_LCD_DATA0,
        .miso_io_num = -1,
        .sclk_io_num = TEST_PIN_NUM_LCD_PCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TEST_LCD_H_RES * 80 * TEST_LCD_BIT_PER_PIXEL / 8,
    };
    spi_bus_initialize(TEST_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config =
        ILI9341_PANEL_IO_SPI_CONFIG(TEST_PIN_NUM_LCD_CS, TEST_PIN_NUM_LCD_DC, NULL, NULL);
    // Attach the LCD to the SPI bus
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TEST_LCD_HOST, &io_config, &io_handle);

    ESP_LOGI(TAG, "Install ili9341 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
#endif
        .bits_per_pixel = TEST_LCD_BIT_PER_PIXEL,
        .reset_gpio_num = TEST_PIN_NUM_LCD_RST,
    };
    esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, true);
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_lcd_panel_disp_off(panel_handle, false);
#else
    esp_lcd_panel_disp_on_off(panel_handle, true);
#endif

    test_draw_bitmap(panel_handle);
}

//void Gui::event_handler(lv_event_t *e) {
    // lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t *obj = lv_event_get_target(e);
    // if (code == LV_EVENT_VALUE_CHANGED) {
    //     const char *txt = lv_checkbox_get_text(obj);
    //     const char *state = lv_obj_get_state(obj) & LV_STATE_CHECKED ? "Checked" : "Unchecked";
    //     LV_LOG_USER("%s: %s", txt, state);
    //     ESP_LOGI(TAG, "Button pressed txt : %s  state : %s", txt, state);
    // }
//}

void Gui::init() {
    lv_init(); // init lvgl
    initDisplay();
    
    const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &inc_lvgl_tick, .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 1000));

        // Run GUI task on a separate core to ensure smooth performance
    xTaskCreatePinnedToCore(vTaskGui,                 // Function
                            "Core1_Task",             // Name
                            8192,                     // Stack size
                            NULL,                     // Parameter
                            configMAX_PRIORITIES - 1, // Priority
                            NULL,                     // Handle
                            1                         // <--- Pinned to Core 1
    );
//     font_normal = LV_FONT_DEFAULT;
//     font_large = LV_FONT_DEFAULT;

//     /* disp_size == DISP_SMALL */
//     lv_coord_t tab_h = 45;

// #if LV_FONT_MONTSERRAT_12
//     font_normal = &lv_font_montserrat_12;
// #else
//     LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
// #endif

// #if LV_FONT_MONTSERRAT_24
//     font_large = &lv_font_montserrat_24;
// #else
//     LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
// #endif

// #if LV_USE_THEME_DEFAULT
//     lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
//                           LV_THEME_DEFAULT_DARK, font_normal);
// #endif

//     lv_style_init(&style_text_muted);
//     lv_style_set_text_opa(&style_text_muted, LV_OPA_50);

//     lv_style_init(&style_title);
//     lv_style_set_text_font(&style_title, font_large);

//     lv_style_init(&style_icon);
//     lv_style_set_text_color(&style_icon, lv_theme_get_color_primary(NULL));
//     lv_style_set_text_font(&style_icon, font_large);

//     lv_style_init(&style_bullet);
//     lv_style_set_border_width(&style_bullet, 0);
//     lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

//     lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);

//     tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, tab_h);

//     lv_obj_t *t1 = lv_tabview_add_tab(tv, "Main");
//     lv_obj_t *t2 = lv_tabview_add_tab(tv, "Debug");
//     lv_obj_t *t3 = lv_tabview_add_tab(tv, "Settings");

//     mainTabCreate(t1);
//     debugTabCreate(t2);
//     settingsTabCreate(t3);
}

// void Gui::mainTabCreate(lv_obj_t *parent) {
    // lv_obj_t *panel1 = lv_obj_create(parent);
    // lv_obj_set_height(panel1, LV_SIZE_CONTENT);

    // lv_obj_t *menu_btn1 = lv_btn_create(panel1);
    // lv_obj_set_height(menu_btn1, LV_SIZE_CONTENT);

    // lv_obj_t *label = lv_label_create(menu_btn1);
    // lv_label_set_text(label, "Menu 1");
    // lv_obj_center(label);

    // lv_obj_t *menu_btn2 = lv_btn_create(panel1);
    // lv_obj_set_height(menu_btn2, LV_SIZE_CONTENT);

    // lv_obj_t *label1 = lv_label_create(menu_btn2);
    // lv_label_set_text(label1, "Menu 2");
    // lv_obj_center(label1);

    // lv_obj_t *menu_btn3 = lv_btn_create(panel1);
    // lv_obj_set_height(menu_btn3, LV_SIZE_CONTENT);

    // lv_obj_t *label2 = lv_label_create(menu_btn3);
    // lv_label_set_text(label2, "Menu 3");
    // lv_obj_center(label2);

    // static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    // static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
    // LV_GRID_TEMPLATE_LAST}; lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

    // /*Create the top panel*/
    // static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    // static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT, /*Button1*/
    //                                       LV_GRID_CONTENT, /*Button2*/
    //                                       LV_GRID_CONTENT, /*Button3*/
    //                                       LV_GRID_TEMPLATE_LAST};

    // lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);

    // lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    // lv_obj_set_grid_cell(menu_btn1, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
    // lv_obj_set_grid_cell(menu_btn2, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 1, 1);
    // lv_obj_set_grid_cell(menu_btn3, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 2, 1);
//}   

//void Gui::settingsTabCreate(lv_obj_t *parent) {
    // lv_obj_t *panel1 = lv_obj_create(parent);
    // lv_obj_add_flag(panel1, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    // lv_obj_set_height(panel1, LV_PCT(100));

    // lv_obj_set_flex_flow(panel1, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_flex_grow(panel1, 1);

    // lv_obj_t *cb;
    // cb = lv_checkbox_create(panel1);
    // lv_checkbox_set_text(cb, "Seting 1");
    // lv_obj_add_state(cb, LV_STATE_DEFAULT);
    // lv_obj_add_event_cb(cb, event_handler, LV_EVENT_ALL, NULL);

    // cb = lv_checkbox_create(panel1);
    // lv_checkbox_set_text(cb, "Setting 2");
    // lv_obj_add_state(cb, LV_STATE_DEFAULT);
    // lv_obj_add_event_cb(cb, event_handler, LV_EVENT_ALL, NULL);

    // cb = lv_checkbox_create(panel1);
    // lv_checkbox_set_text(cb, "Setting 3");
    // lv_obj_add_state(cb, LV_STATE_DEFAULT);
    // lv_obj_add_event_cb(cb, event_handler, LV_EVENT_ALL, NULL);

    // cb = lv_checkbox_create(panel1);
    // lv_obj_add_state(cb, LV_STATE_CHECKED | LV_STATE_DISABLED);
    // lv_checkbox_set_text(cb, "Setting 4");
    // lv_obj_add_event_cb(cb, event_handler, LV_EVENT_ALL, NULL);

    // lv_obj_update_layout(cb);
//}

//void Gui::debugTabCreate(lv_obj_t *parent) {
    // lv_obj_t *panel1 = lv_obj_create(parent);
    // lv_obj_set_height(panel1, LV_SIZE_CONTENT);

    // // Get information about CPU, RAM and storage usage
    // lv_obj_t *cpu_freq_label = lv_label_create(panel1);
    // lv_label_set_text_fmt(cpu_freq_label, "CPU Frequency: %" LV_PRId32 " MHz", sys_info.getCpuFreqMHz());

    // lv_obj_t *total_free_dram_label = lv_label_create(panel1);
    // lv_label_set_text_fmt(total_free_dram_label, "Total free DRAM available:  %u KB",
    //                       sys_info.getTotalFreeDRAMSizeKB());

    // lv_obj_t *total_free_iram_label = lv_label_create(panel1);
    // lv_label_set_text_fmt(total_free_iram_label, "Total free Internal DRAM available:  %u KB",
    //                       sys_info.getFreeInternalDRAMSizeKB());

    // lv_obj_t *total_internal_mem_pool = lv_label_create(panel1);
    // lv_label_set_text_fmt(total_internal_mem_pool, "Total Internal RAM size:  %u KB",
    //                       sys_info.getTotalInternalSizeKB());

    // lv_obj_t *total_external_mem_pool = lv_label_create(panel1);
    // lv_label_set_text_fmt(total_external_mem_pool, "Total External PSRAM available:  %u KB",
    //                       sys_info.getTotalPsramSize());

    // lv_obj_t *storage_size_label = lv_label_create(panel1);
    // lv_label_set_text(storage_size_label, "Storage Size: 16GB");

    // static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    // static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
    // LV_GRID_TEMPLATE_LAST}; lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

    // /*Create the top panel*/
    // static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    // static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT, /*CPU freq*/
    //                                       LV_GRID_CONTENT, /*RAM size*/
    //                                       LV_GRID_CONTENT, /*IRAM size*/
    //                                       LV_GRID_CONTENT, /*PSRAM size*/
    //                                       LV_GRID_CONTENT, /*Memory pool size*/
    //                                       LV_GRID_CONTENT, /*Storage size*/
    //                                       LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

    // lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);

    // lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    // lv_obj_set_grid_cell(cpu_freq_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    // lv_obj_set_grid_cell(total_free_dram_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    // lv_obj_set_grid_cell(total_free_iram_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
    // lv_obj_set_grid_cell(total_internal_mem_pool, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    // lv_obj_set_grid_cell(total_external_mem_pool, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 5, 1);
    // lv_obj_set_grid_cell(storage_size_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
//}
