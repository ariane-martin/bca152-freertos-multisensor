#include <stdio.h>

#include "motion.h"
#include "system_state.h"
#include "rtos_objects.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"

#define PIR_PIN GPIO_NUM_27

void motion_task(void *pvParameters)
{
    gpio_config_t pir_config = {};
    pir_config.pin_bit_mask = (1ULL << PIR_PIN);
    pir_config.mode = GPIO_MODE_INPUT;
    pir_config.pull_up_en = GPIO_PULLUP_DISABLE;
    pir_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    pir_config.intr_type = GPIO_INTR_DISABLE;

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

            /* Wake system if inactive */
            if (!system_is_active())
            {
                system_state_set(SYSTEM_ACTIVE);

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
            if (system_is_active() &&
                ((currentTime - lastMotionTime) >=
                 pdMS_TO_TICKS(15000)))
            {
                system_state_set(SYSTEM_INACTIVE);

                xSemaphoreTake(serialMutex, portMAX_DELAY);
                printf("SYSTEM STATE: INACTIVE\n");
                xSemaphoreGive(serialMutex);
            }
        }

        /* Block briefly instead of busy looping */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}