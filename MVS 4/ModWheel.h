//
//  ModWheel.h
//  MVS 4
//
//  Created by Bob Miller on 12/20/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS_4__ModWheel__
#define __MVS_4__ModWheel__

// Fill a buffer with smoothed Modulation Wheel values.

class ModWheel {

public:

             ModWheel();
    void     initialize(double sample_rate);

    void     set_raw_MSB(uint8_t value);
    void     set_raw_LSB(uint8_t value);

    void     generate(float *samples_out, size_t count);
    void     generate_scaled(float scale, float *samples_out, size_t count);

private:

    float    mSampleRate;
    float    mDecay;
    uint16_t mIntTarget;
    float    mTarget;
    float    mActual;
    bool     mLSBSeen;

};

#endif /* defined(__MVS_4__ModWheel__) */
