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

typedef enum
{
    DISPLAY_TEMPERATURE,
    DISPLAY_HUMIDITY,
    DISPLAY_LIGHT,
    DISPLAY_MOTION
} DisplayMode;

static DisplayMode current_display_mode = DISPLAY_TEMPERATURE;

#define ENCODER_CLK GPIO_NUM_32
#define ENCODER_DT  GPIO_NUM_33
#define ENCODER_SW  GPIO_NUM_25
#define BUZZER_PIN GPIO_NUM_26

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

void alarm_task(void *pvParameters)
{
    gpio_config_t buzzer_config = {
        .pin_bit_mask = (1ULL << BUZZER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&buzzer_config);
    gpio_set_level(BUZZER_PIN, 0);

    AlarmState previous_alarm = ALARM_NORMAL;

    while (1)
    {
        if (system_is_active())
        {
            AlarmState alarm =
                evaluateTemperature(sensors_get_latest_temperature());

            if (alarm != previous_alarm)
            {
                if (alarm == ALARM_LOW_TEMPERATURE)
                {
                    xSemaphoreTake(serialMutex, portMAX_DELAY);
                    printf("ALARM: TEMPERATURE TOO LOW\n");
                    xSemaphoreGive(serialMutex);
                }
                else if (alarm == ALARM_HIGH_TEMPERATURE)
                {
                    xSemaphoreTake(serialMutex, portMAX_DELAY);
                    printf("ALARM: TEMPERATURE TOO HIGH\n");
                    xSemaphoreGive(serialMutex);
                }
                else
                {
                    xSemaphoreTake(serialMutex, portMAX_DELAY);
                    printf("ALARM: TEMPERATURE NORMAL\n");
                    xSemaphoreGive(serialMutex);
                }

                previous_alarm = alarm;
            }

            if (alarm != ALARM_NORMAL)
            {
            xEventGroupSetBits(system_events, EVENT_ALARM);
            }
            else
            {
                xEventGroupClearBits(system_events, EVENT_ALARM);
            }

            gpio_set_level(
                BUZZER_PIN,
                alarm != ALARM_NORMAL
            );
        }
        else
        {
            xEventGroupClearBits(system_events, EVENT_ALARM);
            gpio_set_level(BUZZER_PIN, 0);
            previous_alarm = ALARM_NORMAL;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void display_task(void *pvParameters)
{
    xSemaphoreTake(serialMutex, portMAX_DELAY);
    printf("DisplayTask started\n");
    xSemaphoreGive(serialMutex);

    oled_init();
    oled_clear();

    SensorData receivedData;

    bool oled_is_on = true;

    while (1)
    {
        if (xQueueReceive(
                sensor_queue,
                &receivedData,
                portMAX_DELAY) == pdPASS)
        {

        if (!system_is_active())
    {
        if (oled_is_on)
    {
        oled_clear();
        oled_set_power(false);
        oled_is_on = false;
        xSemaphoreTake(serialMutex, portMAX_DELAY);
        printf("OLED OFF - SYSTEM INACTIVE\n");
        xSemaphoreGive(serialMutex);
    }

        continue;
    }

        if (!oled_is_on)
    {
        oled_set_power(true);
        oled_is_on = true;
        xSemaphoreTake(serialMutex, portMAX_DELAY);
        printf("OLED ON - SYSTEM ACTIVE\n");
        xSemaphoreGive(serialMutex);
    }

            char valueText[20];

            oled_clear();

            oled_set_cursor(0, 0);
            oled_write_string("ROOM MONITOR");

            if (current_display_mode == DISPLAY_TEMPERATURE)
            {
                snprintf(
                    valueText,
                    sizeof(valueText),
                    "%.1f C",
                    receivedData.temperature
                );

                oled_set_cursor(2, 0);
                oled_write_string("TEMPERATURE");

                oled_set_cursor(4, 0);
                oled_write_string(valueText);
            }
            else if (current_display_mode == DISPLAY_HUMIDITY)
            {
                snprintf(
                    valueText,
                    sizeof(valueText),
                    "%.1f %%",
                    receivedData.humidity
                );

                oled_set_cursor(2, 0);
                oled_write_string("HUMIDITY");

                oled_set_cursor(4, 0);
                oled_write_string(valueText);
            }
            else if (current_display_mode == DISPLAY_LIGHT)
            {
                snprintf(
                    valueText,
                    sizeof(valueText),
                    "%d %%",
                    receivedData.lightLevel
                );

                oled_set_cursor(2, 0);
                oled_write_string("LIGHT LEVEL");

                oled_set_cursor(4, 0);
                oled_write_string(valueText);
            }
            else if (current_display_mode == DISPLAY_MOTION)
            {
                oled_set_cursor(2, 0);
                oled_write_string("MOTION");

                oled_set_cursor(4, 0);

                if (receivedData.motionDetected)
                {
                    oled_write_string("DETECTED");
                }
                else
                {
                    oled_write_string("NONE");
                }
            }
        }
    }
}

void input_task(void *pvParameters)
{
    gpio_config_t encoder_config = {
        .pin_bit_mask =
            (1ULL << ENCODER_CLK) |
            (1ULL << ENCODER_DT) |
            (1ULL << ENCODER_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&encoder_config);

    int last_clk = gpio_get_level(ENCODER_CLK);

    while (1)
    {
        if (!system_is_active())
    {
        last_clk = gpio_get_level(ENCODER_CLK);
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
    }

        int current_clk = gpio_get_level(ENCODER_CLK);

        if (current_clk != last_clk && current_clk == 1)
        {
            int dt = gpio_get_level(ENCODER_DT);

            if (dt != current_clk)
            {
                /* Clockwise */
                current_display_mode =
                    (current_display_mode + 1) % 4;

                xSemaphoreTake(serialMutex, portMAX_DELAY);
                printf("Encoder CW -> page %d\n",
                       current_display_mode);
                xSemaphoreGive(serialMutex);
            }
            else
            {
                /* Counterclockwise */
                current_display_mode =
                (current_display_mode + 3) % 4;

                xSemaphoreTake(serialMutex, portMAX_DELAY);
                printf("Encoder CCW -> page %d\n",
                       current_display_mode);
                xSemaphoreGive(serialMutex);
            }
        }

        last_clk = current_clk;

        vTaskDelay(pdMS_TO_TICKS(10));
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