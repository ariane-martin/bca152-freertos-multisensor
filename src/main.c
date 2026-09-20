#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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