#pragma once
#include <lvgl.h>

void fs_mount_once();
void fs_set_tool_gif(lv_obj_t* gif_obj, int tool_number);
lv_obj_t* fs_load_tool_gif(lv_obj_t* parent, int tool_number);
int detect_my_tool_number();