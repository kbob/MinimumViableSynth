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

    // Oscillator has four waveforms.
    //
    // It can continuously modulate frequency, and the square and
    // triangle waveform can continuously modulate pulse width.
    //
    // Can generate sync pulses and can hard sync to another
    // oscillator's sync pulses.
    //

    enum Waveform {
        None,
        Saw,
        Square,
        Triangle,
        Sine,
    };

           Oscillator();

    void   initialize                  (float        sample_rate);

    // All *freq* arguments are expressed as a fraction of the
    // sample rate.
    void   generate                    (Waveform     waveform,
                                        float        freq,
                                        float        width,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated          (Waveform     waveform,
                                        float const *freqs,
                                        float const *widths,
                                        float       *samples_out,
                                        size_t       count);

    // sync_out and sync_in point to an array of waveform start times
    // in units of samples.  They are float, so fractional sample times
    // can be used.  The syncs are terminated by a number higher
    // than count.
    void   generate_with_sync          (Waveform     waveform,
                                        float const *freqs,
                                        float const *widths,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t       count);

private:

    Waveform mWaveform;
    bool   mWaveformChanged;
    float  mPhase;
    float  mThresh;             // phase of last PWM transition
    float  mShiftZ[4];

    void   begin_chunk                 (Waveform     waveform,
                                        float        freq,
                                        float        width0);

    void   calc_h_m                    (Waveform     waveform,
                                        float        freq,
                                        float        phase,
                                        float        thresh,
                                        float       *h_out,
                                        float       *m_out);

    // Generators

    void   generate_saw                (float        freq,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_square             (float        freq,
                                        float        width,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_triangle           (float        freq,
                                        float        width,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_sine               (float        freq,
                                        float       *samples_out,
                                        size_t       count);

    // Modulated Generators

    void   generate_modulated_saw      (float const *freqs,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated_square   (float const *freqs,
                                        float const *widths,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated_triangle (float const *freqs,
                                        float const *widths,
                                        float       *samples_out,
                                        size_t       count);

    void   generate_modulated_sine     (float const *freqs,
                                        float       *samples_out,
                                        size_t       count);

    // Synchronized Generators

    void   generate_sync_only          (float const *freqs,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t       count);

    void   generate_sync_saw           (float const *freqs,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t       count);

    void   generate_sync_square        (float const *freqs,
                                        float const *widths,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t       count);

    void   generate_sync_triangle      (float const *freqs,
                                        float const *widths,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t       count);

    void   generate_sync_sine          (float const *freqs,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t       count);

};

#endif /* defined(__MVS__Oscillator__) */
