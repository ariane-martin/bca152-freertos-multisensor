#include <stdio.h>

#include "input.h"
#include "system_state.h"
#include "rtos_objects.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"

#define ENCODER_CLK GPIO_NUM_32
#define ENCODER_DT  GPIO_NUM_33
#define ENCODER_SW  GPIO_NUM_25

static int current_display_mode = 0;

static TaskHandle_t input_task_handle = NULL;

/*
 * GPIO interrupt handler.
 * It does very little work:
 * it only wakes InputTask.
 */
static void IRAM_ATTR encoder_isr_handler(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (input_task_handle != NULL)
    {
        vTaskNotifyGiveFromISR(
            input_task_handle,
            &higher_priority_task_woken
        );
    }

    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

int input_get_display_mode(void)
{
    return current_display_mode;
}

void input_task(void *pvParameters)
{
    input_task_handle = xTaskGetCurrentTaskHandle();

    /*
     * Configure rotary encoder pins.
     */
    gpio_config_t encoder_config = {};

    encoder_config.pin_bit_mask =
        (1ULL << ENCODER_CLK) |
        (1ULL << ENCODER_DT) |
        (1ULL << ENCODER_SW);

    encoder_config.mode = GPIO_MODE_INPUT;
    encoder_config.pull_up_en = GPIO_PULLUP_ENABLE;
    encoder_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    encoder_config.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&encoder_config);

    /*
     * Generate interrupts whenever CLK changes.
     */
    gpio_set_intr_type(
        ENCODER_CLK,
        GPIO_INTR_ANYEDGE
    );

    /*
     * Install ESP32 GPIO ISR service.
     */
    esp_err_t isr_result =
        gpio_install_isr_service(0);

    if (isr_result != ESP_OK &&
        isr_result != ESP_ERR_INVALID_STATE)
    {
        xSemaphoreTake(serialMutex, portMAX_DELAY);
        printf("ERROR: Failed to install encoder ISR service\n");
        xSemaphoreGive(serialMutex);
    }

    gpio_isr_handler_add(
        ENCODER_CLK,
        encoder_isr_handler,
        NULL
    );

    int last_clk = gpio_get_level(ENCODER_CLK);

    while (1)
    {
        /*
         * Sleep here until the encoder interrupt wakes us.
         *
         * No polling loop.
         * No busy waiting.
         */
        ulTaskNotifyTake(
            pdTRUE,
            portMAX_DELAY
        );

        int current_clk = gpio_get_level(ENCODER_CLK);

        /*
         * Only process one edge of CLK.
         */
        if (current_clk == 1 && last_clk == 0)
        {
            int dt = gpio_get_level(ENCODER_DT);

            /*
             * Ignore navigation while the system is inactive.
             */
            if (system_is_active())
            {
                if (dt == 0)
                {
                    current_display_mode =
                        (current_display_mode + 1) % 4;

                    xSemaphoreTake(
                        serialMutex,
                        portMAX_DELAY
                    );

                    printf(
                        "Encoder CW -> page %d\n",
                        current_display_mode
                    );

                    xSemaphoreGive(serialMutex);
                }
                else
                {
                    current_display_mode =
                        (current_display_mode + 3) % 4;

                    xSemaphoreTake(
                        serialMutex,
                        portMAX_DELAY
                    );

                    printf(
                        "Encoder CCW -> page %d\n",
                        current_display_mode
                    );

                    xSemaphoreGive(serialMutex);
                }
            }
        }

        last_clk = current_clk;
    }
}