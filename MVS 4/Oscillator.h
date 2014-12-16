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
        None,
        Saw,
        Square,
        Triangle,
        Sine,
    };

    Oscillator();

    void   initialize                  (double       sampleRate);

    void   generate                    (Type         type,
                                        double       freq,
                                        float        modifier,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated          (Type         type,
                                        float const *phaseIncrements,
                                        float        modifier,
                                        float       *samples_out,
                                        size_t       count);

    // sync_out and sync_in point to an array of waveform start times
    // in units of samples.  They are float, so fractional sample times
    // can be used.  The syncs are terminated by a number higher
    // than count.
    void   generate_with_sync          (Type         type,
                                        float const *phaseIncrements,
                                        float        modifier,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t       count);

private:
    double mSampleRate;
    double mInverseSampleRate;
    Type   mType;
    float  mPhase;
    float  mNewThresh;
    float  mThresh;
    float  mShiftZ[4];

    void   generate_sync_only          (float const *phaseIncrements,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t      count);

    void   generate_saw                (double       freq,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated_saw      (float const *phaseIncrements,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_sync_saw           (float const *phaseIncrements,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t      count);

    void   generate_square             (double       freq,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated_square   (float const *phaseIncrements,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_sync_square        (float const *phaseIncrements,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t      count);

    void   generate_triangle           (double       freq,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated_triangle (float const *phaseIncrements,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_sync_triangle      (float const *phaseIncrements,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t      count);

    void   generate_sine               (double       freq,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated_sine     (float const *phaseIncrements,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_sync_sine          (float const *phaseIncrements,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t      count);

    void   begin_chunk                 (Type         type,
                                        float        freq,
                                        float        modifier);

    void   calc_h_m                    (Type         type,
                                        float        freq,
                                        float        phase,
                                        float        thresh,
                                        float       *h_out,
                                        float       *m_out);
    
};

#endif /* defined(__MVS__Oscillator__) */
