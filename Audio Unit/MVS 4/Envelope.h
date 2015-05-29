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

    enum Type {
        Linear,
        Exponential
    };

    Envelope            ();
    void    initialize          (float        sample_rate);

    float   amplitude           () const;

    void    release             ();

    size_t  generate            (Type         type,
                                 float const *attack,
                                 float const *decay,
                                 float const *sustain,
                                 float const *release,
                                 float const *amount,
                                 float       *out,
                                 size_t       count);

private:

    enum Segment {
        Attack,
        Decay,
        Sustain,
        Release,
        Done
    };

    float   mSampleRate;
    float   mInverseSampleRate;
    Segment mSegment;
    size_t  mSamplesDone;
    float   mLevel;
    float   mAmplitude;
    float   mAttackDuration;
    float   mReleaseTime;

    size_t  generate_linear     (float const *attack,
                                 float const *decay,
                                 float const *sustain,
                                 float const *release,
                                 float const *amount,
                                 float       *out,
                                 size_t       count);

    size_t generate_exponential (float const *attack,
                                 float const *decay,
                                 float const *sustain,
                                 float const *release,
                                 float const *amount,
                                 float       *out,
                                 size_t       count);
    
};

#endif /* defined(__MVS__Envelope__) */
