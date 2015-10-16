#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_dow_layer;
static TextLayer *s_date_layer;
static Layer *s_line_layer, *s_time_layer;
static Layer *s_status_layer;

static struct tm curr_time;
static BatteryChargeState s_battery;
static bool s_connected;

static GBitmap *s_digits_bitmap;
static GBitmap *s_digit_bitmaps[11];

static GBitmap *s_status_bitmap;
static GBitmap *s_conn_bitmap;
static GBitmap *s_batt_bitmap;
static GBitmap *s_batt_charge_bitmap;

static void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void time_layer_update_callback(Layer *layer, GContext* ctx) {
  int h = curr_time.tm_hour;
  int m = curr_time.tm_min;
  
  if (!clock_is_24h_style()) {
    h = h % 12;
    h = h ? h : 12; // display 12 instead of 0
  }
  
  if (clock_is_24h_style() || (h / 10)) {
    graphics_draw_bitmap_in_rect(ctx, s_digit_bitmaps[h / 10], GRect(0, 0, 28, 40));
  }
  graphics_draw_bitmap_in_rect(ctx, s_digit_bitmaps[h % 10], GRect(28, 0, 28, 40));
  graphics_draw_bitmap_in_rect(ctx, s_digit_bitmaps[10], GRect(56, 0, 13, 40));
  graphics_draw_bitmap_in_rect(ctx, s_digit_bitmaps[m / 10], GRect(69, 0, 28, 40));
  graphics_draw_bitmap_in_rect(ctx, s_digit_bitmaps[m % 10], GRect(97, 0, 28, 40));
}

static void status_layer_update_callback(Layer *layer, GContext* ctx) {
  int batt_x_off = PBL_IF_RECT_ELSE(24, 12);
  if (!s_connected) {
    graphics_draw_bitmap_in_rect(ctx, s_conn_bitmap, GRect(0, 0, 24, 24));
#ifdef PBL_ROUND
    batt_x_off = 24;
#endif
  }
  if (s_battery.is_charging) {
    graphics_draw_bitmap_in_rect(ctx, s_batt_charge_bitmap, GRect(batt_x_off, 0, 24, 24));
  } else {
    uint16_t charge = s_battery.charge_percent;
    graphics_draw_bitmap_in_rect(ctx, s_batt_bitmap, GRect(batt_x_off, 0, 24, 24));
    graphics_context_set_fill_color(ctx, charge <= 20 ? GColorRed 
                                       : charge <= 30 ? GColorOrange
                                       : charge <= 40 ? GColorChromeYellow
                                       : charge <= 50 ? GColorYellow
                                       : GColorWhite);
    uint16_t batt_fill = 14 * charge / 100;
    graphics_fill_rect(ctx, GRect(batt_x_off + 9, 20, 6, -batt_fill), 0, GCornerNone);
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  static char s_date_text[] = "Xxxxxxxxx 00";
  static char s_dow_text[] = "Xxxxxxxxx 00";
  
  memcpy(&curr_time, tick_time, sizeof(struct tm));
  layer_mark_dirty(s_time_layer);

  strftime(s_date_text, sizeof(s_date_text), "%B %e", tick_time);
  text_layer_set_text(s_date_layer, s_date_text);

  strftime(s_dow_text, sizeof(s_dow_text), "%A", tick_time);
  text_layer_set_text(s_dow_layer, s_dow_text);
}

static void handle_connection_change(bool connected) {
  s_connected = connected;
  layer_mark_dirty(s_status_layer);
}

static void handle_battery_state_change(BatteryChargeState battery) {
  s_battery = battery;
  layer_mark_dirty(s_status_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  int x_off = PBL_IF_RECT_ELSE(0, 18);
  int y_off = PBL_IF_RECT_ELSE(0, 6);
  s_date_layer = text_layer_create(GRect(8 + x_off, 62 + y_off, 136, 100));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  s_dow_layer = text_layer_create(GRect(8 + x_off, 34 + y_off, 136, 100));
  text_layer_set_text_color(s_dow_layer, GColorWhite);
  text_layer_set_background_color(s_dow_layer, GColorClear);
  text_layer_set_font(s_dow_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  layer_add_child(window_layer, text_layer_get_layer(s_dow_layer));

  s_time_layer = layer_create(GRect(7 + x_off, 105 + y_off, 125, 40));
  layer_set_update_proc(s_time_layer, time_layer_update_callback);
  layer_add_child(window_layer, s_time_layer);

  int w_off = PBL_IF_RECT_ELSE(0, 15);
  s_line_layer = layer_create(GRect(8 + x_off, 97 + y_off, 139 + w_off, 2));
  layer_set_update_proc(s_line_layer, line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer);

  s_status_layer = layer_create(GRect(PBL_IF_RECT_ELSE(94, 66), 2, 48, 24));
  layer_set_update_proc(s_status_layer, status_layer_update_callback);
  layer_add_child(window_layer, s_status_layer);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_date_layer);

  layer_destroy(s_time_layer);
  layer_destroy(s_line_layer);
  layer_destroy(s_status_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  s_digits_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIGITS);
  for (int i = 0; i <= 9; i++) {
    s_digit_bitmaps[i] = gbitmap_create_as_sub_bitmap(s_digits_bitmap, GRect(28 * i, 0, 28, 40));
  }
  s_digit_bitmaps[10] = gbitmap_create_as_sub_bitmap(s_digits_bitmap, GRect(280, 0, 13, 40));

  s_status_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATUS);
  s_conn_bitmap = gbitmap_create_as_sub_bitmap(s_status_bitmap, GRect(0, 0, 24, 24));
  s_batt_bitmap = gbitmap_create_as_sub_bitmap(s_status_bitmap, GRect(24, 0, 24, 24));
  s_batt_charge_bitmap = gbitmap_create_as_sub_bitmap(s_status_bitmap, GRect(48, 0, 24, 24));
  
  s_connected = bluetooth_connection_service_peek();
  bluetooth_connection_service_subscribe(handle_connection_change);
  s_battery = battery_state_service_peek();
  battery_state_service_subscribe(handle_battery_state_change);
  
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);  
  // Prevent starting blank
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_minute_tick(t, MINUTE_UNIT);
}

static void deinit() {
  window_destroy(s_main_window);
  
  for (int i = 0; i <= 10; i++) {
    gbitmap_destroy(s_digit_bitmaps[i]);
  }
  gbitmap_destroy(s_digits_bitmap);

  gbitmap_destroy(s_conn_bitmap);
  gbitmap_destroy(s_batt_bitmap);
  gbitmap_destroy(s_batt_charge_bitmap);
  gbitmap_destroy(s_status_bitmap);
  
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
}

int main() {
  init();
  app_event_loop();
  deinit();
}