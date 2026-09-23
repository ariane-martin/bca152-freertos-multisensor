#include <stdio.h>

#include "display.h"
#include "sensors.h"
#include "input.h"
#include "system_state.h"
#include "rtos_objects.h"

extern "C" {
#include "oled.h"
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

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

            if (input_get_display_mode() == 0)
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
            else if (input_get_display_mode() == 1)
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
            else if (input_get_display_mode() == 2)
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
            else if (input_get_display_mode() == 3)
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