//
//  Oscillator.cpp
//  MVS
//
//  Created by Bob Miller on 11/17/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Oscillator.h"

// Stupid delay line.

// This delay line is stored in a member of the Oscillator object, but
// it is also cached in four local variables.  Between BEGIN_Z and
// END_Z, the local variables are authoritative; elsewhere the
// member variable is the only copy.
//
// I do it this way in the hope that the compiler can generate better
// code than if every access touched the object.

#define BEGIN_Z                                                         \
    {                                                                   \
        float zm2 = mShiftZ[0],                                         \
              zm1 = mShiftZ[1],                                         \
              z0  = mShiftZ[2],                                         \
              zp1 = mShiftZ[3];

#define END_Z                                                           \
    {                                                                   \
        mShiftZ[0] = zm2;                                               \
        mShiftZ[1] = zm1;                                               \
        mShiftZ[2] = z0;                                                \
        mShiftZ[3] = zp1;                                               \
    } }

// Clear the whole delay line.
#define CLEAR_Z() (mShiftZ[0] = mShiftZ[1] = mShiftZ[2] = mShiftZ[3] = 0)

// Access the oldest sample.
#define SHIFT_OUT() (zm2)

// Insert a new sample, shift others down.
#define SHIFT_IN(x) (zm2 = zm1, zm1 = z0 + (x), z0 = zp1, zp1 = 0)


// polyBLEP: polynomial approximation to Bandwidth-Limited stEP function
// polyBLAM: polynomial approximation to Bandwidth-Limited rAMp function
//
// Not really.  It's just fun to say BLEP and BLAM.
//
// BLEP_ME and BLAM_ME implicitly use the delay line, so only call
// them between Z_BEGIN and Z_END.

#define BLEP_ORDER 3
#define BLAM_ORDER 1

#if BLEP_ORDER == 3

    #define BLEP_ME(d, h)                                               \
        ((zm2 += (h) * (+ (d) * (d) * (d) * (d) / 24)),                 \
         (zm1 += (h) * (- (d) * (d) * (d) * (d) / 8                     \
                        + (d) * (d) * (d) / 6                           \
                        + (d) * (d) / 4                                 \
                        + (d) / 6                                       \
                        + 1 / 24.)),                                    \
         (z0  += (h) * (+ (d) * (d) * (d) * (d) / 8                     \
                        - (d) * (d) * (d) / 3                           \
                        + (d) * 2 / 3                                   \
                        - 1 / 2.)),                                     \
         (zp1 += (h) * (- (d) * (d) * (d) * (d) / 24                    \
                        + (d) * (d) * (d) / 6                           \
                        - (d) * (d) / 4                                 \
                        + (d) / 6                                       \
                        - 1 / 24.)))

#elif BLEP_ORDER == 1

    #define BLEP_ME(d, h)                                               \
          ((zm1 += (h) * (+ (d) * (d) / 2)),                            \
           (z0  += (h) * (- (d) * (d) / 2                               \
                          + (d)                                         \
                          - 1 / 2.)))
 
#elif BLEP_ORDER == 0

    #define BLEP_ME(d, h) ((void)0) /* La di da... */

#else

    #error "unknown BLEP order"

#endif

#if BLAM_ORDER == 1

    #define BLAM_ME(d, h)                                               \
        ((zm1 += (h) * (+ (d) * (d) * (d) / 6)),                        \
         (z0  += (h) * (+ (1-(d)) * (1-(d)) * (1-(d)) / 6)))

#elif BLAM_ORDER == 0

    #define BLAM_ME(d, h) ((void)0)

#else

    #error "unknown BLAM order"

#endif

Oscillator::Oscillator()
{}

void Oscillator::initialize(double sampleRate)
{
    mSampleRate = sampleRate;
    mInverseSampleRate = 1.0 / sampleRate;
    mWaveform = None;
    mPhase = 0.0;
    mNewThresh = 0.5;
    mThresh = -1;               // signal begin_chunk to initialize.

    CLEAR_Z();
}

const char *OTname(Oscillator::Waveform t)
{
    switch (t) {

        case Oscillator::None:
            return "None";

        case Oscillator::Saw:
            return "Saw";

        case Oscillator::Square:
            return "Square";

        case Oscillator::Triangle:
            return "Triangle";

        case Oscillator::Sine:
            return "Sine";
    }
}

