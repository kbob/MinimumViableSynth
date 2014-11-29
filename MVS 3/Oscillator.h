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
        Saw,
        Square,
        Triangle,
        Sine,
    };

            Oscillator();

    void    initialize                  (Float64        sampleRate,
                                         Type           type = Saw);

    void    generate                    (Float64        freq,
                                         Float32        modifier,
                                         Float32       *sampBuf,
                                         UInt32         count);

    void    generate_modulated          (Float32        modifier,
                                         Float32       *sampBuf,
                                         Float32 const *phaseIncrements,
                                         UInt32         count);

private:
    Float64 mSampleRate;
    Float64 mInverseSampleRate;
    Type    mType;
    Float32 mPhase;
    Float32 mZm2, mZm1, mZ0, mZp1;
    Float32 mLevel;

    void    generate_saw                (Float64        freq,
                                         Float32        modifier,
                                         Float32       *sampBuf,
                                         UInt32         count);

    void    generate_modulated_saw      (Float32        modifier,
                                         Float32       *sampBuf,
                                         Float32 const *phaseIncrements,
                                         UInt32         count);

    void    generate_square             (Float64        freq,
                                         Float32        modifier,
                                         Float32       *sampBuf,
                                         UInt32         count);

    void    generate_modulated_square   (Float32        modifier,
                                         Float32       *sampBuf,
                                         Float32 const *phaseIncrements,
                                         UInt32         count);

    void    generate_triangle           (Float64        freq,
                                         Float32        modifier,
                                         Float32       *sampBuf,
                                         UInt32         count);

    void    generate_modulated_triangle (Float32        modifier,
                                         Float32       *sampBuf,
                                         Float32 const *phaseIncrements,
                                         UInt32         count);

    void    generate_sine               (Float64        freq,
                                         Float32        modifier,
                                         Float32       *sampBuf,
                                         UInt32         count);

    void    generate_modulated_sine     (Float32        modifier,
                                         Float32       *sampBuf,
                                         Float32 const *phaseIncrements,
                                         UInt32         count);
    
};

#endif /* defined(__MVS__Oscillator__) */
