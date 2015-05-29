//
//  Decimator.cpp
//  Sample4X
//
//  Created by Bob Miller on 11/26/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "Decimator.h"

Decimator::Decimator()
: mKernel(NULL),
  mShift(NULL)
{}

Decimator::~Decimator()
{
    delete [] mKernel;
    delete [] mShift;
}

void Decimator::initialize(Float64  decimatedSampleRate,
                           Float64  passFrequency,
                           unsigned oversampleRatio)
{
    mDecimatedSampleRate = decimatedSampleRate;
    mRatio = oversampleRatio;
    mOversampleRate = decimatedSampleRate * oversampleRatio;
    if (mRatio == 1)
        return;           // The null decimator

    Float64 NyquistFreq = decimatedSampleRate / 2;
    assert(passFrequency < NyquistFreq);

    // See DSP Guide.
    Float64 Fc = (NyquistFreq + passFrequency) / 2 / mOversampleRate;
    Float64 BW = (NyquistFreq - passFrequency) / mOversampleRate;
    int M = ceil(4 / BW);
    if (M % 2) M++;
    size_t activeKernelSize = M + 1;
    size_t inactiveSize = mRatio - activeKernelSize % mRatio;
    mKernelSize = activeKernelSize + inactiveSize;

    // DSP Guide uses approx. values.  Got these from Wikipedia.
    Float64 a0 = 7938. / 18608., a1 = 9240. / 18608., a2 = 1430. / 18608.;

    // Allocate and initialize the FIR filter kernel.
    delete [] mKernel;
    mKernel = new Float32[mKernelSize];
    Float64 gain = 0;
    for (size_t i = 0; i < inactiveSize; i++)
        mKernel[i] = 0;
    for (int i = 0; i < activeKernelSize; i++) {
        Float64 y;
        if (i == M/2)
            y = 2 * M_PI * Fc;
        else
            y = (sin(2 * M_PI * Fc * (i - M / 2)) / (i - M / 2) *
                 (a0 - a1 * cos(2 * M_PI * i/ M) + a2 * cos(4 * M_PI / M)));
        gain += y;
        mKernel[inactiveSize + i] = y;
    }

    // Adjust the kernel for unity gain.
    Float32 inv_gain = 1 / gain;
    for (size_t i = inactiveSize; i < mKernelSize; i++)
        mKernel[i] *= inv_gain;

    // Allocate and clear the shift register.
    delete [] mShift;
    mShift = new Float32[mKernelSize];
    for (size_t i = 0; i < mKernelSize; i++)
        mShift[i] = 0;
    mCursor = 0;
}

// The filter kernel is linear.  Coefficients for oldest samples are
// on the left; newest on the right.
//
// The shift register is circular.  Oldest samples are at cursor;
// newest are just left of cursor.
//
// We have to do the multiply-accumulate in two pieces.
//
//  Kernel
//  +------------+----------------+
//  | 0 .. n-c-1 |   n-c .. n-1   |
//  +------------+----------------+
//   ^            ^                ^
//   0            n-c              n
//
//  Shift Register
//  +----------------+------------+
//  |   n-c .. n-1   | 0 .. n-c-1 |
//  +----------------+------------+
//   ^                ^            ^
//   mShift           shiftp       n

void Decimator::decimate(Float32 *in, Float32 *out, size_t outCount)
{
    if (mRatio == 1) {
        for (size_t i = 0; i < outCount; i++)
            out[i] = in[i];
        return;
    }

    assert(!(mCursor % mRatio));
    assert(mCursor < mKernelSize);
    size_t cursor = mCursor;
    Float32 *inp = in;
    Float32 *shiftp = mShift + cursor;
    for (size_t i = 0; i < outCount; i++) {

        // Insert mRatio input samples at cursor.
        for (size_t j = 0; j < mRatio; j++)
            *shiftp++ = *inp++;
        if ((cursor += mRatio) == mKernelSize) {
            cursor = 0;
            shiftp = mShift;
        }

        // Calculate one output sample.
        Float64 acc = 0;
        size_t size0 = mKernelSize - cursor;
        size_t size1 = cursor;
        const Float32 *kernel1 = mKernel + size0;
        for (size_t j = 0; j < size0; j++)
            acc += shiftp[j] * mKernel[j];
        for (size_t j = 0; j < size1; j++)
            acc += mShift[j] * kernel1[j];
        out[i] = acc;
    }
    mCursor = cursor;
}
