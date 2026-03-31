#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>

#define SCREENSHOT_MODE 1
#define GAP 4

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

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    s_bounds = layer_get_bounds(window_layer);

    window_set_background_color(window, GColorBlack);

    s_canvas_layer = layer_create(s_bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);

    update_date();
}

static void main_window_unload(Window *window) {
    layer_destroy(s_canvas_layer);
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
}

static void deinit() {
    window_destroy(s_main_window);
    ffont_destroy(s_font_regular);
    ffont_destroy(s_font_leco);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
