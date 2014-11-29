//
//  Oscillator.cpp
//  MVS
//
//  Created by Bob Miller on 11/17/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Oscillator.h"

#define BLEP_ORDER 3
#define BLAM_ORDER 1

#if BLEP_ORDER == 3

    #define BLEP_ME(d, h)                                               \
        ((zm2 += (h) * (+ (d)*(d)*(d)*(d)/24)),                         \
         (zm1 += (h) * (- (d)*(d)*(d)*(d)/8                             \
                        + (d)*(d)*(d)/6                                 \
                        + (d)*(d)/4                                     \
                        + (d)/6                                         \
                        + 1/24.)),                                      \
         (z0  += (h) * (+ (d)*(d)*(d)*(d)/8                             \
                        - (d)*(d)*(d)/3                                 \
                        + (d)*2/3                                       \
                        - 1/2.)),                                       \
         (zp1 += (h) * (- (d)*(d)*(d)*(d)/24                            \
                        + (d)*(d)*(d)/6                                 \
                        - (d)*(d)/4                                     \
                        + (d)/6                                         \
                        - 1/24.)))

#elif BLEP_ORDER == 1

    #define BLEP_ME(d, h)                                               \
          ((zm1 += (h) * (+ (d)*(d)/2)),                                \
           (z0  += (h) * (- (d)*(d)/2                                   \
                          + (d)                                         \
                          - 1/2.)))

#elif BLEP_ORDER == 0

    #define BLEP_ME(d, h) ((void)0) /* La di da... */

#else

    #error "unknown BLEP order"

#endif

#if BLAM_ORDER == 1

    #define BLAM_ME(d, h)                                               \
        ((zm1 += (h) * (+ (d)*(d)*(d)/6)),                              \
         (z0  += (h) * (+ (1-(d))*(1-(d))*(1-(d))/6)))

#elif BLAM_ORDER == 0

    #define BLAM_ME(d, h) ((void)0)

#else

    #error "unknown BLAM order"

#endif

Oscillator::Oscillator()
{}

void Oscillator::initialize(Float64 sampleRate, Type type)
{
    mSampleRate = sampleRate;
    mType = type;
    mInverseSampleRate = 1.0 / sampleRate;
    mPhase = 0.0;

    Float32 zm2 = 0, zm1 = 0, z0 = 0, zp1 = 0;
    switch (type) {

    case Saw:
        BLEP_ME(0, -1);
        break;

    case Square:
        BLEP_ME(0, +1);
        break;

    case Triangle:
        BLEP_ME(0, -1);
        break;

    case Sine:
        break;
    }
    mZm2 = zm2;
    mZm1 = zm1;
    mZ0  = z0;
    mZp1 = zp1;
}

void Oscillator::generate(Float64  freq,
                          Float32  modifier,
                          Float32 *sampBuf,
                          UInt32   count)
{
    switch (mType) {
    case Saw:
        generate_saw(freq, modifier, sampBuf, count);
        break;

    case Square:
        generate_square(freq, modifier, sampBuf, count);
        break;

    case Triangle:
        generate_triangle(freq, modifier, sampBuf, count);
        break;

    case Sine:
        generate_sine(freq, modifier, sampBuf, count);
        break;
    }
}

void Oscillator::generate_modulated(Float32        modifier,
                                    Float32       *sampBuf,
                                    Float32 const *phaseIncrements,
                                    UInt32         count)
{
    switch (mType) {
    case Saw:
        generate_modulated_saw(modifier, sampBuf, phaseIncrements, count);
        break;

    case Square:
        generate_modulated_square(modifier, sampBuf, phaseIncrements, count);
        break;

    case Triangle:
        generate_modulated_triangle(modifier, sampBuf, phaseIncrements, count);
        break;

    case Sine:
        generate_modulated_sine(modifier, sampBuf, phaseIncrements, count);
        break;
    }
}

