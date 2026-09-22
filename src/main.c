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

typedef struct
{
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
} SensorData;

typedef enum
{
    DISPLAY_TEMPERATURE,
    DISPLAY_HUMIDITY,
    DISPLAY_LIGHT,
    DISPLAY_MOTION
} DisplayMode;

static DisplayMode current_display_mode = DISPLAY_TEMPERATURE;

typedef enum
{
    SYSTEM_ACTIVE,
    SYSTEM_INACTIVE
} SystemState;

static SystemState current_system_state = SYSTEM_ACTIVE;

#define ENCODER_CLK GPIO_NUM_32
#define ENCODER_DT  GPIO_NUM_33
#define ENCODER_SW  GPIO_NUM_25
#define PIR_PIN GPIO_NUM_27
#define BUZZER_PIN GPIO_NUM_26
#define EVENT_ACTIVE BIT0
#define EVENT_MOTION BIT1
#define EVENT_ALARM  BIT2

/* ADC handle used by SensorTask */
static adc_oneshot_unit_handle_t adc_handle;
static QueueHandle_t sensor_queue;
static EventGroupHandle_t system_events;
static SemaphoreHandle_t serialMutex;
static float latest_temperature = 0.0f;

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

        /* Convert ADC reading to relative 0-100% light level */
        int light_level = (raw_light * 100) / 4095;

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
        if (current_system_state == SYSTEM_ACTIVE)
        {
            AlarmState alarm =
                evaluateTemperature(latest_temperature);

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

        if (current_system_state == SYSTEM_INACTIVE)
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
        if (current_system_state == SYSTEM_INACTIVE)
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

void motion_task(void *pvParameters)
{
    gpio_config_t pir_config = {
        .pin_bit_mask = (1ULL << PIR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&pir_config);

    TickType_t lastMotionTime = xTaskGetTickCount();

    xSemaphoreTake(serialMutex, portMAX_DELAY);
    printf("MotionTask started - SYSTEM ACTIVE\n");
    xSemaphoreGive(serialMutex);

    while (1)
    {
        int motion = gpio_get_level(PIR_PIN);

        if (motion == 1)
        {
            xEventGroupSetBits(system_events, EVENT_MOTION);

            /* Motion detected: reset inactivity timer */
            lastMotionTime = xTaskGetTickCount();

            /* Wake the system if it was inactive */
            if (current_system_state == SYSTEM_INACTIVE)
            {
                current_system_state = SYSTEM_ACTIVE;
                xEventGroupSetBits(system_events, EVENT_ACTIVE);
                xSemaphoreTake(serialMutex, portMAX_DELAY);
                printf("SYSTEM STATE: ACTIVE\n");
                xSemaphoreGive(serialMutex);
            }
        }
        else
        {
            xEventGroupClearBits(system_events, EVENT_MOTION);
            
            TickType_t currentTime = xTaskGetTickCount();

            /* No motion for 15 seconds */
            if ((current_system_state == SYSTEM_ACTIVE) &&
                ((currentTime - lastMotionTime) >= pdMS_TO_TICKS(15000)))
            {
                current_system_state = SYSTEM_INACTIVE;
                xEventGroupClearBits(system_events, EVENT_ACTIVE);
                xSemaphoreTake(serialMutex, portMAX_DELAY);
                printf("SYSTEM STATE: INACTIVE\n");
                xSemaphoreGive(serialMutex);
            }
        }

        /* MotionTask blocks briefly instead of busy looping */
        vTaskDelay(pdMS_TO_TICKS(100));
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
    xEventGroupSetBits(system_events, EVENT_ACTIVE);

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