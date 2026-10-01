#include <stdio.h>

#include "alarm.h"
#include "sensors.h"
#include "system_state.h"
#include "rtos_objects.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define BUZZER_PIN GPIO_NUM_26

void alarm_task(void *pvParameters)
{
    ledc_timer_config_t buzzer_timer = {};
    buzzer_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    buzzer_timer.duty_resolution = LEDC_TIMER_10_BIT;
    buzzer_timer.timer_num = LEDC_TIMER_0;
    buzzer_timer.freq_hz = 2000;
    buzzer_timer.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&buzzer_timer);

    ledc_channel_config_t buzzer_channel = {};
    buzzer_channel.gpio_num = BUZZER_PIN;
    buzzer_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    buzzer_channel.channel = LEDC_CHANNEL_0;
    buzzer_channel.timer_sel = LEDC_TIMER_0;
    buzzer_channel.duty = 0;
    buzzer_channel.hpoint = 0;
    ledc_channel_config(&buzzer_channel);

    AlarmState previous_alarm = ALARM_NORMAL;
    SensorData data;

    while (1)
    {
        /* Block until SensorTask sends new sensor data */
        if (xQueueReceive(
                alarm_queue,
                &data,
                portMAX_DELAY) == pdPASS)
        {
            if (system_is_active())
            {
                AlarmState alarm =
                    evaluateTemperature(data.temperature);

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

                if (alarm != ALARM_NORMAL)
                {
                    // Turn buzzer ON: 2 kHz tone at 50% duty cycle
                    ledc_set_duty(
                        LEDC_LOW_SPEED_MODE,
                        LEDC_CHANNEL_0,
                        512
                    );

                    ledc_update_duty(
                        LEDC_LOW_SPEED_MODE,
                        LEDC_CHANNEL_0
                    );
                }
                else
                {
                    // Turn buzzer OFF
                    ledc_set_duty(
                        LEDC_LOW_SPEED_MODE,
                        LEDC_CHANNEL_0,
                        0
                    );

                    ledc_update_duty(
                        LEDC_LOW_SPEED_MODE,
                        LEDC_CHANNEL_0
                    );
                }
            }
            else
            {
                xEventGroupClearBits(
                    system_events,
                    EVENT_ALARM
                );

                ledc_set_duty(
                LEDC_LOW_SPEED_MODE,
                LEDC_CHANNEL_0,
                0
            );

            ledc_update_duty(
                LEDC_LOW_SPEED_MODE,
                LEDC_CHANNEL_0
            );
                previous_alarm = ALARM_NORMAL;
            }
        }
    }
}