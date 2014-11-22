//
//  Oscillator.h
//  MVS
//
//  Created by Bob Miller on 11/17/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS__Oscillator__
#define __MVS__Oscillator__

class Oscillator {

public:
    enum Type {
        OT_Sine,
        OT_Saw,
        OT_Pulse,
    };

    Oscillator();

    void initialize(Float64 sampleRate,
                    Type    type = OT_Saw);

    void generate(Float64  freq,
                  Float32  modifier,
                  Float32 *sampBuf,
                  UInt32   count);
    void generate_modulated(Float32  modifier,
                            Float32 *sampBuf,
                            Float64 *freqs,
                            UInt32   count);

private:
    Float64 mSampleRate;
    Type    mType;

    Float64 mInverseSampleRate;

    Float64 mPhase;
    
};

#endif /* defined(__MVS__Oscillator__) */