void Oscillator::generate(Waveform waveform,
                          double   freq,
                          float    modifier,
                          float   *samples_out,
                          size_t   count)
{
    begin_chunk(waveform, freq, modifier);
    switch (waveform) {

        case None:
            break;

        case Saw:
            generate_saw(freq, samples_out, count);
            break;

        case Square:
            generate_square(freq, samples_out, count);
            break;

        case Triangle:
            generate_triangle(freq, samples_out, count);
            break;

        case Sine:
            generate_sine(freq, samples_out, count);
            break;
    }
    mThresh = mNewThresh;
}

void Oscillator::generate_modulated(Waveform     waveform,
                                    float const *phaseIncrements,
                                    float        modifier,
                                    float       *samples_out,
                                    size_t       count)
{
    float freq = phaseIncrements[0] * mSampleRate;
    begin_chunk(waveform, freq, modifier);
    switch (waveform) {

        case None:
            break;

        case Saw:
            generate_modulated_saw(phaseIncrements, samples_out, count);
            break;

        case Square:
            generate_modulated_square(phaseIncrements, samples_out, count);
            break;

        case Triangle:
            generate_modulated_triangle(phaseIncrements, samples_out, count);
            break;

        case Sine:
            generate_modulated_sine(phaseIncrements, samples_out, count);
            break;
    }
    mThresh = mNewThresh;
}

void Oscillator::generate_with_sync(Waveform     waveform,
                                    float const *phaseIncrements,
                                    float        modifier,
                                    float       *samples_out,
                                    float const *sync_in,
                                    float       *sync_out,
                                    size_t       count)
{
    float freq = phaseIncrements[0] * mSampleRate;
    begin_chunk(waveform, freq, modifier);
    switch (waveform) {

        case None:
            generate_sync_only     (phaseIncrements,
                                    samples_out,
                                    sync_in,
                                    sync_out,
                                    count);
            break;

        case Saw:
            generate_sync_saw      (phaseIncrements,
                                    samples_out,
                                    sync_in,
                                    sync_out,
                                    count);
            break;

        case Square:
            generate_sync_square   (phaseIncrements,
                                    samples_out,
                                    sync_in,
                                    sync_out,
                                    count);
            break;

        case Triangle:
            generate_sync_triangle (phaseIncrements,
                                    samples_out,
                                    sync_in,
                                    sync_out,
                                    count);
            break;

        case Sine:
            generate_sync_sine     (phaseIncrements,
                                    samples_out,
                                    sync_in,
                                    sync_out,
                                    count);
            break;
    }
    mThresh = mNewThresh;
}

