#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

// Event Group bits
#define EVENT_ACTIVE BIT0
#define EVENT_MOTION BIT1
#define EVENT_ALARM  BIT2

// Shared FreeRTOS objects
extern QueueHandle_t sensor_queue;
extern QueueHandle_t alarm_queue;
extern EventGroupHandle_t system_events;
extern SemaphoreHandle_t serialMutex;