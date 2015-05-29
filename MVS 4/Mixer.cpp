//
//  Mixer.cpp
//  MVS 4
//
//  Created by Bob Miller on 12/24/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Mixer.h"

Mixer::Mixer()
{}

void Mixer::initialize(float sample_rate)
{}

void Mixer::generate(Operator     op,
                     float const *src1, float const *src1_level,
                     float const *src2, float const *src2_level,
                     float const *src3, float const *src3_level,
                     float       *out,
                     size_t       nsamp)
{
    switch (op) {

        case Mix:
        case HardSync:
            generate_mix    (src1, src1_level,
                             src2, src2_level,
                             src3, src3_level,
                             out,  nsamp);
            break;

        case RingMod:
            generate_ringmod(src1, src1_level,
                             src2, src2_level,
                             src3, src3_level,
                             out,  nsamp);
            break;
    }
}

void Mixer::generate_mix(float const *src1, float const *src1_level,
                         float const *src2, float const *src2_level,
                         float const *src3, float const *src3_level,
                         float       *out,
                         size_t       nsamp)
{
    for (size_t i = 0; i < nsamp; i++)
        out[i] = src1_level[i] * src1[i] +
                 src2_level[i] * src2[i] +
                 src3_level[i] * src3[i];
}

void Mixer::generate_ringmod(float const *src1, float const *src1_level,
                             float const *src2, float const *src2_level,
                             float const *src3, float const *src3_level,
                             float       *out,
                             size_t       nsamp)
{
    for (size_t i = 0; i < nsamp; i++)
        out[i] = src1_level[i] * src1[i] * src2[i] +
                 src3_level[i] * src3[i];
}
