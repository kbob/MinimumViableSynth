//
//  Filter.h
//  MVS 4
//
//  Created by Bob Miller on 1/1/15.
//  Copyright (c) 2015 kbobsoft.com. All rights reserved.
//

#ifndef __MVS_4__Filter__
#define __MVS_4__Filter__

class Filter {

public:

    enum Type {
        Off,
        LowPass,
        HighPass,
        BandPass,
        BandReject
    };

    Filter();

    void  initialize           (float sample_rate);

    void generate              (Type         type,
                                float const *signal,
                                float const *cutoff,
                                float const *resonance,
                                float const *drive,
                                float       *out,
                                size_t      nsamp);

private:

    struct LadderStage {
        float v;                // V - voltage
        float dv;               // dV/dt
        float cv;               // clipped V

        LadderStage()
        : v(0), dv(0), cv(0)
        {}
    };

    float mSampleRate;
    float mInv_2Fs;
    LadderStage S1, S2, S3, S4;

    void  generate_unfiltered  (float const *signal,
                                float       *out,
                                size_t      nsamp);
    void  generate_low_pass    (float const *signal,
                                float const *cutoff,
                                float const *resonance,
                                float const *drive,
                                float       *out,
                                size_t      nsamp);
    void  generate_high_pass   (float const *signal,
                                float const *cutoff,
                                float const *resonance,
                                float const *drive,
                                float       *out,
                                size_t      nsamp);
    void  generate_band_pass   (float const *signal,
                                float const *cutoff,
                                float const *resonance,
                                float const *drive,
                                float       *out,
                                size_t      nsamp);
    void  generate_band_reject (float const *signal,
                                float const *cutoff,
                                float const *resonance,
                                float const *drive,
                                float       *out,
                                size_t      nsamp);

    inline void step_stage     (LadderStage& S,
                                float in,
                                float pc_2AFs);
    
};

#endif /* defined(__MVS_4__Filter__) */
