#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "dht22.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include <stdbool.h>
#include "freertos/queue.h"
#include "oled.h"
#include "alarm.h"
#include "rtos_objects.h"
#include "system_state.h"
#include "sensors.h"
#include "motion.h"
#include "input.h"  
#include "display.h"
#include "alarm.h"

/* Temporary foundation task */
void task_a(void *pvParameters) 
{
    while (1)
    {
        xSemaphoreTake(serialMutex, portMAX_DELAY);
        printf("Task A running\n");
        xSemaphoreGive(serialMutex);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Temporary foundation task */
void task_b(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(serialMutex, portMAX_DELAY);
        printf("Task B running\n");
        xSemaphoreGive(serialMutex);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");

    sensor_queue = xQueueCreate(5, sizeof(SensorData));

    system_events = xEventGroupCreate();

    if (system_events == NULL)
    {
    printf("Failed to create event group\n");
    return;
    }

    /* System starts ACTIVE */
    system_state_init();

    /* Create Serial mutex */
    serialMutex = xSemaphoreCreateMutex();

    if (serialMutex == NULL)
    {
    printf("Failed to create serial mutex\n");
    return;
    }

    if (sensor_queue == NULL)
    {
    printf("Failed to create sensor queue\n");
    return;
    }

    sensors_init();

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

    xTaskCreate(
    input_task,
    "InputTask",
    2048,
    NULL,
    3,
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

    xTaskCreate(
        motion_task,
        "MotionTask",
        2048,
        NULL,
        3,
        NULL
    );

    xTaskCreate(
    alarm_task,
    "AlarmTask",
    2048,
    NULL,
    2,
    NULL
    );
}