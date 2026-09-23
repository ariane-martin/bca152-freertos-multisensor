#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SYSTEM_ACTIVE,
    SYSTEM_INACTIVE
} SystemState;

void system_state_init(void);
void system_state_set(SystemState state);
bool system_is_active(void);
SystemState system_state_get(void);

#ifdef __cplusplus
}
#endif

#endif