void Oscillator::generate_sync_only(float const *phaseIncrements,
                                    float       *samples_out,
                                    float const *sync_in,
                                    float       *sync_out,
                                    size_t       count)
{
    BEGIN_Z;
    float phase = mPhase;
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];
    for (size_t i = 0; i < count; i++) {
        float inc = phaseIncrements[i];
        phase += inc;
        if (i == next_reset) {
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * inc * mInverseSampleRate;
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
            // BLEP/BLAM not needed.  We are silent.
        } else if (phase >= 1.0) {
            // negative discontinuity
            phase -= 1.0;
            float d = phase / inc;
            sync_out[soi++] = i - 1 + d;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(0);
    }
    sync_out[soi] = count + 1;
    assert(sync_in[sii] >= count);
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_saw(double freq,
                              float *samples_out,
                              size_t count)
{
    BEGIN_Z;
    float phase   = mPhase;
    float inc     = freq * mInverseSampleRate;
    float inv_inc = 1 / inc;
    for (size_t i = 0; i < count; i++) {
        phase += inc;
        if (phase >= 1.0) {
            // negative discontinuity
            phase -= 1.0;
            BLEP_ME(phase * inv_inc, -2.0);
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(2 * phase - 1);
    }
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_modulated_saw(float const *phaseIncrements,
                                        float       *samples_out,
                                        size_t       count)
{
    BEGIN_Z;
    float phase = mPhase;
    for (size_t i = 0; i < count; i++) {
        float inc = phaseIncrements[i];
        phase += inc;
        if (phase >= 1.0) {
            // negative discontinuity
            phase -= 1.0;
            BLEP_ME(phase / inc, -2.0);
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(2 * phase - 1);
    }
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_sync_saw(float const *phaseIncrements,
                                   float       *samples_out,
                                   float const *sync_in,
                                   float       *sync_out,
                                   size_t       count)
{
    BEGIN_Z;
    float phase = mPhase;
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];
    for (size_t i = 0; i < count; i++) {
        float inc = phaseIncrements[i];
        phase += inc;
        if (i == next_reset) {
            float h0 = 2 * phase - 1;
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * inc * mInverseSampleRate;
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
            BLEP_ME(d, -1 - h0);
            // BLAM not needed.  Slope is constant.
        } else if (phase >= 1.0) {
            // negative discontinuity
            phase -= 1.0;
            float d = phase / inc;
            BLEP_ME(d, -2.0);
            sync_out[soi++] = i - 1 + d;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(2 * phase - 1);
    }
    sync_out[soi] = count + 1;
    assert(sync_in[sii] >= count);
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_square(double freq,
                                 float *samples_out,
                                 size_t count)
{
    BEGIN_Z;
    float phase        = mPhase;
    float thresh       = mThresh;
    float new_thresh   = mNewThresh;
    float high         = 1.0;
    float low          = thresh / (thresh - 1);
    float level        = phase < thresh ? high : low;
    float fall_trigger = level > 0 ? thresh : 9999;
    float inc          = freq * mInverseSampleRate;
    float inv_inc      = 1 / inc;
    for (size_t i = 0; i < count; i++) {
        phase += inc;
        if (phase >= 1.0) {
            // positive discontinuity
            phase -= 1.0;
            level = high;
            BLEP_ME(phase * inv_inc, high - low);
            thresh = new_thresh;
            low = thresh / (thresh - 1);
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // negative discontinuity
            level = low;
            BLEP_ME((phase - thresh) * inv_inc, low - high);
            fall_trigger = 9999;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(level);
    }
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_modulated_square(float const *phaseIncrements,
                                           float       *samples_out,
                                           size_t       count)
{
    BEGIN_Z;
    float phase        = mPhase;
    float thresh       = mThresh;
    float new_thresh   = mNewThresh;
    float high         = 1.0;
    float low          = thresh / (thresh - 1);
    float level        = phase < thresh ? high : low;
    float fall_trigger = level > 0 ? thresh : 9999;
    for (size_t i = 0; i < count; i++) {
        float inc = phaseIncrements[i];
        phase += inc;
        if (phase >= 1.0) {
            // positive discontinuity
            phase -= 1.0;
            level = high;
            BLEP_ME(phase / inc, high - low);
            thresh = new_thresh;
            low = thresh / (thresh - 1);
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // negative discontinuity
            level = low;
            BLEP_ME((phase - thresh) / inc, low - high);
            fall_trigger = 9999;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(level);
    }
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_sync_square(float const *phaseIncrements,
                                      float       *samples_out,
                                      float const *sync_in,
                                      float       *sync_out,
                                      size_t       count)
{
    BEGIN_Z;
    float phase        = mPhase;
    float thresh       = mThresh;
    float new_thresh   = mNewThresh;
    float high         = 1.0;
    float low          = thresh / (thresh - 1);
    float level        = phase < thresh ? high : low;
    float fall_trigger = level > 0 ? thresh : 9999;
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];
    for (size_t i = 0; i < count; i++) {
        float inc = phaseIncrements[i];
        phase += inc;
        if (i == next_reset) {
            float h0 = level;
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * inc * mInverseSampleRate;
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
            BLEP_ME(phase / inc, high - h0);
            // BLAM not needed.  Slope is always zero.
            level = high;
            if (thresh != new_thresh) {
                thresh = new_thresh;
                low = thresh / (thresh - 1);
            }
            fall_trigger = thresh;
        } else if (phase >= 1.0) {
            // positive discontinuity
            phase -= 1.0;
            level = high;
            float d = phase / inc;
            BLEP_ME(d, high - low);
            if (thresh != new_thresh) {
                thresh = new_thresh;
                low = thresh / (thresh - 1);
            }
            fall_trigger = thresh;
            sync_out[soi++] = i - 1 + d;
        } else if (phase > fall_trigger) {
            // negative discontinuity
            level = low;
            BLEP_ME((phase - thresh) / inc, low - high);
            fall_trigger = 9999;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(level);
    }
    sync_out[soi] = count + 1;
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_triangle(double freq,
                                   float *samples_out,
                                   size_t count)
{
    BEGIN_Z;
    float phase        = mPhase;
    float thresh       = mThresh;
    float new_thresh   = mNewThresh;
    float up_slope     = 2 / thresh;
    float dn_slope     = -2 / (1 - thresh);
    float m            = phase < thresh ? up_slope : dn_slope;
    float o            = phase < thresh ? 0 : thresh;
    float b            = phase < thresh ? -1 : +1;
    float fall_trigger = phase < thresh ? thresh : 9999;
    float inc          = freq * mInverseSampleRate;
    float inv_inc      = 1 / inc;
    for (size_t i = 0; i < count; i++) {
        phase += inc;
        if (phase >= 1.0) {
            // bottom corner and start of new waveform.
            // N.B., calculate BLAM using old dn_slope and new up_slope.
            phase       -= 1.0;
            thresh       = new_thresh;
            up_slope     = 2 / thresh;
            m            = up_slope;
            o            = 0;
            b            = -1;
            BLAM_ME(phase * inv_inc, (up_slope - dn_slope) * inc);
            dn_slope     = -2 / (1 - thresh);
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // top corner
            m = dn_slope;
            o = thresh;
            b = +1;
            BLAM_ME((phase - thresh) * inv_inc, (dn_slope - up_slope) * inc);
            fall_trigger = 9999;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(m * (phase - o) + b);
    }
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_modulated_triangle(float const *phaseIncrements,
                                             float       *samples_out,
                                             size_t       count)
{
    BEGIN_Z;
    float phase        = mPhase;
    float thresh       = mThresh;
    float new_thresh   = mNewThresh;
    float up_slope     = 2 / thresh;
    float dn_slope     = -2 / (1 - thresh);
    float m            = phase < thresh ? up_slope : dn_slope;
    float o            = phase < thresh ? 0 : thresh;
    float b            = phase < thresh ? -1 : +1;
    float fall_trigger = phase < thresh ? thresh : 9999;
    for (size_t i = 0; i < count; i++) {
        float inc = phaseIncrements[i];
        phase += inc;
        if (phase >= 1.0) {
            // bottom corner and start of new waveform.
            // N.B., calculate BLAM using old dn_slope and new up_slope.
            phase       -= 1.0;
            thresh       = new_thresh;
            up_slope     = 2 / thresh;
            m            = up_slope;
            o            = 0;
            b            = -1;
            BLAM_ME(phase / inc, (up_slope - dn_slope) * inc);
            dn_slope     = -2 / (1 - thresh);
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // top corner
            m = dn_slope;
            o = thresh;
            b = +1;
            BLAM_ME((phase - thresh) / inc, (dn_slope - up_slope) * inc);
            fall_trigger = 9999;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(m * (phase - o) + b);
    }
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_sync_triangle(float const *phaseIncrements,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t       count)
{
    BEGIN_Z;
    float phase        = mPhase;
    float thresh       = mThresh;
    float new_thresh   = mNewThresh;
    float up_slope     = 2 / thresh;
    float dn_slope     = -2 / (1 - thresh);
    float m            = phase < thresh ? up_slope : dn_slope;
    float o            = phase < thresh ? 0 : thresh;
    float b            = phase < thresh ? -1 : +1;
    float fall_trigger = phase < thresh ? thresh : 9999;
    size_t sii = 0, soi = 0;
    size_t next_reset  = (size_t)sync_in[sii];
    for (size_t i = 0; i < count; i++) {
        float inc = phaseIncrements[i];
        phase += inc;
        if (i == next_reset) {
            float h0 = m * (phase - o) + b;
            float m0 = m;
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * inc * mInverseSampleRate;
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
            thresh       = new_thresh;
            up_slope     = 2 / thresh;
            m            = up_slope;
            o            = 0;
            b            = -1;
            float h1 = m * (phase - o) + b;
            BLEP_ME(d, h1 - h0);
            if (m0 != m)
                BLAM_ME(d, (m - m0) * inc);
            fall_trigger = thresh;
        } else if (phase >= 1.0) {
            // bottom corner and start of new waveform.
            // N.B., calculate BLAM using old dn_slope and new up_slope.
            phase       -= 1.0;
            thresh       = new_thresh;
            up_slope     = 2 / thresh;
            m            = up_slope;
            o            = 0;
            b            = -1;
            float d = phase / inc;
            BLAM_ME(d, (up_slope - dn_slope) * inc);
            dn_slope     = -2 / (1 - thresh);
            sync_out[soi++] = i - 1 + d;
            fall_trigger = thresh;
        } else if (phase > fall_trigger) {
            // top corner
            m = dn_slope;
            o = thresh;
            b = +1;
            BLAM_ME((phase - thresh) / inc, (dn_slope - up_slope) * inc);
            fall_trigger = 9999;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(m * (phase - o) + b);
    }
    sync_out[soi] = count + 1;
    assert(sync_in[sii] >= count);
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_sine(double freq,
                               float *samples_out,
                               size_t count)
{
    BEGIN_Z;
    float phase = mPhase;
    float inc   = freq * mInverseSampleRate;
    for (size_t i = 0; i < count; i++) {
        phase += inc;
        if (phase >= 1.0) {
            phase -= 1.0;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(sinf(2 * M_PI * phase));
    }
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_modulated_sine(float const *phaseIncrements,
                                         float       *samples_out,
                                         size_t       count)
{
    BEGIN_Z;
    float phase = mPhase;
    for (size_t i = 0; i < count; i++) {
        phase += phaseIncrements[i];
        if (phase >= 1.0) {
            phase -= 1.0;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(sinf(2 * M_PI * phase));
    }
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_sync_sine(float const *phaseIncrements,
                                    float       *samples_out,
                                    float const *sync_in,
                                    float       *sync_out,
                                    size_t       count)
{
    BEGIN_Z;
    float phase = mPhase;
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];
    for (size_t i = 0; i < count; i++) {
        float inc = phaseIncrements[i];
        phase += inc;
        if (i == next_reset) {
            float h0 = sinf(2 * M_PI * phase);
            float m0 = cosf(2 * M_PI * phase) * inc;
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * inc * mInverseSampleRate;
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
            float h1 = 0;
            float m1 = 1 * inc;
            BLEP_ME(d, h1 - h0);
            BLAM_ME(d, m1 - m0);
        } else if (phase >= 1.0) {
            phase -= 1.0;
            float d = phase / inc;
            sync_out[soi++] = i - 1 + d;
        }
        samples_out[i] = SHIFT_OUT();
        SHIFT_IN(sinf(2 * M_PI * phase));
    }
    sync_out[soi] = count + 1;
    assert(sync_in[sii] >= count);
    mPhase = phase;
    END_Z;
}

void Oscillator::begin_chunk(Waveform waveform, float freq, float modifier)
{
    float thresh = 0.5 - 0.49 * modifier; // maximum skew is 99%.
    float phase = mPhase;
    if (mWaveform != waveform) {
        BEGIN_Z;
        float h0, h1, m0, m1;  // height and slope
        if (mThresh == -1)
            mThresh = thresh;
        calc_h_m(mWaveform, freq, mPhase, mThresh, &h0, &m0);
        calc_h_m(waveform, freq, phase, mThresh, &h1, &m1);
        // fprintf(stderr, "waveform %s -> %s\n", OTname(mWaveform), OTname(waveform));
        if (h1 != h0) {
            BLEP_ME(0, h1 - h0);
            // fprintf(stderr, "BLEP h = %g - %g = %g\n", h1, h0, h1 - h0);
        }
        if (m1 != m0) {
            BLAM_ME(0, m1 - m0);
            // fprintf(stderr, "BLAM m = %g - %g = %g\n", m1, m0, m1 - m0);
        }
        // fprintf(stderr, "\n");
        mWaveform = waveform;
        END_Z;
    }
    mPhase = phase;
    mNewThresh = thresh;
}

// h is the height of the waveform at the given phase.  m is its slope.
// In other works, y and dy/dx.
inline void Oscillator::calc_h_m(Waveform waveform,
                                 float    freq,
                                 float    phase,
                                 float    thresh,
                                 float   *h,
                                 float   *m)
{
    switch (waveform) {

        case None:
            *h = 0;
            *m = 0;
            break;

        case Saw:
            *h = 2 * phase - 1;
            *m = 2 * freq / mSampleRate;
            break;

        case Square:
            // fprintf(stderr, "calc square: phase=%g thresh=%g\n", phase, thresh);
            if (phase < thresh)
                *h = 1;
            else
                *h = thresh / (thresh - 1);
            // fprintf(stderr, "             *h = %g\n", *h);
            *m = 0;
            break;

        case Triangle:
            if (phase < thresh) {
                float up_slope = 2 / thresh;
                // fprintf(stderr, "calc tri:    freq = %g\n", freq);
                // fprintf(stderr, "             up slope = %g\n", up_slope);
                // fprintf(stderr, "             phase=%g thresh=%g\n",
                //                               phase, thresh);
                *h = phase * up_slope - 1;
                *m = up_slope * freq / mSampleRate;
            } else {
                float dn_slope = -2 / (1 - thresh);
                // fprintf(stderr, "calc tri:    freq = %g\n", freq);
                // fprintf(stderr, "             dn slope = %g\n", dn_slope);
                // fprintf(stderr, "             phase=%g thresh=%g\n",
                //                               phase, thresh);
                *h = 1 + dn_slope * (phase - thresh);
                *m = dn_slope * freq / mSampleRate;
            }
            break;

        case Sine:
            *h = sinf(2 * M_PI * phase);
            *m = cosf(2 * M_PI * phase) * freq / mSampleRate;
            // fprintf(stderr, "calc sine:   freq = %g\n", freq);
            // fprintf(stderr, "             phase = %g\n", phase);
            // fprintf(stderr, "             *h = %g\n", *h);
            // fprintf(stderr, "             *m = %g\n", *m);
            break;
    }
}
