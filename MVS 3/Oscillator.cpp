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

void Oscillator::initialize(Float64 sampleRate,
                            Type    type)
{
    mSampleRate        = sampleRate;
    mType              = type;
    mInverseSampleRate = 1.0 / sampleRate;
    mPhase             = 0.0;
}

void Oscillator::generate(Float64 freq, Float32 modifier, float *sampBuf, UInt32 count)
{
    Float32 thresh;
    Float64 phase = mPhase;
    switch (mType) {

    case OT_Sine:
        for (UInt32 i = 0; i < count; i++) {
            phase += 2.0 * freq * mInverseSampleRate;
            if (phase > 1.0)
                phase -= 2.0;
            sampBuf[i] = sin(M_PI * phase);
        }
        break;

    case OT_Saw:
        if (modifier == 0) {
            for (UInt32 i = 0; i < count; i++) {
                phase += 2 * freq * mInverseSampleRate;
                if (phase > 1.0)
                    phase -= 2.0;
                sampBuf[i] = phase;
            }
        } else {
            thresh = 1.0 - modifier / 2.0;
            Float32 upSlope = 2.0 / thresh;
            Float32 downSlope = -2.0 / (1 - thresh);
            for (UInt32 i = 0; i < count; i++) {
                phase += freq * mInverseSampleRate;
                if (phase > 1.0)
                    phase -= 1.0;
                Float32 samp;
                if (phase < thresh)
                    samp = phase * upSlope - 1;
                else
                    samp = (phase - thresh) * downSlope + 1;
                sampBuf[i] = samp;
            }
        }
        break;

    case OT_Pulse:
        thresh = 0.5 * (1 - modifier * 0.9);
            Float32 high = 1.0;
            Float32 low = -0.5 / (1 - thresh) / (0.5 / thresh);
        for (UInt32 i = 0; i < count; i++) {
            phase += freq * mInverseSampleRate;
            if (phase > 1.0)
                phase -= 1.0;
            sampBuf[i] = (phase < thresh) ? high : low;
        }
        break;

    }
    mPhase = phase;
}
