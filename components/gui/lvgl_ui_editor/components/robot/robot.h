#ifndef _ROBOT_H_
#define _ROBOT_H_

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
    #include "lvgl_private.h"
#else
    #include "lvgl/lvgl.h"
    #include "lvgl/lvgl_private.h"
#endif

#ifdef LV_USE_XML
    #include "lv_xml/lv_xml.h"
#endif


#ifdef __cplusplus
extern "C" {
void create_robot_face_ui(void);
}
#endif

#endif /* _ROBOT_H_ */