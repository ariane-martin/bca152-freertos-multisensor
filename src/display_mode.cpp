#include "display_mode.h"

DisplayMode nextDisplayMode(DisplayMode current)
{
    return (DisplayMode)((current + 1) % 4);
}

DisplayMode previousDisplayMode(DisplayMode current)
{
    return (DisplayMode)((current + 3) % 4);
}