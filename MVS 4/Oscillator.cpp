//
//  Oscillator.cpp
//  MVS
//
//  Created by Bob Miller on 11/17/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Oscillator.h"

#include <algorithm>

// Stupid delay line trick.

// This delay line is stored in a member of the Oscillator object, but
// it is also cached in four local variables.  Between BEGIN_Z and
// END_Z, the local variables are authoritative; elsewhere the
// member variable is the only copy.
//
// I do it this way in the hope that the compiler can generate better
// code than if every access touched the array.

#define BEGIN_Z                                                         \
    {                                                                   \
        float zm2 = mShiftZ[0],                                         \
              zm1 = mShiftZ[1],                                         \
              z0  = mShiftZ[2],                                         \
              zp1 = mShiftZ[3],                                         \
              ztmp; ztmp = 0;

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

#define SHIFT_Z(x) ((ztmp = SHIFT_OUT()), SHIFT_IN((x)), ztmp)

// polyBLEP: polynomial approximation to Bandwidth-Limited stEP function
// polyBLAM: polynomial approximation to Bandwidth-Limited rAMp function
//
// Not really.  It's just fun to say BLEP and BLAM.
//
// BLEP_ME and BLAM_ME implicitly use the delay line, so only call
// them between Z_BEGIN and Z_END.
//
// BLEP is implemented for orders 3, 1, and 0 (none).
// BLAM is implemented for orders 1 and 0.

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

    #define BLEP_ME(d, h) ((void)((d) + (h))) /* La di da... */

#else

    #error "unknown BLEP order"

#endif

#if BLAM_ORDER == 1

    #define BLAM_ME(d, m)                                               \
        ((zm1 += (m) * (+ (d) * (d) * (d) / 6)),                        \
         (z0  += (m) * (+ (1-(d)) * (1-(d)) * (1-(d)) / 6)))

#elif BLAM_ORDER == 0

    #define BLAM_ME(d, m) ((void)((d) + (m)))

#else

    #error "unknown BLAM order"

#endif

static float clamp_width(float width)
{
    return std::max<float>(0.01f, std::min<float>(0.50f, width));
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

Oscillator::Oscillator()
{}

void Oscillator::initialize(float sample_rate)
{
    mWaveform = None;
    mWaveformChanged = true;
    mPhase = 0;
    mThresh = -1;
    CLEAR_Z();
}

void Oscillator::generate(Waveform     waveform,
                          float        freq,
                          float        width,
                          float       *samples_out,
                          size_t       count)
{
    begin_chunk(waveform, freq, width);
    switch (waveform) {

    case None:
        break;

    case Saw:
        generate_saw(freq, samples_out, count);
        break;

    case Square:
        generate_square(freq, width, samples_out, count);
        break;

    case Triangle:
        generate_triangle(freq, width, samples_out, count);
        break;

    case Sine:
        generate_sine(freq, samples_out, count);
        break;
    }
}


void Oscillator::generate_modulated(Waveform     waveform,
                                    float const *freqs,
                                    float const *widths,
                                    float       *samples_out,
                                    size_t       count)
{
    begin_chunk(waveform, freqs[0], widths[0]);
    switch (waveform) {

    case None:
        break;

    case Saw:
        generate_modulated_saw(freqs, samples_out, count);
        break;

    case Square:
        generate_modulated_square(freqs, widths, samples_out, count);
        break;

    case Triangle:
        generate_modulated_triangle(freqs, widths, samples_out, count);
        break;

    case Sine:
        generate_modulated_sine(freqs, samples_out, count);
        break;
    }
}

void Oscillator::generate_with_sync(Waveform     waveform,
                                    float const *freqs,
                                    float const *widths,
                                    float       *samples_out,
                                    float const *sync_in,
                                    float       *sync_out,
                                    size_t       count)
{
    begin_chunk(waveform, freqs[0], widths[0]);
    switch (waveform) {

    case None:
        generate_sync_only(freqs, samples_out, sync_in, sync_out, count);
        break;

    case Saw:
        generate_sync_saw(freqs, samples_out, sync_in, sync_out, count);
        break;

    case Square:
        generate_sync_square(freqs,
                             widths,
                             samples_out,
                             sync_in,
                             sync_out,
                             count);
        break;

    case Triangle:
        generate_sync_triangle(freqs,
                               widths,
                               samples_out,
                               sync_in,
                               sync_out,
                               count);
        break;

    case Sine:
        generate_sync_sine(freqs, samples_out, sync_in, sync_out, count);
        break;
    }
    
}

// h is the height of the waveform at the given phase.  m is its slope.
// In other words, y and dy/dx.
inline void Oscillator::calc_h_m(Waveform   waveform,
                                 float  freq,
                                 float  phase,
                                 float  thresh,
                                 float *h,
                                 float *m)
{
    switch (waveform) {

    case None:
        *h = 0;
        *m = 0;
        break;

    case Saw:
        *h = -2 * phase + 1;
        *m = -2 * freq;
        break;

    case Square:
        if (phase < thresh)
            *h = 1;
        else
            *h = thresh / (thresh - 1);
        *m = 0;
        break;

    case Triangle:
        if (phase < thresh) {
            float up_slope = +2 / thresh;
            *h = up_slope * phase - 1;
            *m = up_slope * freq;
        } else {
            float dn_slope = -2 / (1 - thresh);
            *h = dn_slope * (phase - thresh) + 1;
            *m = dn_slope * freq;
        }
        break;

    case Sine:
        *h = sinf(2 * M_PI * phase);
        *m = cosf(2 * M_PI * phase) * freq;
        break;
    }
}

void Oscillator::begin_chunk(Waveform waveform, float freq, float width0)
{
    mWaveformChanged = mWaveform != waveform;
    if (mWaveformChanged) {
        BEGIN_Z;
        float phase = mPhase;
        float h0, h1, m0, m1;
        if (mThresh == -1)
            mThresh = width0;
        width0 = clamp_width(width0);
        calc_h_m(mWaveform, freq, phase, width0, &h0, &m0);
        calc_h_m(waveform, freq, phase + freq, width0, &h1, &m1);
        // printf("New Waveform: %s -> %s\n",
        //        OTname(mWaveform), OTname(waveform));
        if (h1 != h0) {
            BLEP_ME(0, h1 - h0);
            // printf("    BLEP h = %g - %g = %g\n", h1, h0, h1 - h0);
        }
        if (m1 != m0) {
            BLAM_ME(0, m1 - m0);
            // printf("    BLAM m = %g - %g = %g\n", m1, m0, m1 - m0);
        }
        // printf("\n");
        mWaveform = waveform;
        END_Z;
    }
}


// -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark Basic Generators


void Oscillator::generate_saw(float freq, float *samples_out, size_t count)
{
    BEGIN_Z;
    float phase    = mPhase;
    float inv_freq = 1 / freq;

    for (size_t i = 0; i < count; i++) {
        phase += freq;
        if (phase >= 1.0) {
            // positive discontinuity
            phase -= 1.0;
            if (i || !mWaveformChanged)
                BLEP_ME(phase * inv_freq, +2.0);
        }
        samples_out[i] = SHIFT_Z(-2 * phase + 1);
    }
    mPhase = phase;
    mThresh = -1;
    END_Z;
}

void Oscillator::generate_square(float freq,
                                 float width,
                                 float *samples_out,
                                 size_t count)
{
    BEGIN_Z;
    float phase    = mPhase;
    float thresh   = mThresh;
    float high     = 1.0;
    float low      = thresh / (thresh - 1);
    float level    = phase < thresh ? high : low;
    float inv_freq = 1 / freq;

    width = clamp_width(width);
    for (size_t i = 0; i < count; i++) {
        phase += freq;
        if (phase >= 1.0) {
            // Positive discontinuity
            phase -= 1;
            if (i || !mWaveformChanged)
                BLEP_ME(phase * inv_freq, high - level);
            level = high;
        }
        if (level > 0 && phase >= width) {
            // Negative discontinuity
            low = width / (width - 1);
            if (i || !mWaveformChanged)
                BLEP_ME((phase - width) * inv_freq, low - level);
            level = low;
            thresh = width;
        }
        samples_out[i] = SHIFT_Z(level);
    }
    mPhase  = phase;
    mThresh = thresh;
    END_Z;
}

void Oscillator::generate_triangle(float freq,
                                   float width,
                                   float *samples_out,
                                   size_t count)
{
    BEGIN_Z;
    float phase    = mPhase;
    float thresh   = mThresh;
    float slope    = phase < thresh ? +2 / thresh : -2 / (1 - thresh);
    float inv_freq = 1 / freq;

    width = clamp_width(width);
    for (size_t i = 0; i < count; i++) {
        phase += freq;
        if (phase >= 1.0) {
            // bottom corner and start of new waveform
            phase -= 1.0;
            float m0 = slope;
            slope = 2 / width;
            if (i || !mWaveformChanged)
                BLAM_ME(phase * inv_freq, (slope - m0) * freq);
        }
        float level;
        if (phase < width) {
            // rising slope
            level = slope * phase - 1;
        } else if (slope > 0) {
            // top corner
            float m0 = slope;
            slope = -2 / (1 - width);
            thresh = width;
            float ph1 = phase - thresh;
            if (i || !mWaveformChanged)
                BLAM_ME(ph1 * inv_freq, (slope - m0) * freq);
            level = slope * ph1 + 1;
        } else {
            // falling slope
            float ph1 = phase - thresh;
            level = slope * ph1 + 1;
        }
        samples_out[i] = SHIFT_Z(level);
    }
    mPhase  = phase;
    mThresh = thresh;
    END_Z;
}

void Oscillator::generate_sine(float freq,
                               float *samples_out,
                               size_t count)
{
    BEGIN_Z;
    float phase = mPhase;

    for (size_t i = 0; i < count; i++) {
        phase += freq;
        if (phase >= 1.0)
            phase -= 1.0;
        float y = sinf(2 * M_PI * phase);
        samples_out[i] = SHIFT_Z(y);
    }
    mPhase = phase;
    mThresh = -1;
    END_Z;
}


// -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark Modulated Generators


void Oscillator::generate_modulated_saw(float const *freqs,
                                        float       *samples_out,
                                        size_t       count)
{
    BEGIN_Z;
    float phase = mPhase;

    for (size_t i = 0; i < count; i++) {
        float freq = freqs[i];
        phase += freq;
        if (phase >= 1.0) {
            // positive discontinuity
            phase -= 1.0;
            if (i || !mWaveformChanged)
                BLEP_ME(phase / freq, +2.0);
        }
        samples_out[i] = SHIFT_Z(-2 * phase + 1);
    }
    mPhase = phase;
    mThresh = -1;
    END_Z;
}

void Oscillator::generate_modulated_square(float const *freqs,
                                           float const *widths,
                                           float       *samples_out,
                                           size_t       count)
{
    BEGIN_Z;
    float phase  = mPhase;
    float thresh = mThresh;
    float high   = 1.0;
    float low    = thresh / (thresh - 1);
    float level  = phase < thresh ? high : low;

    for (size_t i = 0; i < count; i++) {
        float freq = freqs[i];
        phase += freq;
        if (phase >= 1.0) {
            // Positive discontinuity
            phase -= 1;
            if (i || !mWaveformChanged)
                BLEP_ME(phase / freq, high - level);
            level = high;
        }
        if (level > 0) {
            float width = clamp_width(widths[i]);
            if (phase >= width) {
                // Negative discontinuity
                low = width / (width - 1);
                if (i || !mWaveformChanged) {
                    float d = (phase - width) / freq;
                    if (d >= 1)
                        d = 0;
                    float h = low - level;
                    BLEP_ME(d, h);
                    // printf("N: d = %.3g = (%.3g - %.3g) / %.3g, "
                    //        "h = %.3g = %.3g - %.3g\n",
                    //        d, phase, width, freq, h, low, level);
                }
                level = low;
            }
            thresh = width;
        } 
        samples_out[i] = SHIFT_Z(level);
    }
    mPhase  = phase;
    mThresh = thresh;
    END_Z;
}

void Oscillator::generate_modulated_triangle(float const *freqs,
                                             float const *widths,
                                             float       *samples_out,
                                             size_t       count)
{
    BEGIN_Z;
    float phase  = mPhase;
    float thresh = mThresh;
    float slope  = phase < thresh ? +2 / thresh : -2 / (1 - thresh);

    static size_t nsamp = 0;
    
    for (size_t i = 0; i < count; i++) {
        float width = clamp_width(widths[i]);
        float freq = freqs[i];
        phase += freq;
        if (phase >= 1.0) {
            // bottom corner and start of new waveform
            phase -= 1.0;
            float m0 = slope;
            slope = +2 / width;
            if (i || !mWaveformChanged)
                BLAM_ME(phase / freq, (slope - m0) * freq);
            // else
            //     printf("No BLAM!!!\n");
            // printf("B %zu\n", nsamp + i);
        }
        float level;
        if (phase < width) {
            // rising slope
            slope = +2 / width;
            thresh = width;
            level = slope * phase - 1;
        } else if (slope > 0) {
            // top corner
            float m0 = slope;
            slope = -2 / (1 - width);
            thresh = width;
            float ph1 = phase - thresh;
            if (i || !mWaveformChanged) {
                float d = ph1 / freq;
                if (d >= 1)
                    d = 0;
                float m = (slope - m0) * freq;
                BLAM_ME(d, m);
                // printf("T %zu: d = %.3g = %.3g / %.3g, "
                //        "m = %.3g = (%.3g - %.3g) * %.3g\n",
                //        nsamp + i, d, ph1, freq, m, slope, m0, freq);
            }
            // else
            //     printf("No BLAM!!!\n");
            level = slope * ph1 + 1;
        } else {
            // falling slope
            float ph1 = phase - thresh;
            level = slope * ph1 + 1;
        }
        samples_out[i] = SHIFT_Z(level);
    }
    nsamp += count;
    mPhase  = phase;
    mThresh = thresh;
    END_Z;
}

void Oscillator::generate_modulated_sine(float const *freqs,
                                         float       *samples_out,
                                         size_t       count)
{
    BEGIN_Z;
    float phase = mPhase;

    for (size_t i = 0; i < count; i++) {
        float freq = freqs[i];
        phase += freq;
        if (phase >= 1.0)
            phase -= 1.0;
        float y = sinf(2 * M_PI * phase);
        samples_out[i] = SHIFT_Z(y);
    }
    mPhase = phase;
    mThresh = -1;
    END_Z;
}


// -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark Synchronized Generators

void Oscillator::generate_sync_only(float const *freqs,
                                    float       *samples_out,
                                    float const *sync_in,
                                    float       *sync_out,
                                    size_t      count)
{
    BEGIN_Z;
    float phase = mPhase;
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];

    for (size_t i = 0; i < count; i++) {
        float freq = freqs[i];
        phase += freq;
        if (i == next_reset) {
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * freq;
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
            // BLEP/BLAM not needed.  We are silent.
        } else if (phase >= 1.0) {
            phase -= 1.0;
            float d = phase / freq;
            sync_out[soi++] = i - 1 + d;
        }
        samples_out[i] = SHIFT_Z(0);
    }
    sync_out[soi] = count + 1;
    mPhase = phase;
    END_Z;
}

void Oscillator::generate_sync_saw(float const *freqs,
                                   float       *samples_out,
                                   float const *sync_in,
                                   float       *sync_out,
                                   size_t      count)
{
    BEGIN_Z;
    float phase = mPhase;
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];

    for (size_t i = 0; i < count; i++) {
        float freq = freqs[i];
        phase += freq;
        if (i == next_reset) {
            // hard sync
            float h0 = -2 * phase + 1;
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * freq;
            if (i || !mWaveformChanged)
                BLEP_ME(d, +1 - h0);
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
        } else if (phase >= 1.0) {
            // positive discontinuity
            phase -= 1.0;
            float d = phase / freq;
            if (i || !mWaveformChanged)
                BLEP_ME(phase / freq, +2.0);
            sync_out[soi++] = i - 1 + d;
        }
        samples_out[i] = SHIFT_Z(-2 * phase + 1);
    }
    sync_out[soi] = count + 1;
    mPhase = phase;
    mThresh = -1;
    END_Z;
}

