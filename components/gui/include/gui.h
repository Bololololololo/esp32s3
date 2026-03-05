#ifndef GUI_H
#define GUI_H

#include <iostream>
#include <memory>

#include "lvgl.h"
#include "system_info.h"

using namespace utils;

class Gui
{
private:
    static std::unique_ptr<Gui> instance;
    struct _cons
    {
        explicit _cons() = default;
    };

    const lv_font_t *font_normal;
    const lv_font_t *font_large;
    lv_style_t style_text_muted;
    lv_style_t style_title;
    lv_style_t style_icon;
    lv_style_t style_bullet;
    lv_obj_t *tv;

    SystemInfo sys_info;

    void mainTabCreate(lv_obj_t *parent);
    void settingsTabCreate(lv_obj_t *parent);
    void debugTabCreate(lv_obj_t *parent);
    static void event_handler(lv_event_t *e);

public:
    Gui(_cons) {}

    static std::unique_ptr<Gui> instanceFactory()
    {
        return std::make_unique<Gui>(_cons{});
    }

    static Gui *getInstance()
    {
        /*
            If control enters the declaration concurrently while the variable is being initialized,
            the concurrent execution shall wait for completion of the initialization.
        */
        static std::unique_ptr<Gui> instance{Gui::instanceFactory()};
        return instance.get();
    }

    void init();
};

#endif /* GUI_H */