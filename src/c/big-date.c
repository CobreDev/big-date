#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>

#define SCREENSHOT_MODE 0
#define GAP 4

#define SETTINGS_PERSIST_KEY 1

typedef struct {
    int quit_timeout; // seconds until auto-quit; 0 = never
} Settings;

static Settings s_settings;
static AppTimer *s_quit_timer;
static bool s_quick_launched; // true if the app was opened via quick launch

static Window *s_main_window;
static Layer *s_canvas_layer;
static FFont *s_font_regular;
static FFont *s_font_leco;

static int s_text_h;
static int s_num_h;
static int s_start_y;
static int s_y_offset;
static GRect s_bounds;

static char s_day_buffer[18];
static char s_month_buffer[16];
static char s_date_num_buffer[4];

static void update_date() {
#if SCREENSHOT_MODE
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Sunday,");
    snprintf(s_month_buffer, sizeof(s_month_buffer), "July");
    snprintf(s_date_num_buffer, sizeof(s_date_num_buffer), "17");
#else
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    strftime(s_day_buffer, sizeof(s_day_buffer) - 2, "%A", tick_time);
    snprintf(s_day_buffer + strlen(s_day_buffer), 2, ",");
    strftime(s_month_buffer, sizeof(s_month_buffer), "%B", tick_time);
    strftime(s_date_num_buffer, sizeof(s_date_num_buffer), "%d", tick_time);
#endif
    layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_date();
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
#ifdef PBL_COLOR
    fctx_enable_aa(true);
#endif

    FPoint pos;

    FContext fctx;
    fctx_init_context(&fctx, ctx);
    fctx_set_fill_color(&fctx, GColorWhite);
    fctx_set_color_bias(&fctx, 0);

    fctx_begin_fill(&fctx);

    fctx_set_text_em_height(&fctx, s_font_regular, s_text_h);

    pos = FPointI(s_bounds.size.w / 2, s_start_y + s_text_h / 2 + s_y_offset);
    fctx_set_offset(&fctx, pos);
    fctx_draw_string(&fctx, s_day_buffer, s_font_regular, GTextAlignmentCenter, FTextAnchorMiddle);

    pos = FPointI(s_bounds.size.w / 2, s_start_y + s_text_h + GAP + s_text_h / 2 + s_y_offset);
    fctx_set_offset(&fctx, pos);
    fctx_draw_string(&fctx, s_month_buffer, s_font_regular, GTextAlignmentCenter, FTextAnchorMiddle);

    fctx_set_text_em_height(&fctx, s_font_leco, s_num_h);

    pos = FPointI(s_bounds.size.w / 2, s_start_y + s_text_h + GAP + s_text_h + GAP + s_num_h / 2 + s_y_offset);
    fctx_set_offset(&fctx, pos);
    fctx_draw_string(&fctx, s_date_num_buffer, s_font_leco, GTextAlignmentCenter, FTextAnchorMiddle);

    fctx_end_fill(&fctx);

    fctx_deinit_context(&fctx);
}

static void quit_timer_callback(void *data) {
    s_quit_timer = NULL;
    window_stack_pop_all(false);
}

static void schedule_quit() {
    if (s_quit_timer) {
        app_timer_cancel(s_quit_timer);
        s_quit_timer = NULL;
    }
    // Only auto-quit when the app was opened via quick launch.
    if (s_quick_launched && s_settings.quit_timeout > 0) {
        s_quit_timer = app_timer_register(s_settings.quit_timeout * 1000, quit_timer_callback, NULL);
    }
}

static void load_settings() {
    if (persist_exists(SETTINGS_PERSIST_KEY)) {
        persist_read_data(SETTINGS_PERSIST_KEY, &s_settings, sizeof(s_settings));
    }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *quit_tuple = dict_find(iter, MESSAGE_KEY_QuitTimeout);
    if (!quit_tuple) {
        return;
    }

    // Clay sends the select's value; our options use string values, but read
    // defensively so a numeric value of any width is also handled correctly.
    switch (quit_tuple->type) {
        case TUPLE_CSTRING:
            s_settings.quit_timeout = atoi(quit_tuple->value->cstring);
            break;
        case TUPLE_INT:
            if (quit_tuple->length == 1) {
                s_settings.quit_timeout = quit_tuple->value->int8;
            } else if (quit_tuple->length == 2) {
                s_settings.quit_timeout = quit_tuple->value->int16;
            } else {
                s_settings.quit_timeout = quit_tuple->value->int32;
            }
            break;
        case TUPLE_UINT:
            if (quit_tuple->length == 1) {
                s_settings.quit_timeout = quit_tuple->value->uint8;
            } else if (quit_tuple->length == 2) {
                s_settings.quit_timeout = quit_tuple->value->uint16;
            } else {
                s_settings.quit_timeout = quit_tuple->value->uint32;
            }
            break;
        default:
            return; // unexpected type; leave the current setting untouched
    }

    persist_write_data(SETTINGS_PERSIST_KEY, &s_settings, sizeof(s_settings));
    schedule_quit();
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    s_bounds = layer_get_bounds(window_layer);

    window_set_background_color(window, GColorBlack);

    s_canvas_layer = layer_create(s_bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);

    update_date();
}

