#include <fwkes/joyplayer.h>
#include <fwkes/bus.h>

void joyplayer_init(JoyPlayer *p, const JoyEvent *events, int count) {
    p->events = events;
    p->count = count;
    p->index = 0;
    p->timer = 0;
    p->current_buttons = 0;
    p->active = true;
}

void joyplayer_reset(JoyPlayer *p) {
    p->timer = 0;
    p->current_buttons = 0;
}

void joyplayer_update(JoyPlayer *p, struct Bus *bus) {
    if (!p->active) return;

    if (p->index >= p->count) {
        p->index = 0;
    }

    if (p->timer == 0) {
        JoyEvent ev = p->events[p->index++];

        if (ev.type == JOY_EVENT_RESET) {
            BusEvent bev = { .id = BUS_EVENT_RESET };
            bus_add_event(bus, &bev);
            return;
        }

        p->current_buttons = ev.buttons;
        p->timer = ev.duration ? ev.duration : 1;
    }

    --p->timer;
}