void Oscillator::generate_sync_square(float const *freqs,
                                      float const *widths,
                                      float       *samples_out,
                                      float const *sync_in,
                                      float       *sync_out,
                                      size_t      count)
{
    BEGIN_Z;
    float phase  = mPhase;
    float thresh = mThresh;
    float high   = 1.0;
    float low    = thresh / (thresh - 1);
    float level  = phase < thresh ? high : low;
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];

    static size_t  nsamp = 0;

    for (size_t i = 0; i < count; i++) {
        float freq = freqs[i];
        phase += freq;
        if (i == next_reset) {
            // hard sync
            float h0 = level;
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * freq;
            level = high;
            if (i || !mWaveformChanged)
                BLEP_ME(d, level - h0);
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
        } else if (phase >= 1.0) {
            // Positive discontinuity
            phase -= 1;
            float d = phase / freq;
            if (i || !mWaveformChanged)
                BLEP_ME(d, high - level);
            level = high;
            sync_out[soi++] = i - 1 + d;
        }
        if (level > 0) {
            float width = clamp_width(widths[i]);
            if (phase >= width) {
                // Negative discontinuity
                low = width / (width - 1);
                if (i || !mWaveformChanged) {
                    float d = (phase - width) / freq;
                    if (d >= 1)
                        d = 0;
                    float h = low - level;
                    BLEP_ME(d, h);
                    // printf("N: d = %.3g = (%.3g - %.3g) / %.3g, "
                    //        "h = %.3g = %.3g - %.3g\n",
                    //        d, phase, width, freq, h, low, level);
                }
                level = low;
            }
            thresh = width;
        }
        samples_out[i] = SHIFT_Z(level);
    }
    nsamp += count;
    sync_out[soi] = count + 1;
    mPhase  = phase;
    mThresh = thresh;
    END_Z;
}

