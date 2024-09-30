#ifndef SCREEN_API_H
#define SCREEN_API_H

#include "lvgl.h"

void screen_has_joined();

void screen_add_event(const char *event);

void screen_initial_setup(lv_disp_t *disp);

#endif // SCREEN_API_H