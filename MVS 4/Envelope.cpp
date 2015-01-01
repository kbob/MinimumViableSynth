//
//  Envelope.cpp
//  MVS
//
//  Created by Bob Miller on 11/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Envelope.h"

static const float kInaudibleLevel = 0.0001;
static const float kReallyInaudibleLevel = 1.0 / (1 << 16);

static inline float clamp(float y0, float y1, float y)
{
    if (y < y0)
        return y0;
    if (y > y1)
        return y1;
    return y;
}

Envelope::Envelope()
: mLevel(0),
mAmplitude(0)
{}

void Envelope::initialize(float sample_rate)
{
    mSampleRate = sample_rate;
    mInverseSampleRate = 1 / sample_rate;
    mSegment = Attack;
    mSamplesDone = 0;
    mLevel = 0;
    mAmplitude = 0;
    mAttackDuration = 0;
    mReleaseTime = 0;
}

float Envelope::amplitude() const
{
    return mAmplitude;
}

void Envelope::release()
{
    mSegment = Release;
    mReleaseTime = mSamplesDone * mInverseSampleRate;
//    printf("Release at %zu = %g\n",
//           mSamplesDone, mReleaseTime);
}

size_t Envelope::generate(Type         type,
                          float const *attack,
                          float const *decay,
                          float const *sustain,
                          float const *release,
                          float const *amount,
                          float       *out,
                          size_t       count)
{
    switch (type) {

        case Linear:
            return generate_linear(attack,
                                   decay,
                                   sustain,
                                   release,
                                   amount,
                                   out,
                                   count);

        case Exponential:
            return generate_exponential(attack,
                                        decay,
                                        sustain,
                                        release,
                                        amount,
                                        out,
                                        count);
    }
}

size_t Envelope::generate_linear(float const *attack,
                                 float const *decay,
                                 float const *sustain,
                                 float const *release,
                                 float const *amount,
                                 float       *out,
                                 size_t       count)
{
    float  level        = mLevel;
    float  amplitude    = mAmplitude;
    size_t samples_done = mSamplesDone;
    size_t end_frame    = SIZE_MAX;

    for (size_t i = 0; i < count; ) {
        switch (mSegment) {

            case Attack:
                for ( ; i < count; i++) {
                    float t = (samples_done + i) * mInverseSampleRate;
                    float a = attack[i];
                    if (t > a) {
                        mSegment = Decay;
                        mAttackDuration = t;
                        break;
                    }
                    level = t / a;
                    amplitude = level * amount[i];
                    out[i] = amplitude;
                }
                /* FALLTHRU */    // Remember lint?

            case Decay:
                for ( ; i < count; i++) {
                    float t = (samples_done + i) * mInverseSampleRate
                    - mAttackDuration;
                    float d = decay[i];
                    if (t > d) {
                        mSegment = Sustain;
                        break;
                    }
                    float s = clamp(0, 1, sustain[i]);
                    level = 1 - (1 - s) * t / d;
                    amplitude = level * amount[i];
                    out[i] = amplitude;
                }
                /* FALLTHRU */

            case Sustain:
                for ( ; i < count; i++) {
                    level = clamp(0, 1, sustain[i]);
                    amplitude = level * amount[i];
                    out[i] = amplitude;
                }
                break;

            case Release:
                for ( ; i < count && mSegment == Release; i++) {
                    float r = release[i];
                    level = r ? level - mInverseSampleRate / r : 0;
                    // printf("%zu: r = %g, level = %g\n",
                    //        samples_done + i, r, level);
                    if (level < 0) {
                        level = 0;
                        mSegment = Done;
                        end_frame = i;
                    }
                    amplitude = level * amount[i];
                    out[i] = amplitude;
                }
                break;

            case Done:
                end_frame = i;
                for ( ; i < count; i++)
                    out[i] = level = 0;
                break;
        }
    }
    mSamplesDone += count;
    mLevel = level;
    mAmplitude = amplitude;
    return end_frame;
}

size_t Envelope::generate_exponential(float const *attack,
                                      float const *decay,
                                      float const *sustain,
                                      float const *release,
                                      float const *amount,
                                      float       *out,
                                      size_t       count)
{
    float const scale = -logf(kInaudibleLevel) * mInverseSampleRate;
    float       level        = mLevel;
    float       amplitude    = mAmplitude;
    size_t      samples_done = mSamplesDone;
    size_t      end_frame    = SIZE_MAX;

    for (size_t i = 0; i < count; ) {
        switch (mSegment) {

            case Attack:
                if (samples_done == 0)
                    level = kInaudibleLevel;
                for ( ; i < count; i++) {
                    float a = attack[i];
                    if (a)
                        level *= 1 + scale / a;
                    else
                        level = 1;
                    if (level >= 1) {
                        level = 1;
                        mSegment = Decay;
                        mAttackDuration = a;
                        // printf("Attack done at %zu = %g\n",
                        //        samples_done + i,
                        //        (samples_done + i) * mInverseSampleRate);
                        break;
                    }
                    amplitude = level * amount[i];
                    out[i] = amplitude;
                }
                /* FALLTHRU */    // Remember lint?

            case Decay:
                for ( ; i < count; i++) {
                    float d = decay[i];
                    float s = clamp(0, 1, sustain[i]);
                    if (d) {
                        float delta = 1 - scale / d;
                        level = s + (level - s) * delta;
                    } else
                        level = s;
                    if (level <= s * 1.0001) {
                        level = s;
                        mSegment = Sustain;
                        // printf("Decay done at %zu = %g\n",
                        //        samples_done + i,
                        //        (samples_done + i) * mInverseSampleRate);
                        break;
                    }
                    amplitude = level * amount[i];
                    out[i] = amplitude;
                }
                /* FALLTHRU */

            case Sustain:
                for ( ; i < count; i++) {
                    level = clamp(0, 1, sustain[i]);
                    amplitude = level * amount[i];
                    out[i] = amplitude;
                }
                break;

            case Release:
                for ( ; i < count && mSegment == Release; i++) {
                    float r = release[i];
                    if (r)
                        level *= 1 - scale / r;
                    else
                        level = 0;
                    if (level < kReallyInaudibleLevel) {
                        level = 0;
                        mSegment = Done;
                        end_frame = i;
                        // printf("Release done at %zu = %g\n",
                        //        samples_done + i,
                        //        (samples_done + i) * mInverseSampleRate);
                    }
                    amplitude = level * amount[i];
                    out[i] = amplitude;
                }
                break;
                
            case Done:
                end_frame = i;
                for ( ; i < count; i++)
                    out[i] = level = 0;
                break;
        }
    }
    mSamplesDone += count;
    mLevel = level;
    mAmplitude = amplitude;
    return end_frame;
}
