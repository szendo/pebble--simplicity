#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef void(* ConfigChanged)();

void config_open(ConfigChanged callback);
void config_close();

bool config_get_day_of_week_enabled();
