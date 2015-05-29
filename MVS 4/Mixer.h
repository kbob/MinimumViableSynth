//
//  Mixer.h
//  MVS 4
//
//  Created by Bob Miller on 12/24/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS_4__Mixer__
#define __MVS_4__Mixer__

class Mixer {

public:

    enum Operator {
        Mix,
        RingMod,
        HardSync
    };

    Mixer();
    void initialize(float sample_rate);

    void generate         (Operator     op,
                           float const *src1, float const *src1_level,
                           float const *src2, float const *src2_level,
                           float const *src3, float const *src3_level,
                           float       *out,
                           size_t       nsamp);

private:

    void generate_mix     (float const *src1, float const *src1_level,
                           float const *src2, float const *src2_level,
                           float const *src3, float const *src3_level,
                           float       *out,
                           size_t       nsamp);

    void generate_ringmod (float const *src1, float const *src1_level,
                           float const *src2, float const *src2_level,
                           float const *src3, float const *src3_level,
                           float       *out,
                           size_t       nsamp);

};

#endif /* defined(__MVS_4__Mixer__) */
