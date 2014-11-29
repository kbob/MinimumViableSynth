//
//  Envelope.h
//  MVS - Minimum Viable Synth
//
//  Created by Bob Miller on 11/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS__Envelope__
#define __MVS__Envelope__

class Envelope {

public:
    enum EnvelopeType {
        ET_Linear,
        ET_Exponential
    };

    Envelope();

    void    initialize(Float64      sampleRate,
                       Float32      maxLevel,
                       Float32      attackTime,
                       Float32      decayTime,
                       Float32      sustainLevel,
                       Float32      releaseTime,
                       EnvelopeType type = ET_Linear);

    void    release();

    Float32 amplitude() const { return mLevel; }

    UInt32  generate(float *sampBuf, UInt32 count);

private:
    enum EnvelopeSegment {
        ES_Attack,
        ES_Decay,
        ES_Sustain,
        ES_Release
    };

    EnvelopeType    mType;
    Float32         mMaxLevel;
    UInt32          mAttackSamples;
    UInt32          mDecaySamples;
    Float32         mSustainLevel;

    Float32         mAttackDelta;
    Float32         mDecayDelta;
    Float32         mReleaseDelta;

    EnvelopeSegment mSegment;
    UInt32          mSamplesDone;
    Float32         mLevel;

};


#endif /* defined(__MVS__Envelope__) */
