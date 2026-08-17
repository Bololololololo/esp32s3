#include "robot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "lvgl.h"
// #include <stdlib.h>

// Eye Widgets
static lv_obj_t *left_eye;
static lv_obj_t *right_eye;
static lv_obj_t *mouth;

// Eye Configuration Dimensions
#define EYE_WIDTH 70
#define EYE_HEIGHT 100
#define EYE_RADIUS 25
#define EYE_COLOR lv_color_hex(0x000000) // Cyan / Neon Blue
#define BG_COLOR lv_color_hex(0xFFFFFF)  // Deep Black

// Robot Emotions
typedef enum {
    EMOTION_NEUTRAL,
    EMOTION_HAPPY,
    EMOTION_SAD,
    EMOTION_ANGRY
} robot_emotion_t;

static robot_emotion_t current_emotion = EMOTION_NEUTRAL;

/* Set the facial expression */
static void robot_set_emotion(robot_emotion_t emotion) {
    current_emotion = emotion;

    switch (emotion) {
    case EMOTION_NEUTRAL:
        lv_obj_set_size(left_eye, EYE_WIDTH, EYE_HEIGHT);
        lv_obj_set_size(right_eye, EYE_WIDTH, EYE_HEIGHT);
        lv_obj_set_style_transform_angle(left_eye, 0, 0);
        lv_obj_set_style_transform_angle(right_eye, 0, 0);
        break;

    case EMOTION_HAPPY:
        // Squinted eyes / taller curves
        lv_obj_set_size(left_eye, EYE_WIDTH + 10, EYE_HEIGHT - 30);
        lv_obj_set_size(right_eye, EYE_WIDTH + 10, EYE_HEIGHT - 30);
        lv_obj_set_style_transform_angle(left_eye, -100, 0); // -10 degrees
        lv_obj_set_style_transform_angle(right_eye, 100, 0); // 10 degrees
        break;

    case EMOTION_ANGRY:
        // Slanted inward eyes
        lv_obj_set_size(left_eye, EYE_WIDTH, EYE_HEIGHT - 10);
        lv_obj_set_size(right_eye, EYE_WIDTH, EYE_HEIGHT - 10);
        lv_obj_set_style_transform_angle(left_eye, 200, 0); // Slant in
        lv_obj_set_style_transform_angle(right_eye, -200, 0);
        break;

    case EMOTION_SAD:
        // Slanted outward eyes
        lv_obj_set_style_transform_angle(left_eye, -150, 0);
        lv_obj_set_style_transform_angle(right_eye, 150, 0);
        break;
    }
}

/* Perform a quick blink animation */
void robot_blink(void) {
    // Animate eye height down to 4px (closed)
    lv_anim_t a_left, a_right;

    lv_anim_init(&a_left);
    lv_anim_set_var(&a_left, left_eye);
    lv_anim_set_values(&a_left, lv_obj_get_height(left_eye), 4);
    lv_anim_set_time(&a_left, 120);
    lv_anim_set_playback_time(&a_left, 120); // Re-open
    lv_anim_set_exec_cb(&a_left, (lv_anim_exec_xcb_t)lv_obj_set_height);

    lv_anim_init(&a_right);
    lv_anim_set_var(&a_right, right_eye);
    lv_anim_set_values(&a_right, lv_obj_get_height(right_eye), 4);
    lv_anim_set_time(&a_right, 120);
    lv_anim_set_playback_time(&a_right, 120);
    lv_anim_set_exec_cb(&a_right, (lv_anim_exec_xcb_t)lv_obj_set_height);

    lv_anim_start(&a_left);
    lv_anim_start(&a_right);
}

/* Background animation task */
static void robot_face_anim_task(void *pvParameters) {
    while (1) {
        // Random interval between blinks (2 to 6 seconds)

        vTaskDelay(pdMS_TO_TICKS(2000 + (rand() % 4000)));

        // Acquire LVGL lock if using multithreaded FreeRTOS LVGL port
        // lvgl_port_lock(0);

        static int counter = 0;
        static int emotion = 0;
        counter++;
        emotion++;
        if (counter <= 3) {
            robot_blink();
        } else {
            counter = 0;
            robot_emotion_t new_emotion = (robot_emotion_t)(emotion > 3 ? 0 : emotion);
            robot_set_emotion(new_emotion);
        }
    }
}

/* Initialize Robot Face UI */
void create_robot_face_ui(void) {
    // Get active screen and make background black
    lv_obj_t *scr = lv_screen_active(); // LVGL 8: lv_scr_act()
    lv_obj_set_style_bg_color(scr, BG_COLOR, 0);
    lv_obj_set_width(scr, 320);
    lv_obj_set_height(scr, 240);

    // ==================== LEFT EYE ====================
    left_eye = lv_obj_create(scr);
    lv_obj_remove_style_all(left_eye);

    lv_obj_set_size(left_eye, EYE_WIDTH, EYE_HEIGHT);
    lv_obj_align(left_eye, LV_ALIGN_CENTER, -65, -10);

    lv_obj_set_style_bg_color(left_eye, EYE_COLOR, 0);
    lv_obj_set_style_bg_opa(left_eye, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(left_eye, EYE_RADIUS, 0);
    lv_obj_set_style_transform_pivot_x(left_eye, EYE_WIDTH / 2, 0);
    lv_obj_set_style_transform_pivot_y(left_eye, EYE_HEIGHT / 2, 0);

    // ==================== RIGHT EYE ====================
    right_eye = lv_obj_create(scr);
    lv_obj_remove_style_all(right_eye);

    lv_obj_set_size(right_eye, EYE_WIDTH, EYE_HEIGHT);
    lv_obj_align(right_eye, LV_ALIGN_CENTER, 65, -10);

    lv_obj_set_style_bg_color(right_eye, EYE_COLOR, 0);
    lv_obj_set_style_bg_opa(right_eye, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(right_eye, EYE_RADIUS, 0);
    lv_obj_set_style_transform_pivot_x(right_eye, EYE_WIDTH / 2, 0);
    lv_obj_set_style_transform_pivot_y(right_eye, EYE_HEIGHT / 2, 0);

    // Initial emotion
    robot_set_emotion(EMOTION_NEUTRAL);

    // Create FreeRTOS task for procedural animations (blinking/looking)
    xTaskCreatePinnedToCore(robot_face_anim_task, "robot_anim", 3072, NULL, 5, NULL, 1);
}