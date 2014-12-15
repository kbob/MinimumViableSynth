//
//  Envelope.cpp
//  MVS
//
//  Created by Bob Miller on 11/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Envelope.h"

#include <cmath>

static const Float32 kInaudibleLevel = 0.0001;

Envelope::Envelope()
{}

static Float32 expDelta(Float32 start, Float32 end, size_t samples)
{
    if (!samples)
        return start < end ? 1.0 : 0.0;

    if (start < kInaudibleLevel)
        start = kInaudibleLevel;
    if (end < kInaudibleLevel)
        end = kInaudibleLevel;
    return 1.0 + (log(end) - log(start)) / samples;
}

void Envelope::initialize(Float64      sampleRate,
                          float        maxLevel,
                          float        attackTime,
                          float        decayTime,
                          float        sustainLevel,
                          float        releaseTime,
                          EnvelopeType type)
{
    mType          = type;
    mMaxLevel      = maxLevel;
    mAttackSamples = attackTime * sampleRate;
    mDecaySamples  = decayTime * sampleRate;
    mSustainLevel  = maxLevel * sustainLevel;
    mSegment       = ES_Attack;
    mSamplesDone   = 0;
    mLevel         = 0;
    switch (type) {

        case Linear:
            mAttackDelta  = maxLevel / mAttackSamples;
            mDecayDelta   = -maxLevel * (1.0 - sustainLevel) / mDecaySamples;
            mReleaseDelta = -maxLevel / (releaseTime * sampleRate);
            mLevel = 0;
            break;

        case Exponential:
            mAttackDelta = expDelta(0, maxLevel, mAttackSamples);
            mDecayDelta = expDelta(1.0001, sustainLevel, mDecaySamples);
            mReleaseDelta = expDelta(maxLevel, 0, releaseTime * sampleRate);
            mLevel = kInaudibleLevel;
            break;
    }
}

void Envelope::release()
{
    mSegment = ES_Release;
}

UInt32 Envelope::generate(float *sampBuf, UInt32 count)
{
    UInt32  i = 0, n;
    UInt32  endFrame = 0xFFFFFFFF;
    UInt32  samplesDone = mSamplesDone;
    Float32 level = mLevel;
    Float32 delta;

    while (i < count) {
        switch (mSegment) {

        case ES_Attack:
            delta = mAttackDelta;
            n = mAttackSamples - samplesDone;
            if (n > count)
                n = count;
            else
                mSegment = ES_Decay;
            if (mType == Exponential) {
                for ( ; i < n; i++) {
                    level *= delta;
                    sampBuf[i] = level;
                }
            } else { // mType == Linear
                for ( ; i < n; i++) {
                    level += delta;
                    sampBuf[i] = level;
                }
            }
            samplesDone += n;
            if (mSegment == ES_Decay)
                level = mMaxLevel;
            continue;

        case ES_Decay:
            delta = mDecayDelta;
            n = mAttackSamples + mDecaySamples - samplesDone;
            if (n > count)
                n = count;
            else
                mSegment = ES_Sustain;
            if (mType == Exponential) {
                for ( ; i < n; i++) {
                    level *= delta;
                    sampBuf[i] = level;
                }
            } else { // mType == Linear
                for ( ; i < n; i++) {
                    level += delta;
                    sampBuf[i] = level;
                }
            }
            samplesDone += n;
            continue;

        case ES_Sustain:
            level = mSustainLevel;
            for ( ; i < count; i++)
                sampBuf[i] = level;
            continue;

        case ES_Release:
            delta = mReleaseDelta;
            if (mType == Exponential) {
                for ( ; i < count; i++) {
                    level *= delta;
                    if (level < 1.0 / (1 << 16)) {
                        level = 0;
                        endFrame = i;
                        break;
                    }
                    sampBuf[i] = level;
                }
            } else { // mType == Linear
                for ( ; i < count; i++) {
                    level += delta;
                    if (level < 0) {
                        level = 0;
                        endFrame = i;
                        break;
                    }
                    sampBuf[i] = level;
                }
            }
            for ( ; i < count; i++)
                sampBuf[i] = 0;
            samplesDone += count;
            break;
        }
    }
    mSamplesDone = samplesDone;
    mLevel = level;
    return endFrame;
}
