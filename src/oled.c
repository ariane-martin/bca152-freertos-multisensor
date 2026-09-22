#include "oled.h"
#include "driver/i2c_master.h"
#include <string.h>

#define OLED_ADDR 0x3C
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t oled_handle;

static esp_err_t oled_command(uint8_t command)
{
    uint8_t data[2] = {0x00, command};

    return i2c_master_transmit(
        oled_handle,
        data,
        sizeof(data),
        100
    );
}

static esp_err_t oled_data(const uint8_t *data, size_t length)
{
    uint8_t buffer[129];

    if (length > 128)
        return ESP_ERR_INVALID_SIZE;

    buffer[0] = 0x40;
    memcpy(&buffer[1], data, length);

    return i2c_master_transmit(
        oled_handle,
        buffer,
        length + 1,
        100
    );
}

esp_err_t oled_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    ESP_ERROR_CHECK(
        i2c_new_master_bus(&bus_config, &bus_handle)
    );

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_ADDR,
        .scl_speed_hz = 400000
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            bus_handle,
            &dev_config,
            &oled_handle
        )
    );

    const uint8_t init_commands[] = {
        0xAE,
        0xD5, 0x80,
        0xA8, 0x3F,
        0xD3, 0x00,
        0x40,
        0x8D, 0x14,
        0x20, 0x02,
        0xA1,
        0xC8,
        0xDA, 0x12,
        0x81, 0xCF,
        0xD9, 0xF1,
        0xDB, 0x40,
        0xA4,
        0xA6,
        0xAF
    };

    for (size_t i = 0; i < sizeof(init_commands); i++)
    {
        ESP_ERROR_CHECK(oled_command(init_commands[i]));
    }

    oled_clear();

    return ESP_OK;
}

void oled_clear(void)
{
    uint8_t blank[128] = {0};

    for (int page = 0; page < 8; page++)
    {
        oled_set_cursor(page, 0);
        oled_data(blank, sizeof(blank));
    }
}

void oled_set_cursor(unsigned char page, unsigned char column)
{
    oled_command(0xB0 + page);
    oled_command(0x00 + (column & 0x0F));
    oled_command(0x10 + ((column >> 4) & 0x0F));
}

void oled_write_string(const char *text)
{
    static const uint8_t digits[10][5] = {
        {0x3E,0x51,0x49,0x45,0x3E}, // 0
        {0x00,0x42,0x7F,0x40,0x00}, // 1
        {0x42,0x61,0x51,0x49,0x46}, // 2
        {0x21,0x41,0x45,0x4B,0x31}, // 3
        {0x18,0x14,0x12,0x7F,0x10}, // 4
        {0x27,0x45,0x45,0x45,0x39}, // 5
        {0x3C,0x4A,0x49,0x49,0x30}, // 6
        {0x01,0x71,0x09,0x05,0x03}, // 7
        {0x36,0x49,0x49,0x49,0x36}, // 8
        {0x06,0x49,0x49,0x29,0x1E}  // 9
    };

    static const uint8_t uppercase[26][5] = {
        {0x7E,0x11,0x11,0x11,0x7E}, // A
        {0x7F,0x49,0x49,0x49,0x36}, // B
        {0x3E,0x41,0x41,0x41,0x22}, // C
        {0x7F,0x41,0x41,0x22,0x1C}, // D
        {0x7F,0x49,0x49,0x49,0x41}, // E
        {0x7F,0x09,0x09,0x09,0x01}, // F
        {0x3E,0x41,0x49,0x49,0x7A}, // G
        {0x7F,0x08,0x08,0x08,0x7F}, // H
        {0x00,0x41,0x7F,0x41,0x00}, // I
        {0x20,0x40,0x41,0x3F,0x01}, // J
        {0x7F,0x08,0x14,0x22,0x41}, // K
        {0x7F,0x40,0x40,0x40,0x40}, // L
        {0x7F,0x02,0x0C,0x02,0x7F}, // M
        {0x7F,0x04,0x08,0x10,0x7F}, // N
        {0x3E,0x41,0x41,0x41,0x3E}, // O
        {0x7F,0x09,0x09,0x09,0x06}, // P
        {0x3E,0x41,0x51,0x21,0x5E}, // Q
        {0x7F,0x09,0x19,0x29,0x46}, // R
        {0x46,0x49,0x49,0x49,0x31}, // S
        {0x01,0x01,0x7F,0x01,0x01}, // T
        {0x3F,0x40,0x40,0x40,0x3F}, // U
        {0x1F,0x20,0x40,0x20,0x1F}, // V
        {0x3F,0x40,0x38,0x40,0x3F}, // W
        {0x63,0x14,0x08,0x14,0x63}, // X
        {0x07,0x08,0x70,0x08,0x07}, // Y
        {0x61,0x51,0x49,0x45,0x43}  // Z
    };

    while (*text)
    {
        uint8_t character[6] = {0};

        char c = *text;

        /* Use uppercase font for lowercase letters too */
        if (c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 'A';
        }

        if (c >= 'A' && c <= 'Z')
        {
            memcpy(character, uppercase[c - 'A'], 5);
        }
        else if (c >= '0' && c <= '9')
        {
            memcpy(character, digits[c - '0'], 5);
        }
        else if (c == '.')
        {
            uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
            memcpy(character, dot, 5);
        }
        else if (c == ' ')
        {
            /* character remains blank */
        }

        character[5] = 0x00;
        oled_data(character, sizeof(character));

        text++;
        }
    }
    void oled_set_power(bool on)
    {
    oled_command(on ? 0xAF : 0xAE);
    }