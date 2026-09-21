#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht22.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include <stdbool.h>
#include "freertos/queue.h"

typedef struct
{
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
} SensorData;

/* ADC handle used by SensorTask */
static adc_oneshot_unit_handle_t adc_handle;
static QueueHandle_t sensor_queue;

/* Temporary foundation task */
void task_a(void *pvParameters)
{
    while (1)
    {
        printf("Task A running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Temporary foundation task */
void task_b(void *pvParameters)
{
    while (1)
    {
        printf("Task B running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Sensor acquisition task */
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

        /* Convert raw ADC reading to relative 0-100% */
        int light_level = (raw_light * 100) / 4095;

        /* Read DHT22 */
        esp_err_t result =
            dht22_read(GPIO_NUM_15, &temperature, &humidity);

        printf("\n--- SensorTask ---\n");

        printf("LDR Raw: %d\n", raw_light);
        printf("Light Level: %d %%\n", light_level);

        if (result == ESP_OK)
        {
            printf("Temperature: %.2f C\n", temperature);
            printf("Humidity: %.2f %%\n", humidity);
        }
        else
        {
            printf(
                "DHT22 read failed: %s\n",
                esp_err_to_name(result)
            );
        }

        /*
         * Run SensorTask every 2 seconds.
         * vTaskDelayUntil keeps a stable periodic schedule.
         */

         SensorData data;

data.temperature = temperature;
data.humidity = humidity;
data.lightLevel = light_level;
data.motionDetected = false;   // PIR will be added later

if (xQueueSend(sensor_queue, &data, pdMS_TO_TICKS(100)) == pdPASS)
{
    printf("Sensor data sent to queue\n");
}
else
{
    printf("Sensor queue full\n");
}

        vTaskDelayUntil(
            &lastWakeTime,
            pdMS_TO_TICKS(2000)
        );
    }
}

void display_task(void *pvParameters)
{
    SensorData receivedData;

    while (1)
    {
        if (xQueueReceive(sensor_queue, &receivedData, portMAX_DELAY) == pdPASS)
        {
            printf("\n--- DisplayTask received ---\n");
            printf("Temperature: %.2f C\n", receivedData.temperature);
            printf("Humidity: %.2f %%\n", receivedData.humidity);
            printf("Light Level: %d %%\n", receivedData.lightLevel);
        }
    }
}

void app_main(void)
{
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");

    sensor_queue = xQueueCreate(5, sizeof(SensorData));

    if (sensor_queue == NULL)
    {
    printf("Failed to create sensor queue\n");
    return;
    }

    /* Configure ADC1 for the LDR */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &init_config,
            &adc_handle
        )
    );

    /* GPIO34 = ADC1 Channel 6 */
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            ADC_CHANNEL_6,
            &channel_config
        )
    );

    /* Create SensorTask */
    xTaskCreate(
        sensor_task,
        "SensorTask",
        4096,
        NULL,
        2,
        NULL
    );
    
    xTaskCreate(
    display_task,
    "DisplayTask",
    2048,
    NULL,
    1,
    NULL
);

    /* Foundation tasks */
    xTaskCreate(
        task_a,
        "TaskA",
        2048,
        NULL,
        1,
        NULL
    );

    xTaskCreate(
        task_b,
        "TaskB",
        2048,
        NULL,
        1,
        NULL
    );
}