// AppGlance is available on every target platform except aplite.
#ifndef PBL_PLATFORM_APLITE
static void glance_reload_callback(AppGlanceReloadSession *session, size_t limit, void *context) {
    if (limit < 1) {
        return;
    }

    // Custom icon for each day of the month (1..31).
    static const uint32_t date_icons[31] = {
        PUBLISHED_ID_DATE_01, PUBLISHED_ID_DATE_02, PUBLISHED_ID_DATE_03, PUBLISHED_ID_DATE_04,
        PUBLISHED_ID_DATE_05, PUBLISHED_ID_DATE_06, PUBLISHED_ID_DATE_07, PUBLISHED_ID_DATE_08,
        PUBLISHED_ID_DATE_09, PUBLISHED_ID_DATE_10, PUBLISHED_ID_DATE_11, PUBLISHED_ID_DATE_12,
        PUBLISHED_ID_DATE_13, PUBLISHED_ID_DATE_14, PUBLISHED_ID_DATE_15, PUBLISHED_ID_DATE_16,
        PUBLISHED_ID_DATE_17, PUBLISHED_ID_DATE_18, PUBLISHED_ID_DATE_19, PUBLISHED_ID_DATE_20,
        PUBLISHED_ID_DATE_21, PUBLISHED_ID_DATE_22, PUBLISHED_ID_DATE_23, PUBLISHED_ID_DATE_24,
        PUBLISHED_ID_DATE_25, PUBLISHED_ID_DATE_26, PUBLISHED_ID_DATE_27, PUBLISHED_ID_DATE_28,
        PUBLISHED_ID_DATE_29, PUBLISHED_ID_DATE_30, PUBLISHED_ID_DATE_31,
    };

    // Larger-display platforms have room for the full month name; the smaller
    // ones (basalt/chalk/diorite) use the abbreviated month. (aplite has no
    // AppGlance and is excluded above.)
#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_DIORITE)
    const char *format = "%A, %b %d";
#else
    const char *format = "%A, %B %d";
#endif

    // Start of the current local day (midnight).
    time_t now = time(NULL);
    struct tm midnight = *localtime(&now);
    midnight.tm_hour = 0;
    midnight.tm_min = 0;
    midnight.tm_sec = 0;
    time_t start_of_today = mktime(&midnight);

    // Queue one slice per upcoming day, each expiring at the following midnight,
    // so the glance advances by itself (no app launch needed) for as many days
    // as the system allows (`limit`). The calendar day is derived from the real
    // date each iteration, so month lengths, February, and leap years are all
    // handled automatically. 24h steps mean the expiry can drift by an hour
    // across a DST change, which is harmless for a date display.
    int added = 0;
    for (size_t i = 0; i < limit && i < 31; i++) {
        time_t day_noon = start_of_today + (time_t)i * 86400 + 43200; // midday: DST-safe
        struct tm day_tm = *localtime(&day_noon);

        char subtitle[32];
        strftime(subtitle, sizeof(subtitle), format, &day_tm);

        uint32_t icon = APP_GLANCE_SLICE_DEFAULT_ICON;
        int mday = day_tm.tm_mday;
        if (mday >= 1 && mday <= 31) {
            icon = date_icons[mday - 1];
        }

        AppGlanceSlice slice = {
            .layout = {
                .icon = icon,
                .subtitle_template_string = subtitle,
            },
            .expiration_time = start_of_today + (time_t)(i + 1) * 86400,
        };
        if (app_glance_add_slice(session, slice) != APP_GLANCE_RESULT_SUCCESS) {
            break;
        }
        added++;
    }
    APP_LOG(APP_LOG_LEVEL_INFO, "AppGlance: queued %d day(s), limit=%d", added, (int)limit);
}
#endif

static void main_window_unload(Window *window) {
    layer_destroy(s_canvas_layer);

#ifndef PBL_PLATFORM_APLITE
    // Update the app's glance in the launcher so it shows the current date.
    app_glance_reload(glance_reload_callback, NULL);
#endif
}

static void init() {
    s_font_regular = ffont_create_from_resource(RESOURCE_ID_FONT_OSWALD_REGULAR);
    s_font_leco = ffont_create_from_resource(RESOURCE_ID_FONT_LECO);

    s_text_h = 32;
    s_num_h = 70;
    s_y_offset = 2;

#ifdef PBL_PLATFORM_APLITE
    s_text_h = 28;
    s_num_h = 48;
#elif defined(PBL_PLATFORM_BASALT)
    s_text_h = 28;
    s_num_h = 48;
#elif defined(PBL_PLATFORM_CHALK)
    s_text_h = 28;
    s_num_h = 48;
#elif defined(PBL_PLATFORM_DIORITE)
    s_text_h = 28;
    s_num_h = 48;
#elif defined(PBL_PLATFORM_EMERY)
    s_text_h = 38;
    s_num_h = 96;
    s_y_offset = 5;
#elif defined(PBL_PLATFORM_FLINT)
    s_text_h = 28;
    s_num_h = 48;
#elif defined(PBL_PLATFORM_GABBRO)
    s_text_h = 38;
    s_num_h = 96;
    s_y_offset = 22;
#endif

    int total_h = s_text_h + s_text_h + s_num_h;
    int content_h = total_h + (GAP * 2);

    int screen_h = 168;
#ifdef PBL_PLATFORM_EMERY
    screen_h = 228;
#elif defined(PBL_PLATFORM_GABBRO)
    screen_h = 228;
#elif defined(PBL_PLATFORM_CHALK)
    screen_h = 180;
#endif

    s_start_y = (screen_h - content_h) / 2;

    s_main_window = window_create();

    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });

    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(DAY_UNIT, tick_handler);

    s_quick_launched = (launch_reason() == APP_LAUNCH_QUICK_LAUNCH);

    load_settings();

    app_message_register_inbox_received(inbox_received_handler);
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

    schedule_quit();
}

static void deinit() {
    if (s_quit_timer) {
        app_timer_cancel(s_quit_timer);
        s_quit_timer = NULL;
    }
    window_destroy(s_main_window);
    ffont_destroy(s_font_regular);
    ffont_destroy(s_font_leco);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
