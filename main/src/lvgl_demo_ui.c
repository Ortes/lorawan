/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "lvgl.h"

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    lv_theme_t * th = lv_theme_mono_init(disp, false, &lv_font_montserrat_10);
    lv_disp_set_theme(disp, th);

    lv_obj_t *scr = lv_disp_get_scr_act(disp);


    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "Bonjour, Monde!");
    lv_obj_set_width(label, disp->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);


    static lv_style_t style_indic;

    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_color_black());
    lv_style_set_radius(&style_indic, 3);

    lv_obj_t * bar = lv_bar_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_add_style(bar, &style_indic, LV_PART_INDICATOR);

    lv_obj_set_size(bar, 128, 4);
    lv_bar_set_value(bar, 67, LV_ANIM_ON);

    lv_obj_align_to(bar, label , LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);


    static lv_style_t style_arc;
    lv_style_init(&style_arc);
    lv_style_set_arc_color(&style_arc, lv_color_black());
    lv_style_set_arc_width(&style_arc, 6);
    lv_style_set_arc_rounded(&style_arc, true);
    lv_obj_t *spinner = lv_spinner_create(scr, 1500, 200);
    lv_obj_align_to(spinner, bar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(spinner, 46, 46);
    lv_obj_add_style(spinner, &style_arc, LV_PART_INDICATOR);
}
