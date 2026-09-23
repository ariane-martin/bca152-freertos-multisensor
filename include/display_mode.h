#ifndef DISPLAY_MODE_H
#define DISPLAY_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    DISPLAY_TEMPERATURE = 0,
    DISPLAY_HUMIDITY = 1,
    DISPLAY_LIGHT = 2,
    DISPLAY_MOTION = 3
} DisplayMode;

DisplayMode nextDisplayMode(DisplayMode current);
DisplayMode previousDisplayMode(DisplayMode current);

#ifdef __cplusplus
}
#endif

#endif