void Oscillator::generate_sync_triangle(float const *freqs,
                                        float const *widths,
                                        float       *samples_out,
                                        float const *sync_in,
                                        float       *sync_out,
                                        size_t      count)
{
    BEGIN_Z;
    float phase  = mPhase;
    float thresh = mThresh;
    float slope  = phase < thresh ? +2 / thresh : -2 / (1 - thresh);
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];

    static size_t nsamp = 0;
    
    for (size_t i = 0; i < count; i++) {
        float width = clamp_width(widths[i]);
        float freq = freqs[i];
        phase += freq;
        if (i == next_reset) {
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * freq;
            float m0 = slope;
            slope = +2 / width;
            if (i || !mWaveformChanged)
                BLAM_ME(d, (slope - m0) * freq);
            sync_out[soi++] = sync_in[sii++];
        } else if (phase >= 1.0) {
            // bottom corner and start of new waveform
            phase -= 1.0;
            float m0 = slope;
            slope = +2 / width;
            float d = phase / freq;
            if (i || !mWaveformChanged)
                BLAM_ME(d, (slope - m0) * freq);
            // else
            //     printf("No BLAM!!!\n");
            // printf("B %zu\n", nsamp + i);
            sync_out[soi++] = i - 1 + d;
        }
        float level;
        if (phase < width) {
            // rising slope
            slope = +2 / width;
            thresh = width;
            level = slope * phase - 1;
        } else if (slope > 0) {
            // top corner
            float m0 = slope;
            slope = -2 / (1 - width);
            thresh = width;
            float ph1 = phase - thresh;
            if (i || !mWaveformChanged) {
                float d = ph1 / freq;
                if (d >= 1)
                    d = 0;
                float m = (slope - m0) * freq;
                BLAM_ME(d, m);
                // printf("T %zu: d = %.3g = %.3g / %.3g, "
                //        "m = %.3g = (%.3g - %.3g) * %.3g\n",
                //        nsamp + i, d, ph1, freq, m, slope, m0, freq);
            }
            // else
            //     printf("No BLAM!!!\n");
            level = slope * ph1 + 1;
        } else {
            // falling slope
            float ph1 = phase - thresh;
            level = slope * ph1 + 1;
        }
        samples_out[i] = SHIFT_Z(level);
    }
    sync_out[soi] = count + 1;
    nsamp += count;
    mPhase  = phase;
    mThresh = thresh;
    END_Z;
}

