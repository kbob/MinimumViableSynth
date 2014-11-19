//
//  Envelope.cpp
//  MVS
//
//  Created by Bob Miller on 11/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Envelope.h"

#include <cmath>

Envelope::Envelope()
{}

void Envelope::initialize(Float64      sampleRate,
                          float        attackTime,
                          float        decayTime,
                          float        sustainLevel,
                          float        releaseTime,
                          EnvelopeType type)
{
    mType          = type;
    mAttackSamples = attackTime * sampleRate;
    mDecaySamples  = decayTime * sampleRate;
    mSustainLevel  = sustainLevel;
    mAttackDelta   = 1.0 / mAttackSamples;
    mDecayDelta    = -(1.0 - mSustainLevel) / mDecaySamples;
    mReleaseDelta  = -mSustainLevel / (releaseTime * sampleRate);
    mSegment       = ES_Attack;
    mSamplesDone   = 0;
    mLevel         = 0;
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

    assert(mType == ET_Linear);

    while (i < count) {
        switch (mSegment) {

        case ES_Attack:
            delta = mAttackDelta;
            n = mAttackSamples - samplesDone;
            if (n > count)
                n = count;
            else
                mSegment = ES_Decay;
            if (mType == ET_Exponential) {
                // ???
            } else { // mType == ET_Linear
                for ( ; i < n; i++) {
                    level += delta;
                    sampBuf[i] = level;
                }
            }
            samplesDone += n;
            if (mSegment == ES_Decay)
                level = 1.0;
            continue;

        case ES_Decay:
            delta = mDecayDelta;
            n = mAttackSamples + mDecaySamples - samplesDone;
            if (n > count)
                n = count;
            else
                mSegment = ES_Sustain;
            if (mType == ET_Exponential) {
                // ???
            } else { // mType == ET_Linear
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
            if (mType == ET_Exponential) {
                // ???
            } else { // mType == ET_Linear
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
            break;
        }
    }
    mSamplesDone = samplesDone;
    mLevel = level;
    return endFrame;
}
