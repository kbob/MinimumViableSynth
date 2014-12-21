//
//  LFO.h
//  MVS - Low Frequency Oscillator
//
//  Created by Bob Miller on 12/20/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS__LFO__
#define __MVS__LFO__

class LFO {

public:

    enum Waveform {
        None,
        Triangle,
        UpSaw,
        DnSaw,
        Square,
        Random,
        SampleHold,
    };

    enum Polarity {
        Unipolar,
        Bipolar,
    };

    LFO();
    void initialize(double sample_rate);

    void generate(Waveform     waveform,
                  Polarity     polarity,
                  float const *freq,
                  float const *depth,
                  float       *samples_out,
                  size_t       count);

    // simplified version.
    void generate(Waveform     waveform,
                  float const *freq,
                  float       *samples_out,
                  size_t       count);

private:

    double   mSampleRate;
    double   mInverseSampleRate;
    // Waveform mWaveform;
    // Polarity mPolarity;
    float    mPhase;
    float    mRand0;
    float    mRand1;

    void generate_none(float const *freq,
                       float const *depth,
                       float       *samples_out,
                       size_t       count);

    void generate_unipolar_triangle(float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_bipolar_triangle (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_unipolar_upsaw   (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_bipolar_upsaw    (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_unipolar_dnsaw   (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_bipolar_dnsaw    (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_unipolar_square  (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_bipolar_square   (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_unipolar_random  (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_bipolar_random   (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_unipolar_samphold(float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);

    void generate_bipolar_samphold (float const *freq,
                                    float const *depth,
                                    float       *samples_out,
                                    size_t       count);
    
};

#endif /* defined(__MVS__LFO__) */
