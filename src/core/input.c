#include "blades.h"
/* Re-arm only after release, including after reconnection. A/B never repeat. */
unsigned input_update(Input *in, unsigned buttons, float dt, bool connected) {
    if (!connected) { *in=(Input){0}; return 0; }
    if (!in->armed) { if (!buttons) in->armed=true; in->previous=buttons; return 0; }
    unsigned result=buttons & ~in->previous;
    unsigned direction=buttons & (UP|DOWN|LEFT|RIGHT);
    if (direction != in->repeat_direction) {
        in->repeat_direction=direction; in->repeat_time=0.36f;
    } else if (direction) {
        in->repeat_time-=dt;
        if (in->repeat_time<=0) { result|=direction; in->repeat_time=0.11f; }
    }
    in->held=buttons; in->previous=buttons; return result;
}
