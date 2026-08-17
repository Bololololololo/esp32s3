/**
 * @file screen1_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen1_gen.h"
#include "../examples.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * screen1_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    static bool style_inited = false;

    if (!style_inited) {

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
    lv_obj_set_name_static(lv_obj_0, "screen1_#");

    lv_obj_t * roller_1 = roller_create(lv_obj_0, &room_temp);
    lv_obj_set_name(roller_1, "roller_1");
    lv_obj_set_width(roller_1, 320);
    lv_obj_set_height(roller_1, 240);
    lv_roller_set_options(roller_1, "home\nsettings\ninfo", LV_ROLLER_MODE_INFINITE);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

