#include <pebble.h>

// Screenshot mode: set to 1 to show fixed date, 0 for real date
#define SCREENSHOT_MODE 1

static Window *s_main_window;
static TextLayer *s_day_layer;
static TextLayer *s_month_layer;
static TextLayer *s_date_num_layer;
static GFont s_num_font;
static bool s_num_font_is_custom;

static void update_date() {
    static char day_buffer[18];
    static char month_buffer[16];
    static char date_num_buffer[4];

#if SCREENSHOT_MODE
    snprintf(day_buffer, sizeof(day_buffer), "Sunday,");
    snprintf(month_buffer, sizeof(month_buffer), "July");
    snprintf(date_num_buffer, sizeof(date_num_buffer), "17");
#else
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    strftime(day_buffer, sizeof(day_buffer) - 2, "%A", tick_time);
    snprintf(day_buffer + strlen(day_buffer), 2, ",");
    strftime(month_buffer, sizeof(month_buffer), "%B", tick_time);
    strftime(date_num_buffer, sizeof(date_num_buffer), "%d", tick_time);
#endif

    text_layer_set_text(s_day_layer, day_buffer);
    text_layer_set_text(s_month_layer, month_buffer);
    text_layer_set_text(s_date_num_layer, date_num_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_date();
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    window_set_background_color(window, GColorBlack);

    int h = bounds.size.h;

GFont text_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    s_num_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
    s_num_font_is_custom = false;

    int text_h = 32;
    int num_h = 50;

#ifdef PBL_PLATFORM_EMERY
    text_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    s_num_font = fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM);
    text_h = 32;
    num_h = 70;
#elif defined(PBL_PLATFORM_GABBRO)
    text_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    s_num_font = fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM);
    text_h = 32;
    num_h = 70;
#endif

    int total_h = text_h + text_h + num_h;
    int gap = 0;
    int content_h = total_h + (gap * 2);
    int start_y = (h - content_h) / 2;

    int y1 = start_y;
    int y2 = y1 + text_h + gap;
    int y3 = y2 + text_h + gap;

    s_day_layer = text_layer_create(GRect(0, y1, bounds.size.w, text_h));
    text_layer_set_background_color(s_day_layer, GColorClear);
    text_layer_set_text_color(s_day_layer, GColorWhite);
    text_layer_set_font(s_day_layer, text_font);
    text_layer_set_text_alignment(s_day_layer, GTextAlignmentCenter);

    s_month_layer = text_layer_create(GRect(0, y2, bounds.size.w, text_h));
    text_layer_set_background_color(s_month_layer, GColorClear);
    text_layer_set_text_color(s_month_layer, GColorWhite);
    text_layer_set_font(s_month_layer, text_font);
    text_layer_set_text_alignment(s_month_layer, GTextAlignmentCenter);

    s_date_num_layer = text_layer_create(GRect(0, y3, bounds.size.w, num_h));
    text_layer_set_background_color(s_date_num_layer, GColorClear);
    text_layer_set_text_color(s_date_num_layer, GColorWhite);
    text_layer_set_font(s_date_num_layer, s_num_font);
    text_layer_set_text_alignment(s_date_num_layer, GTextAlignmentCenter);

    layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_month_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_date_num_layer));

    update_date();
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_day_layer);
    text_layer_destroy(s_month_layer);
    text_layer_destroy(s_date_num_layer);
    if (s_num_font_is_custom) {
        fonts_unload_custom_font(s_num_font);
    }
}

static void init() {
    s_main_window = window_create();

    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });

    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(DAY_UNIT, tick_handler);
}

static void deinit() {
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
