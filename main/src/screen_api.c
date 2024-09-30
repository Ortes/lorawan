#include "screen_api.h"

static lv_obj_t *main_label;
static lv_obj_t *spinner;

static char events[3][20] = {"", "", ""};
static lv_obj_t *event_labels[3] = {};
static int event_index = 0;

void screen_has_joined()
{
    lv_label_set_text(main_label, "Joined!");
    lv_obj_del(spinner);
}

void screen_add_event(const char *event)
{
    strncpy(events[event_index], event, 20);
    events[event_index][19] = '\0';

    for (int i = 0; i < 3; i++) {
        lv_label_set_text(event_labels[i], events[(event_index - i + 3) % 3]);
    }
    if (++event_index >= 3) {
        event_index = 0;
    }
}

void screen_initial_setup(lv_disp_t *disp)
{
    lv_theme_t * th = lv_theme_mono_init(disp, false, &lv_font_montserrat_10);
    lv_disp_set_theme(disp, th);

    lv_obj_t *scr = lv_disp_get_scr_act(disp);

    lv_obj_t *col = lv_obj_create(scr);
    lv_obj_set_size(col, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *row = lv_obj_create(col);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);

    main_label = lv_label_create(row);
    lv_label_set_long_mode(main_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(main_label, "Joining...");

    static lv_style_t style_arc;
    lv_style_init(&style_arc);
    lv_style_set_arc_color(&style_arc, lv_color_black());
    lv_style_set_arc_width(&style_arc, 2);
    lv_style_set_arc_rounded(&style_arc, true);

    spinner = lv_spinner_create(row, 1500, 200);
    lv_obj_align_to(spinner, scr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(spinner, 12, 12);
    lv_obj_add_style(spinner, &style_arc, LV_PART_INDICATOR);

    lv_obj_t *event_col = lv_obj_create(col);
    lv_obj_remove_style_all(event_col);
    lv_obj_set_size(event_col, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_grow(event_col, 1);
    lv_obj_set_flex_flow(event_col, LV_FLEX_FLOW_COLUMN);

    for (int i = 0; i < 3; i++) {
        event_labels[i] = lv_label_create(event_col);
        lv_obj_set_size(event_labels[i], LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_label_set_text(event_labels[i], events[i]);
    }
}
