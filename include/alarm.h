#ifndef ALARM_H
#define ALARM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ALARM_NORMAL,
    ALARM_LOW_TEMPERATURE,
    ALARM_HIGH_TEMPERATURE
} AlarmState;

AlarmState evaluateTemperature(float temperature);

void alarm_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif