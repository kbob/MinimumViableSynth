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
    enum OscillatorType {
        OT_Sine,
        OT_Saw,
        OT_Triangle,
        OT_Pulse,
    };

    Oscillator();

    void initialize(Float64        sampleRate,
                    OscillatorType type = OT_Saw);

    void generate(Float64 freq, Float32 *sampBuf, UInt32 count);
    void generate_modulated(Float32 *sampBuf, Float64 *freqs, UInt32 count);

private:
    Float64        mSampleRate;
    OscillatorType mType;

    Float64        mInverseSampleRate;

    Float64        mPhase;
    
};

#endif /* defined(__MVS__Oscillator__) */
