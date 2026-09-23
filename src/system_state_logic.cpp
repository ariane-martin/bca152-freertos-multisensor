#include "system_state.h"

SystemState evaluateSystemState(
    SystemState currentState,
    bool motionDetected,
    bool inactivityTimeout
)
{
    if (motionDetected)
    {
        return SYSTEM_ACTIVE;
    }

    if (currentState == SYSTEM_ACTIVE && inactivityTimeout)
    {
        return SYSTEM_INACTIVE;
    }

    return currentState;
}