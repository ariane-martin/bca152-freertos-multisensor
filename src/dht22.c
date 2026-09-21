#include "dht22.h"

#include <stdint.h>
#include "esp_rom_sys.h"
#include "driver/gpio.h"

/*
 * Wait while the GPIO remains at the requested level.
 * Returns how long the signal stayed at that level.
 */
static int wait_for_level(gpio_num_t pin, int level, int timeout_us)
{
    int elapsed = 0;

    while (gpio_get_level(pin) == level)
    {
        if (elapsed >= timeout_us)
        {
            return -1;
        }

        esp_rom_delay_us(1);
        elapsed++;
    }

    return elapsed;
}

esp_err_t dht22_read(gpio_num_t gpio_num,
                     float *temperature,
                     float *humidity)
{
    uint8_t data[5] = {0};

    /* Send start signal */
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_num, 0);
    esp_rom_delay_us(2000);

    /* Release data line */
    gpio_set_level(gpio_num, 1);
    esp_rom_delay_us(30);

    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);

    /* DHT22 response: LOW -> HIGH -> LOW */
    if (wait_for_level(gpio_num, 1, 200) < 0)
        return ESP_ERR_TIMEOUT;

    if (wait_for_level(gpio_num, 0, 200) < 0)
        return ESP_ERR_TIMEOUT;

    if (wait_for_level(gpio_num, 1, 200) < 0)
        return ESP_ERR_TIMEOUT;

    /* Read 40 bits */
    for (int i = 0; i < 40; i++)
    {
        /* Wait for LOW part of bit to finish */
        if (wait_for_level(gpio_num, 0, 100) < 0)
            return ESP_ERR_TIMEOUT;

        /*
         * Instead of measuring the whole HIGH pulse,
         * sample after ~40 us:
         *
         * LOW  = bit 0
         * HIGH = bit 1
         */
        esp_rom_delay_us(40);

        data[i / 8] <<= 1;

        if (gpio_get_level(gpio_num))
        {
            data[i / 8] |= 1;
        }

        /* Wait for HIGH pulse to finish */
        if (wait_for_level(gpio_num, 1, 100) < 0)
            return ESP_ERR_TIMEOUT;
    }

    /* Verify checksum */
    uint8_t checksum =
        (uint8_t)(data[0] + data[1] + data[2] + data[3]);

    if (checksum != data[4])
    {
        return ESP_ERR_INVALID_CRC;
    }

    /* Convert humidity */
    uint16_t raw_humidity =
        ((uint16_t)data[0] << 8) | data[1];

    *humidity = raw_humidity / 10.0f;

    /* Convert temperature */
    uint16_t raw_temperature =
        ((uint16_t)(data[2] & 0x7F) << 8) | data[3];

    *temperature = raw_temperature / 10.0f;

    if (data[2] & 0x80)
    {
        *temperature = -*temperature;
    }

    return ESP_OK;
}