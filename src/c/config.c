#include <pebble.h>
#include "config.h"

#define DAY_OF_WEEK_ENABLED 1

static bool s_day_of_week_enabled;

static ConfigChanged change_callback;

static void app_message_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *next_tuple = dict_read_first(iterator);
  while (NULL != next_tuple) {
    switch (next_tuple->key) {
      case DAY_OF_WEEK_ENABLED:
        persist_write_int(DAY_OF_WEEK_ENABLED, s_day_of_week_enabled = next_tuple->value->uint8);
        break;
    }
    next_tuple = dict_read_next(iterator);
  }
  if (change_callback != NULL) change_callback();
}

static void read_config_v0() {
  s_day_of_week_enabled = persist_read_int(DAY_OF_WEEK_ENABLED);
}

void config_open(ConfigChanged callback) {
  read_config_v0();
  change_callback = callback;
  app_message_register_inbox_received(app_message_inbox_received);
  app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

void config_close() {
  app_message_deregister_callbacks();
}

bool config_get_day_of_week_enabled() {
  return s_day_of_week_enabled;
}
