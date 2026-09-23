#include <stdio.h>

#include "alarm.h"
#include "sensors.h"
#include "system_state.h"
#include "rtos_objects.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"

#define BUZZER_PIN GPIO_NUM_26

void alarm_task(void *pvParameters)
{
    gpio_config_t buzzer_config = {};

    buzzer_config.pin_bit_mask = (1ULL << BUZZER_PIN);
    buzzer_config.mode = GPIO_MODE_OUTPUT;
    buzzer_config.pull_up_en = GPIO_PULLUP_DISABLE;
    buzzer_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    buzzer_config.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&buzzer_config);
    gpio_set_level(BUZZER_PIN, 0);

    AlarmState previous_alarm = ALARM_NORMAL;

    while (1)
    {
        if (system_is_active())
        {
            AlarmState alarm =
                evaluateTemperature(
                    sensors_get_latest_temperature()
                );

            if (alarm != previous_alarm)
            {
                xSemaphoreTake(serialMutex, portMAX_DELAY);

                if (alarm == ALARM_LOW_TEMPERATURE)
                {
                    printf("ALARM: TEMPERATURE TOO LOW\n");
                }
                else if (alarm == ALARM_HIGH_TEMPERATURE)
                {
                    printf("ALARM: TEMPERATURE TOO HIGH\n");
                }
                else
                {
                    printf("ALARM: TEMPERATURE NORMAL\n");
                }

                xSemaphoreGive(serialMutex);

                previous_alarm = alarm;
            }

            if (alarm != ALARM_NORMAL)
            {
                xEventGroupSetBits(
                    system_events,
                    EVENT_ALARM
                );
            }
            else
            {
                xEventGroupClearBits(
                    system_events,
                    EVENT_ALARM
                );
            }

            gpio_set_level(
                BUZZER_PIN,
                alarm != ALARM_NORMAL
            );
        }
        else
        {
            xEventGroupClearBits(
                system_events,
                EVENT_ALARM
            );

            gpio_set_level(BUZZER_PIN, 0);

            previous_alarm = ALARM_NORMAL;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}