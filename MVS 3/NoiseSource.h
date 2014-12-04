//
//  NoiseSource.h
//  MVS 3
//
//  Created by Bob Miller on 12/3/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS_3__NoiseSource__
#define __MVS_3__NoiseSource__

class NoiseSource {

public:

    enum Type {
        White,
        Pink,
        Red
    };

    NoiseSource();

    void     initialize(Float64 sampleRate);
    void     generate(Type type, Float32 *out, size_t n);

private:
    Float64  mSampleRate;
    Float32  mLeakage;
    Float32  mScaling;
    uint32_t mSeed;
    Float32  mB0, mB1, mB2, mB3, mB4, mB5, mB6;
    Float32  mY;

    inline Float32  frandom();
    inline uint32_t cartaRandom();
    
};

#endif /* defined(__MVS_3__NoiseSource__) */
