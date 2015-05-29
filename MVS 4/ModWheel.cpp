//
//  ModWheel.cpp
//  MVS 4
//
//  Created by Bob Miller on 12/20/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "ModWheel.h"

const float decay_time = 0.005; // seconds.  MIDI messages take 3 ms minimum.

ModWheel::ModWheel()
{}

void ModWheel::initialize(double sample_rate)
{
    mSampleRate = sample_rate;
    mDecay = 1 / (decay_time * sample_rate);
    mIntTarget = 0;
    mTarget = 0;
    mActual = 0;
    mLSBSeen = false;
}

void ModWheel::set_raw_MSB(uint8_t value)
{
    const uint16_t LSB_mask = (1 << 7) - (1 << 0);
    const uint16_t MSB_mask = (1 << 14) - (1 << 7);

    if (mLSBSeen)
        mIntTarget = (mIntTarget & ~MSB_mask) | (value << 7 & MSB_mask);
    else
        mIntTarget = (value & LSB_mask) | (value << 7 & MSB_mask);
    mTarget = mIntTarget / (float)((1 << 14) - 1);
}

void ModWheel::set_raw_LSB(uint8_t value)
{
    const uint16_t LSB_mask = (1 << 7) - (1 << 0);

    mIntTarget = (mIntTarget & ~LSB_mask) | (value & LSB_mask);
    mTarget = mIntTarget / (float)((1 << 14) - 1);
    mLSBSeen = true;
}

void ModWheel::generate(float *samples_out, size_t count)
{
    const float decay = mDecay;
    const float non_decay = 1.0 - decay;
    const float target = mTarget;
    float actual = mActual;
    for (size_t i = 0; i < count; i++)
        samples_out[i] = actual = non_decay * actual + decay * target;
    mActual = actual;
}

void ModWheel::generate_scaled(float scale, float *samples_out, size_t count)
{
    const float decay = mDecay;
    const float non_decay = 1.0 - decay;
    const float target = mTarget;
    float actual = mActual;
    for (size_t i = 0; i < count; i++) {
        actual = non_decay * actual + decay * target;
        samples_out[i] = scale * actual;
    }
    mActual = actual;
}