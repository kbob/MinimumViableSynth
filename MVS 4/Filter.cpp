//
//  Filter.cpp
//  MVS 4
//
//  Created by Bob Miller on 1/1/15.
//  Copyright (c) 2015 kbobsoft.com. All rights reserved.
//

#include "Filter.h"

static const float Vt       = 0.026; // Thermal noise voltage
static const float ninv_2Vt = -1 / (2 * Vt);

static inline float clamp(float x0, float x1, float x)
{
    if (x < x0)
        return x0;
    if (x > x1)
        return x1;
    return x;
}

Filter::Filter()
{}

void Filter::initialize(float sample_rate)
{
    mSampleRate = sample_rate;
    mInv_2Fs = 1 / (2 * sample_rate);
    S1.v  = S2.v  = S3.v  = S4.v  = 0;
    S1.dv = S2.dv = S3.dv = S4.dv = 0;
    S1.cv = S2.cv = S3.cv = S4.cv = 0;

}

inline void Filter::step_stage(LadderStage& S,
                               float in,
                               float pc_2AFs)
{
    float dv = pc_2AFs * (in - S.cv);
    S.v += (dv + S.dv) * mInv_2Fs;
    S.dv = dv;
    S.cv = tanhf(S.v);
}

void Filter::generate(Type         type,
                      float const *signal,
                      float const *cutoff,
                      float const *resonance,
                      float const *drive,
                      float       *out,
                      size_t      nsamp)
{
    switch (type) {

        case Off:
            generate_unfiltered(signal, out, nsamp);
            break;

        case LowPass:
            generate_low_pass(signal, cutoff, resonance, drive, out, nsamp);
            break;

        case HighPass:
            generate_high_pass(signal, cutoff, resonance, drive, out, nsamp);
            break;

        case BandPass:
            generate_band_pass(signal, cutoff, resonance, drive, out, nsamp);
            break;

        case BandReject:
            generate_band_reject(signal, cutoff, resonance, drive, out, nsamp);
            break;
    }
}

void Filter::generate_unfiltered(float const *signal,
                                 float       *out,
                                 size_t      nsamp)
{
    for (size_t i = 0; i < nsamp; i++)
        out[i] = signal[i];
}

void Filter::generate_low_pass(float const *signal,
                               float const *cutoff,
                               float const *resonance,
                               float const *drive,
                               float       *out,
                               size_t      nsamp)
{
    for (size_t i = 0; i < nsamp; i++) {
        float Fs       = mSampleRate;
        float Fc       = clamp(20, 20000, cutoff[i]);
        float k        = clamp(0, 4,resonance[i]);
        assert(k >= 0);
        float d        = clamp(-24, +12, drive[i]);
        float gain     = powf(10, d / 20);
        float inv_gain = gain < 0.1 ? 0.1 / gain : 1;
        float x        = M_PI * Fc / Fs;
        float A        = x * (1 - x) / (1 + x);
        float pc_2AFs  = 2 * A * Fs;

        float in = signal[i] * gain;
        step_stage(S1, tanhf((in + k * S4.v) * ninv_2Vt), pc_2AFs);
        step_stage(S2, S1.cv, pc_2AFs);
        step_stage(S3, S2.cv, pc_2AFs);
        step_stage(S4, S3.cv, pc_2AFs);
        out[i] = S4.v * inv_gain;
    }
}

void Filter::generate_high_pass(float const *signal,
                                float const *cutoff,
                                float const *resonance,
                                float const *drive,
                                float       *out,
                                size_t      nsamp)
{
    float a0 = 1;
    float a1 = -4;
    float a2 = 6;
    float a3 = -4;
    float a4 = 1;
    for (size_t i = 0; i < nsamp; i++) {
        float Fs       = mSampleRate;
        float Fc       = clamp(20, 20000, cutoff[i]);
        float k        = clamp(0, 4,resonance[i]);
        assert(k >= 0);
        float d        = clamp(-24, +12, drive[i]);
        float gain     = powf(10, d / 20);
        float inv_gain = gain < 0.1 ? 0.05 / gain : 0.5;
        float x        = M_PI * Fc / Fs;
        float A        = x * (1 - x) / (1 + x);
        float pc_2AFs  = 2 * A * Fs;

        float in = signal[i] * gain;
        step_stage(S1, tanhf((in + k * S4.v) * ninv_2Vt), pc_2AFs);
        step_stage(S2, S1.cv, pc_2AFs);
        step_stage(S3, S2.cv, pc_2AFs);
        step_stage(S4, S3.cv, pc_2AFs);
        out[i] = inv_gain * (a0 * in +
                             a1 * S1.cv +
                             a2 * S2.cv +
                             a3 * S3.cv +
                             a4 * S4.cv);
    }
}

void Filter::generate_band_pass(float const *signal,
                                float const *cutoff,
                                float const *resonance,
                                float const *drive,
                                float       *out,
                                size_t      nsamp)
{
    for (size_t i = 0; i < nsamp; i++)
        out[i] = signal[i];
}

void Filter::generate_band_reject(float const *signal,
                                  float const *cutoff,
                                  float const *resonance,
                                  float const *drive,
                                  float       *out,
                                  size_t      nsamp)
{
    for (size_t i = 0; i < nsamp; i++)
        out[i] = signal[i];
}
