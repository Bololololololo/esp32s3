#pragma once

#include "lvgl.h"
#include <cstdint>

class PixelRobotEyes {
public:
    enum class Emotion {
        Neutral,
        Happy,
        Angry,
        Sad
    };

    enum class Direction {
        Center,
        Left,
        Right
    };

    PixelRobotEyes() = default;
    ~PixelRobotEyes() = default;

    void init(lv_obj_t* parent = nullptr);
    void setEmotion(Emotion emotion);
    void lookAt(Direction direction);
    void blink(uint32_t duration_ms = 120);

private:
    // Pixel Matrix Specs
    static constexpr int32_t MATRIX_COLS = 10;
    static constexpr int32_t MATRIX_ROWS = 14;
    static constexpr int32_t PIXEL_SIZE  = 7;  // Size of each blocky "pixel" in real pixels
    static constexpr int32_t PIXEL_GAP   = 1;  // Gap between blocks for grid feel

    static constexpr int32_t CANVAS_W = MATRIX_COLS * (PIXEL_SIZE + PIXEL_GAP);
    static constexpr int32_t CANVAS_H = MATRIX_ROWS * (PIXEL_SIZE + PIXEL_GAP);
    static constexpr int32_t EYE_OFFSET_X = 65;

    // Palette Colors
    static constexpr uint32_t COLOR_EYE_NEON = 0x00FF66; // Retro Arcade Green / Cyan
    static constexpr uint32_t COLOR_BG       = 0x000000;

    lv_obj_t* m_leftCanvas  = nullptr;
    lv_obj_t* m_rightCanvas = nullptr;

    // Buffer allocations for LVGL canvas
    uint8_t m_leftBuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_W, CANVAS_H)];
    uint8_t m_rightBuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_W, CANVAS_H)];

    Emotion   m_currentEmotion   = Emotion::Neutral;
    Direction m_currentDirection = Direction::Center;

    void initCanvas(lv_obj_t*& canvas, uint8_t* buf, lv_obj_t* parent, int32_t xOffset);
    void renderEyeGrid(lv_obj_t* canvas, Emotion emotion, Direction dir, bool isLeft, bool isClosed);
    void drawPixelBlock(lv_obj_t* canvas, int32_t gridX, int32_t gridY, lv_color_t color);
};
Implementation File (PixelRobotEyes.cpp)
C++
#include "PixelRobotEyes.hpp"
#include <cstring>

void PixelRobotEyes::init(lv_obj_t* parent) {
    if (!parent) {
        parent = lv_screen_active(); // LVGL 8: lv_scr_act()
    }

    lv_obj_set_style_bg_color(parent, lv_color_hex(COLOR_BG), 0);

    // Initialize two canvases as low-res pixel screens
    initCanvas(m_leftCanvas, m_leftBuf, parent, -EYE_OFFSET_X);
    initCanvas(m_rightCanvas, m_rightBuf, parent, EYE_OFFSET_X);

    setEmotion(Emotion::Neutral);
}

void PixelRobotEyes::initCanvas(lv_obj_t*& canvas, uint8_t* buf, lv_obj_t* parent, int32_t xOffset) {
    canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, buf, CANVAS_W, CANVAS_H, LV_COLOR_FORMAT_NATIVE);
    
    // Set position and center alignment
    lv_obj_align(canvas, LV_ALIGN_CENTER, xOffset, 0);
    lv_canvas_fill_bg(canvas, lv_color_hex(COLOR_BG), LV_OPA_COVER);
}

void PixelRobotEyes::drawPixelBlock(lv_obj_t* canvas, int32_t gridX, int32_t gridY, lv_color_t color) {
    if (gridX < 0 || gridX >= MATRIX_COLS || gridY < 0 || gridY >= MATRIX_ROWS) return;

    int32_t px = gridX * (PIXEL_SIZE + PIXEL_GAP);
    int32_t py = gridY * (PIXEL_SIZE + PIXEL_GAP);

    // Draw a blocky pixel square onto canvas
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 0; // Absolute sharp pixel corners

    lv_area_t area = { (int16_t)px, (int16_t)py, (int16_t)(px + PIXEL_SIZE - 1), (int16_t)(py + PIXEL_SIZE - 1) };
    lv_draw_rect(&layer, &dsc, &area);

    lv_canvas_finish_layer(canvas, &layer);
}

void PixelRobotEyes::renderEyeGrid(lv_obj_t* canvas, Emotion emotion, Direction dir, bool isLeft, bool isClosed) {
    lv_canvas_fill_bg(canvas, lv_color_hex(COLOR_BG), LV_OPA_COVER);
    if (isClosed) return; // Closed eye (blank/line)

    int32_t shiftX = 0;
    if (dir == Direction::Left)  shiftX = -2;
    if (dir == Direction::Right) shiftX = 2;

    lv_color_t eyeColor = lv_color_hex(COLOR_EYE_NEON);

    for (int32_t r = 0; r < MATRIX_ROWS; ++r) {
        for (int32_t c = 0; c < MATRIX_COLS; ++c) {
            bool draw = false;

            // Default rounded blocky rectangle shape
            if (r >= 2 && r <= 11 && c >= 1 && c <= 8) {
                // Cut off 4 corner pixels to create a pixelated rounded box
                if (!((r == 2 && (c == 1 || c == 8)) || (r == 11 && (c == 1 || c == 8)))) {
                    draw = true;
                }
            }

            // Emotion modifications (pixel masks)
            if (emotion == Emotion::Angry) {
                // Slant top pixels inward
                if (isLeft && (r - c < 1)) draw = false;
                if (!isLeft && (r - (MATRIX_COLS - 1 - c) < 1)) draw = false;
            } else if (emotion == Emotion::Happy) {
                // Cut bottom half for a pixel arch
                if (r > 7) draw = false;
            }

            if (draw) {
                drawPixelBlock(canvas, c + shiftX, r, eyeColor);
            }
        }
    }
}

void PixelRobotEyes::setEmotion(Emotion emotion) {
    m_currentEmotion = emotion;
    renderEyeGrid(m_leftCanvas, m_currentEmotion, m_currentDirection, true, false);
    renderEyeGrid(m_rightCanvas, m_currentEmotion, m_currentDirection, false, false);
}

void PixelRobotEyes::lookAt(Direction direction) {
    m_currentDirection = direction;
    renderEyeGrid(m_leftCanvas, m_currentEmotion, m_currentDirection, true, false);
    renderEyeGrid(m_rightCanvas, m_currentEmotion, m_currentDirection, false, false);
}

void PixelRobotEyes::blink(uint32_t duration_ms) {
    // Hide eye matrices for closed frame
    renderEyeGrid(m_leftCanvas, m_currentEmotion, m_currentDirection, true, true);
    renderEyeGrid(m_rightCanvas, m_currentEmotion, m_currentDirection, false, true);

    // Simple timer callback or delay restore
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_time(&a, duration_ms);
    lv_anim_set_exec_cb(&a, [](void* var, int32_t) {});
    lv_anim_set_ready_cb(&a, [](lv_anim_t* anim) {
        auto* eyes = static_cast<PixelRobotEyes*>(anim->var);
        eyes->setEmotion(eyes->m_currentEmotion);
    });
    lv_anim_start(&a);
}