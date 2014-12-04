//
//  NoiseSource.cpp
//  MVS 3
//
//  Created by Bob Miller on 12/3/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//
//  References:
//     http://www.firstpr.com.au/dsp/rand31/
//     http://www.firstpr.com.au/dsp/pink-noise/
//     Audacity source

#include "NoiseSource.h"

NoiseSource::NoiseSource()
{}

void NoiseSource::initialize(Float64 sampleRate)
{
    mSampleRate = sampleRate;
    mLeakage = (sampleRate - 144.0) / sampleRate;
    if (mLeakage > 0.9999)
        mLeakage = 0.9999;
    mScaling = 9.0 / sqrt(sampleRate);
    if (mScaling < 0.01)
        mScaling = 0.01;
    mSeed = 1;
    mB0 = 0;
    mB1 = 0;
    mB2 = 0;
    mB3 = 0;
    mB4 = 0;
    mB5 = 0;
    mB6 = 0;
    mY  = 0;
}

inline uint32_t NoiseSource::cartaRandom()
{
    uint32_t lo = 16807 * (mSeed & 0xFFFF);
    uint32_t hi = 16807 * (mSeed >> 16);
    lo += (hi & 0x7FFF) << 16;
    lo += hi >> 15;
    if (lo > 0x7FFFFFFF)
        lo -= 0x7FFFFFFF;
    return mSeed = lo;
}

inline Float32 NoiseSource::frandom()
{
    return cartaRandom() * (2.0 / 0x7fffffff) - 1.0;
}

void NoiseSource::generate(Type type, Float32 *out, size_t n)
{
    switch (type) {

        case White:
            for (size_t i = 0; i < n; i++)
                out[i] = frandom();
            break;

        case Pink:
        {
            Float32 b0 = mB0;
            Float32 b1 = mB1;
            Float32 b2 = mB3;
            Float32 b3 = mB3;
            Float32 b4 = mB4;
            Float32 b5 = mB5;
            Float32 b6 = mB6;
            for (size_t i = 0; i < n; i++) {
                Float32 white = frandom();
                b0 = 0.99886 * b0 + white * 0.0555179;
                b1 = 0.99332 * b1 + white * 0.0750759;
                b2 = 0.96900 * b2 + white * 0.1538520;
                b3 = 0.86650 * b3 + white * 0.3104856;
                b4 = 0.55000 * b4 + white * 0.5329522;
                b5 = -0.7616 * b5 - white * 0.0168980;
                out[i] = 0.129 * (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362);
                b6 = white * 0.115926;
            }
            mB0 = b0;
            mB1 = b1;
            mB2 = b2;
            mB3 = b3;
            mB4 = b4;
            mB5 = b5;
            mB6 = b6;
        }
            break;

        case Red:
            for(size_t i = 0; i < n; i++) {
                Float32 white = frandom();
                Float32 z = mLeakage * mY + white * mScaling;
                mY = (fabs(z) > 1.0) ? (mLeakage * mY - white * mScaling) : z;
                out[i] = mY;
            }
            break;
    }
}