void Oscillator::generate_saw(Float64  freq,
                              Float32  modifier,
                              Float32 *sampBuf,
                              UInt32   count)
{
    Float32 zm2     = mZm2;
    Float32 zm1     = mZm1;
    Float32 z0      = mZ0;
    Float32 zp1     = mZp1;
    Float32 phase   = mPhase;
    Float32 inc     = freq * mInverseSampleRate;
    Float32 inv_inc = 1 / inc;
    for (size_t i = 0; i < count; i++) {
        phase += inc;
        if (phase >= 1.0) {
            // negative discontinuity
            phase -= 1.0;
            BLEP_ME(phase * inv_inc, -2.0);
        }
        z0 += 2 * phase - 1;
        sampBuf[i] += zm2;
        zm2 = zm1; zm1 = z0; z0 = zp1; zp1 = 0;
    }
    mZm2   = zm2;
    mZm1   = zm1;
    mZ0    = z0;
    mZp1   = zp1;
    mPhase = phase;
}

void Oscillator::generate_modulated_saw(Float32        modifier,
                                        Float32       *sampBuf,
                                        Float32 const *phaseIncrements,
                                        UInt32         count)
{
    Float32 zm2   = mZm2;
    Float32 zm1   = mZm1;
    Float32 z0    = mZ0;
    Float32 zp1   = mZp1;
    Float32 phase = mPhase;
    for (size_t i = 0; i < count; i++) {
        Float32 inc = phaseIncrements[i];
        phase += inc;
        if (phase >= 1.0) {
            // negative discontinuity
            phase -= 1.0;
            BLEP_ME(phase / inc, -2.0);
        }
        z0 += 2 * phase - 1;
        sampBuf[i] += zm2;
        zm2 = zm1; zm1 = z0; z0 = zp1; zp1 = 0;
    }
    mZm2   = zm2;
    mZm1   = zm1;
    mZ0    = z0;
    mZp1   = zp1;
    mPhase = phase;
}

void Oscillator::generate_square(Float64  freq,
                                 Float32  modifier,
                                 Float32 *sampBuf,
                                 UInt32   count)
{
    Float32 zm2          = mZm2;
    Float32 zm1          = mZm1;
    Float32 z0           = mZ0;
    Float32 zp1          = mZp1;
    Float32 phase        = mPhase;
    Float32 thresh       = (1 - 0.9 * modifier) / 2;
    Float32 high         = 1.0;
    Float32 low          = thresh / (thresh - 1);
    Float32 level        = phase < thresh ? high : low;
    Float32 fall_trigger = phase < thresh ? thresh : 9999;
    Float32 inc          = freq * mInverseSampleRate;
    Float32 inv_inc      = 1 / inc;
    for (size_t i = 0; i < count; i++) {
        phase += inc;
        if (phase >= 1.0) {
            // positive discontinuity
            phase -= 1.0;
            level = high;
            BLEP_ME(phase * inv_inc, high - low);
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // negative discontinuity
            level = low;
            BLEP_ME((phase - thresh) * inv_inc, low - high);
            fall_trigger = 9999;
        }
        z0 += level;
        sampBuf[i] += zm2;
        zm2 = zm1; zm1 = z0; z0 = zp1; zp1 = 0;
    }
    mZm2   = zm2;
    mZm1   = zm1;
    mZ0    = z0;
    mZp1   = zp1;
    mPhase = phase;
}

void Oscillator::generate_modulated_square(Float32        modifier,
                                           Float32       *sampBuf,
                                           Float32 const *phaseIncrements,
                                           UInt32         count)
{
    Float32 zm2          = mZm2;
    Float32 zm1          = mZm1;
    Float32 z0           = mZ0;
    Float32 zp1          = mZp1;
    Float32 phase        = mPhase;
    Float32 thresh       = (1 - 0.9 * modifier) / 2;
    Float32 high         = 1.0;
    Float32 low          = thresh / (thresh - 1);
    Float32 level        = phase < thresh ? high : low;
    Float32 fall_trigger = phase < thresh ? thresh : 9999;
    for (size_t i = 0; i < count; i++) {
        Float32 inc = phaseIncrements[i];
        phase += inc;
        if (phase >= 1.0) {
            // positive discontinuity
            phase -= 1.0;
            level = high;
            BLEP_ME(phase / inc, high - low);
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // negative discontinuity
            level = low;
            BLEP_ME((phase - thresh) / inc, low - high);
            fall_trigger = 9999;
        }
        z0 += level;
        sampBuf[i] += zm2;
        zm2 = zm1; zm1 = z0; z0 = zp1; zp1 = 0;
    }
    mZm2   = zm2;
    mZm1   = zm1;
    mZ0    = z0;
    mZp1   = zp1;
    mPhase = phase;
}

