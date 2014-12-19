//
//  NoiseSource.cpp
//  MVS
//
//  Created by Bob Miller on 12/3/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//
//  References:
//
//     S. Plaszczynski. Generating long streams of 1/falpha noise. Fluctuation
//     and Noise Letters, World Scientific Publishing, 2007, 7, pp.R1-R13.
//     <10.1142/S0219477507003635>. <in2p3- 00024797v3>
//
//     http://www.firstpr.com.au/dsp/rand31/
//     http://www.firstpr.com.au/dsp/pink-noise/

#include "NoiseSource.h"

NoiseSource::NoiseSource()
: seed(0)
{}

void NoiseSource::initialize(Float64 sampleRate)
{
    if (seed)
        return;

    double const f_min = 10;    // Hz

    // White initialization
    seed = 1;

    // Pink initialization
    double const f_knee    = sampleRate / M_PI;
    float        log_f0    = log(f_min);
    float  const log_fk    = log(f_knee);
    float  const delta_f   = (log_fk - log_f0) / (Nf - 0.5);
    float  const delta_f_2 = delta_f / 2;

    for (size_t i = 0; i < Nf; i++) {
        float f_min = exp(log_f0 + i * delta_f);
        float f_knee = exp(log_f0 + i * delta_f + delta_f_2);
        double xk = M_PI * f_knee / sampleRate;
        double x0 = M_PI * f_min / sampleRate;
        pink_a0[i] = (1 + xk) / (1 + x0);
        pink_a1[i] = -(1 - xk) / (1 + x0);
        pink_b1[i] = (1 - x0) / (1 + x0);
        pink_xm1[i] = pink_ym1[i] = 0;
    }
    pink_gain = 0.44;

    // Red initialization
    double xk = 1;
    double x0 = M_PI * f_min / sampleRate;
    red_a0 = (1 + xk) / (1 + x0);
    red_b1 = (1 - x0) / (1 + x0);
    red_ym1 = 0;
    red_gain = 0.016;           // sounds about right
}

inline uint32_t NoiseSource::cartaRandom()
{
    uint32_t lo = 16807 * (seed & 0xFFFF);
    uint32_t hi = 16807 * (seed >> 16);
    lo += (hi & 0x7FFF) << 16;
    lo += hi >> 15;
    if (lo > 0x7FFFFFFF)
        lo -= 0x7FFFFFFF;
    return
    seed = lo;
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
            float       xm10 = pink_xm1[0];
            float       xm11 = pink_xm1[1];
            float       xm12 = pink_xm1[2];
            float       xm13 = pink_xm1[3];
            // float       xm14 = pink_xm1[4];
            float       ym10 = pink_ym1[0];
            float       ym11 = pink_ym1[1];
            float       ym12 = pink_ym1[2];
            float       ym13 = pink_ym1[3];
            float       ym14 = pink_ym1[4];
            float const a00  = pink_a0[0];
            float const a01  = pink_a0[1];
            float const a02  = pink_a0[2];
            float const a03  = pink_a0[3];
            float const a04  = pink_a0[4];
            float const a10  = pink_a1[0];
            float const a11  = pink_a1[1];
            float const a12  = pink_a1[2];
            float const a13  = pink_a1[3];
            // float const a14  = pink_a1[4];
            float const b10  = pink_b1[0];
            float const b11  = pink_b1[1];
            float const b12  = pink_b1[2];
            float const b13  = pink_b1[3];
            float const b14  = pink_b1[4];
            float gain = pink_gain;
            for (size_t i = 0; i < n; i++) {
                float x0 = frandom();
                float y0 = a00 * x0 + a10 * xm10 + b10 * ym10;
                float x1 = y0;
                float y1 = a01 * x1 + a11 * xm11 + b11 * ym11;
                float x2 = y1;
                float y2 = a02 * x2 + a12 * xm12 + b12 * ym12;
                float x3 = y2;
                float y3 = a03 * x3 + a13 * xm13 + b13 * ym13;
                float x4 = y3;
                float y4 = a04 * x4              + b14 * ym14;
                out[i] = y4 * gain;
                xm10 = x0;
                xm11 = x1;
                xm12 = x2;
                xm13 = x3;
                // xm14 = x4;
                ym10 = y0;
                ym11 = y1;
                ym12 = y2;
                ym13 = y3;
                ym14 = y4;
            }
            pink_xm1[0] = xm10;
            pink_xm1[1] = xm11;
            pink_xm1[2] = xm12;
            pink_xm1[3] = xm13;
            // pink_xm1[4] = xm14;
            pink_ym1[0] = ym10;
            pink_ym1[1] = ym11;
            pink_ym1[2] = ym12;
            pink_ym1[3] = ym13;
            pink_ym1[4] = ym14;
        }
        break;

    case Red:
        float ym1 = red_ym1;
        for (size_t i = 0; i < n; i++) {
            float x = frandom();
            float y = red_a0 * x + red_b1 * ym1;
            out[i] = y * red_gain;
            ym1 = y;
        }
        red_ym1 = ym1;
        break;
    }
}
