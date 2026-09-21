#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht22.h"
#include "driver/gpio.h"

void task_a(void *pvParameters)
{
    while (1)
    {
        printf("Task A running\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_b(void *pvParameters)
{
    while (1)
    {
        printf("Task B running\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");

    float temperature = 0.0f;
float humidity = 0.0f;

esp_err_t result = dht22_read(GPIO_NUM_15, &temperature, &humidity);

if (result == ESP_OK)
{
    printf("Temperature: %.2f C\n", temperature);
    printf("Humidity: %.2f %%\n", humidity);
}
else
{
    printf("DHT22 read failed: %s\n", esp_err_to_name(result));
}

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