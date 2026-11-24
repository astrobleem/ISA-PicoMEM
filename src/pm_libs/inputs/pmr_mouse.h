/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <stdint.h>

#define PMR_MOUSE_OFFSET 0xe20
#define PMR_MOUSE_SIZE 64

/* Core 0 owns the producer and runs it before the legacy mouse dispatch.
 * The PC owns request and tail while leased. The last byte is the BIOS epoch.
 */
namespace pmr_mouse {
enum { state=4, head=5, tail=6, request=7, packets=8, mask=15,
       overflow=56, buttons=57, x_pending=58, y_pending=60,
       pending=62, epoch=63 };
enum { idle=1, active=2 };

inline void publish_barrier()
{
#ifdef __arm__
    __asm volatile("dmb sy" ::: "memory");
#else
    __atomic_thread_fence(__ATOMIC_RELEASE);
#endif
}

inline void post(volatile uint8_t *r, unsigned slot, int dx, int dy, uint8_t b)
{
    r[packets + 3 * slot] = (uint8_t)dx;
    r[packets + 3 * slot + 1] = (uint8_t)dy;
    r[packets + 3 * slot + 2] = b;
    publish_barrier();
    r[head] = (slot + 1) & mask;
}

inline int read_pending(volatile uint8_t *r, unsigned p)
{
    unsigned value = r[p];
    value |= (unsigned)r[p+1] << 8;
    return value < 32768 ? (int)value : (int)value - 65536;
}

inline void save_pending(volatile uint8_t *r, unsigned p, int v)
{
    r[p] = (uint8_t)v;
    r[p+1] = (uint8_t)((uint16_t)v >> 8);
}

inline int clamp(int v, int low, int high)
{
    return v < low ? low : v > high ? high : v;
}

inline void reset_pending(volatile uint8_t *r)
{
    r[pending] = 0;
    save_pending(r, x_pending, 0);
    save_pending(r, y_pending, 0);
}

template<class Mouse>
inline bool tick(volatile uint8_t *r, uint8_t boot_count, volatile Mouse &mouse)
{
    const bool empty = !r[0] && !r[1] && !r[2] && !r[3];
    const bool known = r[0] == 'P' && r[1] == 'M' && r[2] == 'R' && r[3] == 1;
    if (!empty && !known) return false;  // Don't take over an unrecognized DPRAM region.
    if (empty || r[epoch] != boot_count) {
        r[state] = 0;
        r[head] = r[tail] = r[request] = r[overflow] = 0;
        reset_pending(r);
        r[epoch] = boot_count;
        r[0] = 'P'; r[1] = 'M'; r[2] = 'R'; r[3] = 1;
        publish_barrier();
        r[state] = idle;
        return false;
    }
    if (r[request] != 1) {
        if (r[state] != idle) {
            r[head] = r[tail] = 0;
            reset_pending(r);
            publish_barrier();
            r[state] = idle;
        }
        return false;
    }
    const unsigned h = r[head], t = r[tail];
    const unsigned mode = r[state];
    if (h > mask || t > mask || (mode != idle && mode != active)) {
        r[state] = 0;
        return true;
    }
    if (mode == idle) {
        if (h == t) {
            reset_pending(r);
            r[overflow] = 0;
            post(r, h, 0, 0, mouse.buttons);
            publish_barrier();
            r[state] = active;
        }
        return true;
    }
    if (mouse.event) {
        save_pending(r, x_pending, clamp(read_pending(r, x_pending) + mouse.x, -32768, 32767));
        save_pending(r, y_pending, clamp(read_pending(r, y_pending) + mouse.y, -32768, 32767));
        r[buttons] = mouse.buttons;
        r[pending] = 1;
        mouse.event = false;
    }
    if (!r[pending]) return true;
    if (((h + 1) & mask) == t) {
        r[overflow] = 1;
        return true;
    }
    const int x = read_pending(r, x_pending), y = read_pending(r, y_pending);
    const int dx = clamp(x, -128, 127), dy = clamp(y, -128, 127);
    save_pending(r, x_pending, x - dx);
    save_pending(r, y_pending, y - dy);
    post(r, h, dx, dy, r[buttons]);
    if (x == dx && y == dy) r[pending] = 0;
    return true;
}
}
