#include <iostream>

#include "esp_log.h"
#include "gui.h"

#include "esp_timer.h"
#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"

#include "esp_lvgl_port.h"

#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_ft6x36.h"

static const char *TAG = "gui";

#define LCD_HOST SPI2_HOST
#define LCD_H_RES (320)
#define LCD_V_RES (240)
#define LCD_BIT_PER_PIXEL (16)
#define LCD_DRAW_BUFF_HEIGHT (50)
#define LCD_DRAW_BUFF_DOUBLE (true)

#define PIN_NUM_LCD_CS (GPIO_NUM_10)
#define PIN_NUM_LCD_PCLK (GPIO_NUM_12)
#define PIN_NUM_LCD_DATA0 (GPIO_NUM_11)
#define PIN_NUM_LCD_DC (GPIO_NUM_46)
#define PIN_NUM_LCD_RST (GPIO_NUM_18)
#define PIN_NUM_LCD_BL (GPIO_NUM_45)

/* Touch settings */
#define TOUCH_I2C_NUM       (0)
#define TOUCH_I2C_CLK_HZ    (400000)

/* LCD touch pins */
#define TOUCH_I2C_SCL       (GPIO_NUM_15)
#define TOUCH_I2C_SDA       (GPIO_NUM_16)
#define TOUCH_GPIO_INT      (GPIO_NUM_17)
#define TOUCH_GPIO_RST      (GPIO_NUM_18)

/* LVGL display and touch */
static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static i2c_master_bus_handle_t i2c_handle = NULL;
static esp_lcd_panel_io_handle_t tp_io_handle= NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

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

static SemaphoreHandle_t refresh_finish = NULL;

IRAM_ATTR static bool test_notify_refresh_ready(esp_lcd_panel_io_handle_t panel_io,
                                                esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    BaseType_t need_yield = pdFALSE;

    xSemaphoreGiveFromISR(refresh_finish, &need_yield);
    return (need_yield == pdTRUE);
}

static void test_draw_bitmap(esp_lcd_panel_handle_t panel_handle) {
    refresh_finish = xSemaphoreCreateBinary();
    uint16_t row_line = LCD_V_RES / LCD_BIT_PER_PIXEL;
    uint8_t byte_per_pixel = LCD_BIT_PER_PIXEL / 8;
    uint8_t *color = (uint8_t *)heap_caps_calloc(1, row_line * LCD_H_RES * byte_per_pixel, MALLOC_CAP_DMA);

    // Red in RGB565 = 0xF800; panel is BGR-order so 0xF800 maps red to the R channel
    uint16_t red_pixel = SPI_SWAP_DATA_TX(0x00F8, LCD_BIT_PER_PIXEL);
    for (int i = 0; i < row_line * LCD_H_RES; i++) {
        color[i * byte_per_pixel + 0] = (red_pixel >> 0) & 0xFF;
        color[i * byte_per_pixel + 1] = (red_pixel >> 8) & 0xFF;
    }

    for (int j = 0; j < LCD_BIT_PER_PIXEL; j++) {
        esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, LCD_H_RES, (j + 1) * row_line, color);
        xSemaphoreTake(refresh_finish, portMAX_DELAY);
    }
    vSemaphoreDelete(refresh_finish);
    free(color);
}

static esp_err_t app_lvgl_init(int task_affinity)
{
    /* Initialize LVGL */
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_affinity = task_affinity;
    lvgl_port_init(&lvgl_cfg);

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
        .double_buffer = LCD_DRAW_BUFF_DOUBLE,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = true,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
        }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    return ESP_OK;
}

static void _app_button_cb(lv_event_t *e)
{
    lv_disp_rotation_t rotation = lv_disp_get_rotation(lvgl_disp);
    rotation = static_cast<lv_disp_rotation_t>(static_cast<uint8_t>(rotation) + 1);
    if (rotation > LV_DISPLAY_ROTATION_270) {
        rotation = LV_DISPLAY_ROTATION_0;
    }

    /* LCD HW rotation */
    lv_disp_set_rotation(lvgl_disp, rotation);
}

static void app_main_display(void)
{
    lv_obj_t *scr = lv_scr_act();

    /* Task lock */
    lvgl_port_lock(0);

    /* Your LVGL objects code here .... */

    /* Label */
    lv_obj_t *label = lv_label_create(scr);
    lv_obj_set_width(label, LCD_H_RES);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
#if LVGL_VERSION_MAJOR == 8
    lv_label_set_recolor(label, true);
    lv_label_set_text(label,
                      "#FF0000 "LV_SYMBOL_BELL" Hello world Espressif and LVGL "LV_SYMBOL_BELL"#\n#FF9400 "LV_SYMBOL_WARNING" For simplier initialization, use BSP "LV_SYMBOL_WARNING" #");
#else
    lv_label_set_text(label,
                      LV_SYMBOL_BELL" Hello world Espressif and LVGL "LV_SYMBOL_BELL"\n "LV_SYMBOL_WARNING" For simplier initialization, use BSP "LV_SYMBOL_WARNING);
#endif
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

    /* Button */
    lv_obj_t *btn = lv_btn_create(scr);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, "Rotate screen");
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_event_cb(btn, _app_button_cb, LV_EVENT_CLICKED, NULL);

    /* Task unlock */
    lvgl_port_unlock();
}

void Gui::initDisplay(void) {
    ESP_LOGI(TAG, "Turn on the backlight");
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(PIN_NUM_LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_NUM_LCD_BL, 1);

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_LCD_DATA0,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_LCD_PCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    ESP_LOGI(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config =
        ILI9341_PANEL_IO_SPI_CONFIG(PIN_NUM_LCD_CS, PIN_NUM_LCD_DC, NULL, NULL);
    // Attach the LCD to the SPI bus
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &lcd_io);

    ESP_LOGI(TAG, "Install ili9341 panel driver");
    const esp_lcd_panel_dev_config_t panel_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
#endif
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .reset_gpio_num = PIN_NUM_LCD_RST,
    };
    esp_lcd_new_panel_ili9341(lcd_io, &panel_config, &lcd_panel);
    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_swap_xy(lcd_panel, true);
    esp_lcd_panel_mirror(lcd_panel, true, true);
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_lcd_panel_disp_off(lcd_panel, false);
#else
    esp_lcd_panel_disp_on_off(lcd_panel, true);
#endif

    //test_draw_bitmap(lcd_panel);
}


void Gui::initTouch(void)
{
    /* Initilize I2C */
    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = TOUCH_I2C_NUM,
        .sda_io_num = TOUCH_I2C_SDA,
        .scl_io_num = TOUCH_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };
     
    i2c_new_master_bus(&i2c_config, &i2c_handle);

    /* Initialize touch HW */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        .int_gpio_num = TOUCH_GPIO_INT,
        .levels = {
            .reset = 1,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 0,
            .mirror_y = 1,
        },
    };
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT6x36_CONFIG();
    tp_io_config.scl_speed_hz = TOUCH_I2C_CLK_HZ;
    esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &tp_io_handle);
    // we need to wait so the i2c has time to initialize
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_lcd_touch_new_i2c_ft6x36(tp_io_handle, &tp_cfg, &touch_handle);
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
    initDisplay();
    initTouch();
    
    app_lvgl_init(-1); // Initialize LVGL with no task affinity, allowing it to run on any core
    //lv_init(); // init lvgl
    
    app_main_display();

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
