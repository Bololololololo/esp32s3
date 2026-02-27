#include <stdio.h>

#include "gui.h"
// #include "system_info.h"
#include "lvgl.h"

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void mainTabCreate(lv_obj_t *parent);
static void settingsTabCreate(lv_obj_t *parent);
static void debugTabCreate(lv_obj_t *parent);

/**********************
 *  STATIC VARIABLES
 **********************/
static const lv_font_t *font_normal;
static const lv_font_t *font_large;

static lv_style_t style_text_muted;
static lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;

static lv_obj_t *tv;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void guiInit(void)
{
    font_normal = LV_FONT_DEFAULT;
    font_large = LV_FONT_DEFAULT;

    lv_coord_t tab_h;
    /* disp_size == DISP_SMALL */
    tab_h = 45;

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
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
                          font_normal);
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
    lv_obj_t *t2 = lv_tabview_add_tab(tv, "Logs");
    lv_obj_t *t3 = lv_tabview_add_tab(tv, "Settings");

    mainTabCreate(t1);
    debugTabCreate(t2);
    settingsTabCreate(t3);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void mainTabCreate(lv_obj_t *parent)
{
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

static void settingsTabCreate(lv_obj_t *parent)
{
    lv_obj_t *cb;

    lv_obj_t *settings = lv_obj_create(parent);

    lv_obj_add_flag(settings, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_set_height(settings, LV_PCT(100));

    lv_obj_set_flex_flow(settings, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(settings, 1);

    lv_obj_t *title = lv_label_create(settings);
    lv_label_set_text(title, "Settings");
    lv_obj_add_style(title, &style_title, 0);

    cb = lv_checkbox_create(settings);
    lv_checkbox_set_text(cb, "Enable setting 1");
    cb = lv_checkbox_create(settings);
    lv_checkbox_set_text(cb, "Enable setting 2");
    cb = lv_checkbox_create(settings);
    lv_checkbox_set_text(cb, "Enable setting 3");
}

static void debugTabCreate(lv_obj_t *parent)
{
    lv_obj_t *panel1 = lv_obj_create(parent);
    lv_obj_set_height(panel1, LV_SIZE_CONTENT);

    // Get information about CPU, RAM and storage usage
    lv_obj_t *cpu_freq_label = lv_label_create(panel1);
    // lv_label_set_text_fmt(cpu_freq_label, "CPU Frequency: %" LV_PRId32 " MHz", getCpuFreqMHz());

    lv_obj_t *total_free_heap_label = lv_label_create(panel1);
    // lv_label_set_text_fmt(total_free_heap_label, "Total free heap available:  %u KB", getFreeHeapSizeKB());

    lv_obj_t *total_psram_label = lv_label_create(panel1);
    // lv_label_set_text_fmt(total_psram_label, "Total PSRAM available:  %u KB", getPsramSize());

    lv_obj_t *total_mem_pool = lv_label_create(panel1);
    // lv_label_set_text_fmt(total_mem_pool, "Total memory pool:  %u KB", getTotalHeapSizeKB());

    lv_obj_t *storage_size_label = lv_label_create(panel1);
    lv_label_set_text(storage_size_label, "Storage Size: 16GB");

    static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

    /*Create the top panel*/
    static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT, /*CPU freq*/
                                          LV_GRID_CONTENT, /*RAM size*/
                                          LV_GRID_CONTENT, /*Storage size*/
                                          LV_GRID_CONTENT,
                                          LV_GRID_CONTENT,
                                          LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);

    lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(cpu_freq_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(total_free_heap_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(total_psram_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
    lv_obj_set_grid_cell(total_mem_pool, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_grid_cell(storage_size_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 5, 1);
}
