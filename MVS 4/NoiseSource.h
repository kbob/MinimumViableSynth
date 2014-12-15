//
//  NoiseSource.h
//  MVS 4
//
//  Created by Bob Miller on 12/3/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS_4__NoiseSource__
#define __MVS_4__NoiseSource__

class NoiseSource {

public:

    enum Type {
        White,
        Pink,
        Red
    };

           NoiseSource();

    void   initialize(double sampleRate);
    void   generate(Type type, float *out, size_t n);

private:

    // White state
    uint32_t seed;

    // Pink state
    enum { Nf = 5 };
    float  pink_xm1[Nf];
    float  pink_ym1[Nf];
    float  pink_a0[Nf], pink_a1[Nf], pink_b1[Nf];
    float  pink_gain;

    // Red state (e.g., Alabama)
    float  red_ym1;
    float  red_gain;
    float  red_a0, red_b1;

    inline float    frandom();
    inline uint32_t cartaRandom();

};

#endif /* defined(__MVS_4__NoiseSource__) */