void Oscillator::generate_sync_sine(float const *freqs,
                                    float       *samples_out,
                                    float const *sync_in,
                                    float       *sync_out,
                                    size_t      count)
{
    BEGIN_Z;
    float phase = mPhase;
    size_t sii = 0, soi = 0;
    size_t next_reset = (size_t)sync_in[sii];

    for (size_t i = 0; i < count; i++) {
        float freq = freqs[i];
        phase += freq;
        if (i == next_reset) {
            float h0 = sinf(2 * M_PI * phase);
            float m0 = cosf(2 * M_PI * phase) * freq;
            float d = fmodf(sync_in[sii], 1.0);
            phase = d * freq;
            float h1 = sinf(2 * M_PI * phase);
            float m1 = cosf(2 * M_PI * phase) * freq;
            if (i || !mWaveformChanged) {
                BLEP_ME(d, h1 - h0);
                BLAM_ME(d, m1 - m0);
            }
            sync_out[soi++] = sync_in[sii++];
            next_reset = (size_t)sync_in[sii];
        } else if (phase >= 1.0) {
            phase -= 1.0;
            float d = phase / freq;
            sync_out[soi++] = i - 1 + d;
        }
        float y = sinf(2 * M_PI * phase);
        samples_out[i] = SHIFT_Z(y);
    }
    sync_out[soi] = count + 1;
    mPhase = phase;
    mThresh = -1;
    END_Z;
}
