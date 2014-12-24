//
//  LFO.cpp
//  MVS 4
//
//  Created by Bob Miller on 12/20/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "LFO.h"

#include "Random.h"

LFO::LFO()
{}

void LFO::initialize(double sample_rate)
{
    mSampleRate = sample_rate;
    mInverseSampleRate = 1 / sample_rate;
    mPhase = 0;
    mRand0 = 0;
    mRand1 = 0;
}

void LFO::generate(Waveform     waveform,
                   Polarity     polarity,
                   float const *freq,
                   float const *depth,
                   float       *samples_out,
                   size_t       count)
{
#define JOIN(w, p) ((w) << 1 | (p))

    switch (JOIN(waveform, polarity)) {

        case JOIN(None, Unipolar):
        case JOIN(None, Bipolar):
            generate_none(freq, depth, samples_out, count);
            break;

        case JOIN(Triangle, Unipolar):
            generate_unipolar_triangle(freq, depth, samples_out, count);
            break;

        case JOIN(Triangle, Bipolar):
            generate_bipolar_triangle(freq, depth, samples_out, count);
            break;

        case JOIN(UpSaw, Unipolar):
            generate_unipolar_upsaw(freq, depth, samples_out, count);
            break;

        case JOIN(UpSaw, Bipolar):
            generate_bipolar_upsaw(freq, depth, samples_out, count);
            break;

        case JOIN(DnSaw, Unipolar):
            generate_unipolar_dnsaw(freq, depth, samples_out, count);
            break;

        case JOIN(DnSaw, Bipolar):
            generate_bipolar_dnsaw(freq, depth, samples_out, count);
            break;

        case JOIN(Square, Unipolar):
            generate_unipolar_square(freq, depth, samples_out, count);
            break;

        case JOIN(Square, Bipolar):
            generate_bipolar_square(freq, depth, samples_out, count);
            break;

        case JOIN(Random, Unipolar):
            generate_unipolar_random(freq, depth, samples_out, count);
            break;

        case JOIN(Random, Bipolar):
            generate_bipolar_random(freq, depth, samples_out, count);
            break;

        case JOIN(SampleHold, Unipolar):
            generate_unipolar_samphold(freq, depth, samples_out, count);
            break;

        case JOIN(SampleHold, Bipolar):
            generate_bipolar_samphold(freq, depth, samples_out, count);
            break;
    }
#undef JOIN
}

void LFO::generate_none(float const *freq,
                        float const *depth,
                        float       *samples_out,
                        size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        samples_out[i] = 0;
    }
    mPhase = phase;
}

void LFO::generate_unipolar_triangle(float const *freq,
                                     float const *depth,
                                     float       *samples_out,
                                     size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        float h = (phase < 0.5) ? 2 * phase : 2 * (1 - phase);
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
}

void LFO::generate_bipolar_triangle(float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        float h = ((phase < 0.5) ? 4 * phase : 4 * (1 - phase)) - 1;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
}

void LFO::generate_unipolar_upsaw(float const *freq,
                                  float const *depth,
                                  float       *samples_out,
                                  size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        float h = phase;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
}

void LFO::generate_bipolar_upsaw(float const *freq,
                                 float const *depth,
                                 float       *samples_out,
                                 size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        float h = 2 * phase - 1;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
}

void LFO::generate_unipolar_dnsaw(float const *freq,
                                  float const *depth,
                                  float       *samples_out,
                                  size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        float h = 1 - phase;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
}

void LFO::generate_bipolar_dnsaw(float const *freq,
                                 float const *depth,
                                 float       *samples_out,
                                 size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        float h = 1 - 2 * phase;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
}

void LFO::generate_unipolar_square(float const *freq,
                                   float const *depth,
                                   float       *samples_out,
                                   size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        float h = phase < 0.5;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
}

void LFO::generate_bipolar_square(float const *freq,
                                  float const *depth,
                                  float       *samples_out,
                                  size_t       count)
{
    float phase = mPhase;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1)
            phase -= 1;
        float h = phase < 0.5 ? +1 : -1;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
}

void LFO::generate_unipolar_random(float const *freq,
                                   float const *depth,
                                   float       *samples_out,
                                   size_t       count)
{
    float phase = mPhase;
    float rand0 = mRand0;
    float rand1 = mRand1;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1) {
            phase -= 1;
            rand0 = rand1;
            rand1 = frandom();
        }
        float h = (phase * rand1 + (1 - phase) * rand0 + 1) / 2;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
    mRand0 = rand0;
    mRand1 = rand1;
}

void LFO::generate_bipolar_random(float const *freq,
                                  float const *depth,
                                  float       *samples_out,
                                  size_t       count)
{
    float phase = mPhase;
    float rand0 = mRand0;
    float rand1 = mRand1;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1) {
            phase -= 1;
            rand0 = rand1;
            rand1 = frandom();
        }
        float h = phase * rand1 + (1 - phase) * rand0;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
    mRand0 = rand0;
    mRand1 = rand1;
}

void LFO::generate_unipolar_samphold(float const *freq,
                                     float const *depth,
                                     float       *samples_out,
                                     size_t       count)
{
    float phase = mPhase;
    float rand0 = mRand0;
    float rand1 = mRand1;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1) {
            phase -= 1;
            rand0 = rand1;
            rand1 = frandom();
        }
        float h = (rand0 + 1) / 2;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
    mRand0 = rand0;
    mRand1 = rand1;
}

void LFO::generate_bipolar_samphold(float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count)
{
    float phase = mPhase;
    float rand0 = mRand0;
    float rand1 = mRand1;
    const float inv_sample_rate = mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += freq[i] * inv_sample_rate;
        if (phase >= 1) {
            phase -= 1;
            rand0 = rand1;
            rand1 = frandom();
        }
        float h = rand0;
        samples_out[i] = depth[i] * h;
    }
    mPhase = phase;
    mRand0 = rand0;
    mRand1 = rand1;
}
