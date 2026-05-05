#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Bus Bus;

typedef enum {
    JOY_EVENT_INPUT,
    JOY_EVENT_RESET
} JoyEventType;

typedef struct {
    JoyEventType type;

    uint8_t buttons;
    uint16_t duration;
} JoyEvent;

typedef struct {
    const JoyEvent *events;
    int count;

    int index;
    int timer;

    uint8_t current_buttons;
    bool active;
} JoyPlayer;

void joyplayer_init(JoyPlayer *p, const JoyEvent *events, int count);
void joyplayer_update(JoyPlayer *p, Bus *bus);
void joyplayer_reset(JoyPlayer *p);

#ifdef __cplusplus
}
#endif
