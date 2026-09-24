#include "system_state.h"
#include "rtos_objects.h"

static SystemState current_system_state = SYSTEM_ACTIVE;

void system_state_init(void)
{
    current_system_state = SYSTEM_ACTIVE;
    xEventGroupSetBits(system_events, EVENT_ACTIVE);
}

void system_state_set(SystemState state)
{
    current_system_state = state;

    if (state == SYSTEM_ACTIVE)
    {
        xEventGroupSetBits(system_events, EVENT_ACTIVE);
    }
    else
    {
        xEventGroupClearBits(system_events, EVENT_ACTIVE);
    }
}

SystemState system_state_get(void)
{
    return current_system_state;
}

bool system_is_active(void)
{
    EventBits_t bits = xEventGroupGetBits(system_events);
    return (bits & EVENT_ACTIVE) != 0;
}