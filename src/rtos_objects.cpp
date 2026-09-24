#include "rtos_objects.h"

// Definitions of shared FreeRTOS objects
QueueHandle_t sensor_queue = nullptr;
QueueHandle_t alarm_queue = nullptr;
EventGroupHandle_t system_events = nullptr;
SemaphoreHandle_t serialMutex = nullptr;