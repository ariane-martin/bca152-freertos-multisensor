#include "alarm.h"

AlarmState evaluateTemperature(float temperature)
{
    if (temperature < 18.0f)
    {
        return ALARM_LOW_TEMPERATURE;
    }
    else if (temperature > 30.0f)
    {
        return ALARM_HIGH_TEMPERATURE;
    }
    else
    {
        return ALARM_NORMAL;
    }
}