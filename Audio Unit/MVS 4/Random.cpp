//
//  Random.cpp
//  MVS 4
//
//  Created by Bob Miller on 12/20/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Random.h"

static uint32_t seed = 1;

uint32_t carta_random()
{
    uint32_t lo = 16807 * (seed & 0xFFFF);
    uint32_t hi = 16807 * (seed >> 16);
    lo += (hi & 0x7FFF) << 16;
    lo += hi >> 15;
    if (lo > 0x7FFFFFFF)
        lo -= 0x7FFFFFFF;
    return seed = lo;
}

float frandom()
{
    return carta_random() * (2.0 / 0x7fffffff) - 1;
}
