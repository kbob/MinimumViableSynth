//
//  Amplifier.cpp
//  MVS 4
//
//  Created by Bob Miller on 12/31/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Amplifier.h"

Amplifier::Amplifier()
{}

void Amplifier::initialize(float sample_rate)
{}

void Amplifier::generate_sum(float const *signal,
                             float const *gain,
                             float       *out,
                             size_t       count)
{
    for (size_t i = 0; i < count; i++)
        out[i] += gain[i] * signal[i];
}
