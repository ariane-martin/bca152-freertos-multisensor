#ifndef ALARM_H
#define ALARM_H

typedef enum
{
    ALARM_NORMAL,
    ALARM_LOW_TEMPERATURE,
    ALARM_HIGH_TEMPERATURE
} AlarmState;

AlarmState evaluateTemperature(float temperature);

#endif