void Oscillator::generate_triangle(Float64  freq,
                                   Float32  modifier,
                                   Float32 *sampBuf,
                                   UInt32   count)
{
    Float32 zm2          = mZm2;
    Float32 zm1          = mZm1;
    Float32 z0           = mZ0;
    Float32 zp1          = mZp1;
    Float32 phase        = mPhase;
    Float32 thresh       = 0.5 + 0.49 * modifier; // maximum skew is 98%.
    Float32 up_slope     = 2 / thresh;
    Float32 dn_slope     = -2 / (1 - thresh);
    Float32 m            = phase < thresh ? up_slope : dn_slope;
    Float32 o            = phase < thresh ? 0 : thresh;
    Float32 b            = phase < thresh ? -1 : +1;
    Float32 fall_trigger = phase < thresh ? thresh : 9999;
    Float32 inc          = freq * mInverseSampleRate;
    Float32 inv_inc      = 1 / inc;
    for (size_t i = 0; i < count; i++) {
        phase += inc;
        if (phase >= 1.0) {
            // bottom corner
            phase -= 1.0;
            m = up_slope;
            o = 0;
            b = -1;
            BLAM_ME(phase * inv_inc, (up_slope - dn_slope) * inc);
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // top corner
            m = dn_slope;
            o = thresh;
            b = +1;
            BLAM_ME((phase - thresh) * inv_inc, (dn_slope - up_slope) * inc);
            fall_trigger = 9999;
        }
        z0 += m * (phase - o) + b;;
        sampBuf[i] += zm2;
        zm2 = zm1; zm1 = z0; z0 = zp1; zp1 = 0;
    }
    mZm2   = zm2;
    mZm1   = zm1;
    mZ0    = z0;
    mZp1   = zp1;
    mPhase = phase;
}

void Oscillator::generate_modulated_triangle(Float32        modifier,
                                             Float32       *sampBuf,
                                             Float32 const *phaseIncrements,
                                             UInt32         count)
{
    Float32 zm2          = mZm2;
    Float32 zm1          = mZm1;
    Float32 z0           = mZ0;
    Float32 zp1          = mZp1;
    Float32 phase        = mPhase;
    Float32 thresh       = 0.5 + 0.49 * modifier; // maximum skew is 98%.
    Float32 up_slope     = 2 / thresh;
    Float32 dn_slope     = -2 / (1 - thresh);
    Float32 m            = phase < thresh ? up_slope : dn_slope;
    Float32 o            = phase < thresh ? 0 : thresh;
    Float32 b            = phase < thresh ? -1 : +1;
    Float32 fall_trigger = phase < thresh ? thresh : 9999;
    for (size_t i = 0; i < count; i++) {
        Float32 inc = phaseIncrements[i];
        phase += inc;
        if (phase >= 1.0) {
            // bottom corner
            phase -= 1.0;
            m = up_slope;
            o = 0;
            b = -1;
            BLAM_ME(phase / inc, (up_slope - dn_slope) * inc);
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // top corner
            m = dn_slope;
            o = thresh;
            b = +1;
            BLAM_ME((phase - thresh) / inc, (dn_slope - up_slope) * inc);
            fall_trigger = 9999;
        }
        z0 += m * (phase - o) + b;;
        sampBuf[i] += zm2;
        zm2 = zm1; zm1 = z0; z0 = zp1; zp1 = 0;
    }
    mZm2   = zm2;
    mZm1   = zm1;
    mZ0    = z0;
    mZp1   = zp1;
    mPhase = phase;
}


void Oscillator::generate_sine(Float64  freq,
                               Float32  modifier,
                               Float32 *sampBuf,
                               UInt32   count)
{
    Float32 phase = mPhase;
    Float32 inc   = freq * mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += inc;
        if (phase >= 1.0) {
            phase -= 1.0;
        }
        sampBuf[i] += sin(2 * M_PI * phase);
    }
    mPhase = phase;
}

void Oscillator::generate_modulated_sine(Float32        modifier,
                                         Float32       *sampBuf,
                                         Float32 const *phaseIncrements,
                                         UInt32         count)
{
    Float32 phase = mPhase;
    for (size_t i = 0; i < count; i++) {
        phase += phaseIncrements[i];
        if (phase >= 1.0) {
            phase -= 1.0;
        }
        sampBuf[i] += sin(2 * M_PI * phase);
    }
    mPhase = phase;
}
