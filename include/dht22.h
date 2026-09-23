#ifndef DHT22_H
#define DHT22_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * Read temperature and humidity from a DHT22 sensor.
 *
 * gpio_num    - GPIO connected to the DHT22 data pin
 * temperature - receives temperature in degrees Celsius
 * humidity    - receives relative humidity in percent
 */
esp_err_t dht22_read(gpio_num_t gpio_num,
                     float *temperature,
                     float *humidity);

#endif