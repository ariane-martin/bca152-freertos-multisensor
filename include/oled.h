#ifndef OLED_H
#define OLED_H

#include <stdbool.h>

#include "esp_err.h"

esp_err_t oled_init(void);
void oled_clear(void);
void oled_set_cursor(unsigned char page, unsigned char column);
void oled_write_string(const char *text);
void oled_set_power(bool on);

#endif