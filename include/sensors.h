#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
} SensorData;

void sensors_init(void);
void sensor_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif