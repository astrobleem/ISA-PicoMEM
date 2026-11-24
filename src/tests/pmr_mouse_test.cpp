/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host test: c++ -std=c++11 -Wall -Wextra -Werror pmr_mouse_test.cpp && ./a.out */
#include "../pm_libs/inputs/pmr_mouse.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

struct mouse_event { bool event; int8_t x, y; uint8_t buttons; };
static volatile mouse_event mouse = {false, 0, 0, 0};
static uint8_t ram[PMR_MOUSE_OFFSET + PMR_MOUSE_SIZE + 1];
static uint8_t *const r = ram + PMR_MOUSE_OFFSET;
static uint8_t boot = 42;

static bool pump() { return pmr_mouse::tick(r, boot, mouse); }
static void report(int x, int y, unsigned b)
{
    mouse.x = x; mouse.y = y; mouse.buttons = b; mouse.event = true;
}
static void packet(int dx, int dy, unsigned buttons)
{
    assert(r[5] != r[6]);
    unsigned p = 8 + 3 * r[6];
    assert((int8_t)r[p] == dx && (int8_t)r[p+1] == dy && r[p+2] == buttons);
    r[6] = (r[6] + 1) & 15;
}
int main()
{
    r[-1] = r[64] = 0xaa;
    r[3] = 7;
    assert(!pump() && r[3] == 7 && r[0] == 0);
    r[3] = 0;
    report(23, -11, 1);
    assert(!pump() && mouse.event);
    assert(memcmp(r, "PMR\1", 4) == 0 && r[4] == 1 && r[63] == boot);
    assert(!pump() && mouse.event);  // idle leaves the legacy event unconsumed
    r[7] = 1;
    assert(pump() && mouse.event && r[4] == 2);
    packet(0, 0, 1);
    assert(pump() && !mouse.event);
    packet(23, -11, 1);
    assert(pump() && r[5] == r[6]);

    for (unsigned i = 0; i < 15; ++i) {
        report(1, -1, 2);
        assert(pump() && !mouse.event);
    }
    unsigned h = r[5];
    report(127, -128, 3);
    assert(pump() && r[5] == h && r[56] == 1 && !mouse.event);
    report(127, -128, 0);
    assert(pump() && r[5] == h && !mouse.event);
    for (unsigned i = 0; i < 15; ++i) packet(1, -1, 2);
    assert(pump()); packet(127, -128, 0);
    assert(pump()); packet(127, -128, 0);
    assert(pump() && r[5] == r[6]);  // wrapped ring, no duplicate remainder
    report(0, 0, 4);
    assert(pump()); packet(0, 0, 4); // button change without motion

    r[7] = 0;
    report(-10, 10, 2);
    assert(!pump() && mouse.event && r[4] == 1 && r[5] == r[6]);
    r[7] = 1;
    assert(pump() && r[4] == 2);
    packet(0, 0, 2);
    ++boot;
    assert(!pump() && !r[7] && r[4] == 1 && r[63] == boot);
    assert(r[5] == 0 && r[6] == 0);
    r[7] = 1; r[6] = 16;
    assert(pump() && r[4] == 0 && r[5] == 0);
    r[7] = 0;
    assert(!pump() && r[4] == 1 && r[6] == 0);
    assert(r[-1] == 0xaa && r[64] == 0xaa);
    puts("PMR1 producer: legacy, lease, motion, buttons, ring wrap/full, epoch and bounds passed");
}
