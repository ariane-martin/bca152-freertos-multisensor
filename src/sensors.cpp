#include <stdio.h>

#include "sensors.h"

extern "C" {
#include "dht22.h"
}

#include "rtos_objects.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

static adc_oneshot_unit_handle_t adc_handle;
static float latest_temperature = 0.0f;

void sensors_init(void)
{
    /* Configure ADC1 for the LDR */
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT_1;

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &init_config,
            &adc_handle
        )
    );

    /* GPIO34 = ADC1 Channel 6 */
    adc_oneshot_chan_cfg_t channel_config = {};
    channel_config.atten = ADC_ATTEN_DB_12;
    channel_config.bitwidth = ADC_BITWIDTH_DEFAULT;

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            ADC_CHANNEL_6,
            &channel_config
        )
    );
}

float sensors_get_latest_temperature(void)
{
    return latest_temperature;
}

void sensor_task(void *pvParameters)
{
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (1)
    {
        float temperature = 0.0f;
        float humidity = 0.0f;
        int raw_light = 0;

        /* Read LDR */
        adc_oneshot_read(
            adc_handle,
            ADC_CHANNEL_6,
            &raw_light
        );

        /* Convert ADC reading to relative 0-100% light level */
        int light_level = 100 - ((raw_light * 100) / 4095);

        /* Read DHT22 */
        esp_err_t result =
            dht22_read(
                GPIO_NUM_15,
                &temperature,
                &humidity
            );

        xSemaphoreTake(serialMutex, portMAX_DELAY);

        printf("\n--- SensorTask ---\n");
        printf("LDR Raw: %d\n", raw_light);
        printf("Light Level: %d %%\n", light_level);

        if (result == ESP_OK)
        {
            printf("Temperature: %.2f C\n", temperature);
            printf("Humidity: %.2f %%\n", humidity);

            latest_temperature = temperature;
        }
        else
        {
            printf(
                "DHT22 read failed: %s\n",
                esp_err_to_name(result)
            );
        }

        xSemaphoreGive(serialMutex);

        SensorData data;

        data.temperature = temperature;
        data.humidity = humidity;
        data.lightLevel = light_level;

        EventBits_t event_bits = xEventGroupGetBits(system_events);
        data.motionDetected = (event_bits & EVENT_MOTION) != 0;

        if (xQueueSend(
                sensor_queue,
                &data,
                pdMS_TO_TICKS(100)) == pdPASS)
        {
            xSemaphoreTake(serialMutex, portMAX_DELAY);
            printf("Sensor data sent to queue\n");
            xSemaphoreGive(serialMutex);
        }
        else
        {
            xSemaphoreTake(serialMutex, portMAX_DELAY);
            printf("Sensor queue full\n");
            xSemaphoreGive(serialMutex);
        }

        vTaskDelayUntil(
            &lastWakeTime,
            pdMS_TO_TICKS(2000)
        );
    }
}