//
//  Oscillator.cpp
//  MVS
//
//  Created by Bob Miller on 11/17/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Oscillator.h"

Oscillator::Oscillator()
{}

void Oscillator::initialize(Float64        sampleRate,
                            OscillatorType type)
{
    mSampleRate        = sampleRate;
    mType              = type;
    mInverseSampleRate = 1.0 / sampleRate;
    mPhase             = 0.0;
}

void Oscillator::generate(Float64 freq, float *sampBuf, UInt32 count)
{
    Float64 phase = mPhase;
    switch (mType) {

    case OT_Sine:
        // ???
        break;

    case OT_Saw:
        for (UInt32 i = 0; i < count; i++) {
            phase += 2 * freq * mInverseSampleRate;
            if (phase > 1.0)
                phase -= 2.0;
            sampBuf[i] = phase;
        }
        break;

    case OT_Triangle:
        // ???
        break;

    case OT_Pulse:
        // ???
        break;

    }
    mPhase = phase;
}
