//
//  Decimator.h
//  Sample4X
//
//  Created by Bob Miller on 11/26/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

// Polyphase decimation filter.
//
// Convert an oversampled audio stream to non-oversampled.  Uses a
// windowed sinc FIR filter w/ Blackman window to control aliasing.
// Christian Floisand's 'blog explains it very well.
//
// This version has a very simple main processing loop (the decimate
// method) which vectorizes easily.
//
// Refs:
//   https://christianfloisand.wordpress.com/2012/12/05/audio-resampling-part-1/
//   https://christianfloisand.wordpress.com/2013/01/28/audio-resampling-part-2/
//   http://www.dspguide.com/ch16.htm
//   http://en.wikipedia.org/wiki/Window_function#Blackman_windows

#ifndef __Sample4X__Decimator__
#define __Sample4X__Decimator__

#include <stddef.h>

class Decimator {

public:
    Decimator();
    ~Decimator();

    void     initialize(Float64  decimatedSampleRate,
                        Float64  passFrequency,
                        unsigned oversampleRatio);

    Float64  oversampleRate()  const { return mOversampleRate; }
    int      oversampleRatio() const { return mRatio; }

    void     decimate(Float32 *in, Float32 *out, size_t outCount);
    // N.B., in must have (ratio * outCount) samples.

private:
    Float64  mDecimatedSampleRate;
    Float64  mOversampleRate;
    int      mRatio;            // oversample ratio
    Float32 *mKernel;
    size_t   mKernelSize;
    Float32 *mShift;            // shift register
    size_t   mCursor;

};

#endif /* defined(__Sample4X__Decimator__) */
