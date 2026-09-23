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
float sensors_get_latest_temperature(void);

#ifdef __cplusplus
}
#endif

#endif