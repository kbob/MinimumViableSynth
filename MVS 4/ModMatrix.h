//
//  ModMatrix.h
//  MVS 4
//
//  Created by Bob Miller on 12/22/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS_4__ModMatrix__
#define __MVS_4__ModMatrix__

#include "ParamSet.h"


// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark ModMatrix<NS, ND> Declaration

template<int NS, int ND>
class ModMatrix {

public:

    enum {
        SourceCount      = NS,
        DestinationCount = ND,
        NullSrc          = 0,
        NullDest         = 0,
    };

    typedef unsigned short Source, Destination;

    ModMatrix();

    inline Source get_src(Destination dest, size_t index) const;
    inline Destination get_dest(Source src) const;

    void assign(Source mod, Destination dest);

private:

    // map modulator -> dest;
    Destination dest_vec[SourceCount];

    // map dest -> [modulator]
    Source src_matrix[DestinationCount][SourceCount];

    void add_mod(Source mod, Destination dest);
    void remove_mod(Source mod, Destination dest);

};


// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark ModBox<NS, ND> Declaration

template <int NS, int ND>
class ModBox {

public:

    typedef ModMatrix<NS, ND>            Matrix;
    typedef typename Matrix::Source      Source;
    typedef typename Matrix::Destination Destination;

    ModBox(const Matrix&, size_t nsamp, const float **mod_values);
    ModBox(const ModBox&);

    size_t sampleCount() const { return mSampleCount; }

    void set_values(Source src, const float *values);
    const float *get_values(Source src);

    void modulate(float             base,
                  Destination       dest,
                  float            *values_out) const;

    void modulate_freq(float        base,
                       Destination  dest,
                       float       *values_out) const;

private:

    const Matrix& mMatrix;
    size_t mSampleCount;
    const float *mModValues[NS];

    ModBox& operator = (const ModBox&);
    
};


// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark ModMatrix<NS, ND> Definition

template <int NS, int ND>
ModMatrix<NS, ND>::ModMatrix()
{
    for (size_t i = 0; i < DestinationCount; i++) {
        dest_vec[i] = NullDest;
        for (size_t j = 0; j < SourceCount; j++)
            src_matrix[i][j] = NullSrc;
    }
}

template <int NS, int ND>
inline typename ModMatrix<NS, ND>::Source
ModMatrix<NS, ND>::get_src(Destination dest, size_t index) const
{
    if (index >= SourceCount)
        return NullSrc;
    const Source *src_row = src_matrix[dest];
    return src_row[index];
}

template <int NS, int ND>
void ModMatrix<NS, ND>::assign(Source mod, Destination dest)
{
    Destination old_dest = dest_vec[mod];
    if (old_dest != NullDest)
        remove_mod(mod, old_dest);
    if (dest != NullDest)
        add_mod(mod, dest);
    dest_vec[mod] = dest;
}

template <int NS, int ND>
void ModMatrix<NS, ND>::add_mod(Source mod, Destination dest)
{
    Source *src_row = src_matrix[dest];
    size_t i, j;

    for (i = 0; i < SourceCount && src_row[i] != NullSrc && src_row[i] < mod; i++)
        continue;
    assert(i < SourceCount);
    if (src_row[i] == mod)
        return;
    for (j = i; j < SourceCount && src_row[j] != NullSrc; j++)
        continue;
    for ( ; j > i; --j)
        src_row[j] = src_row[j - 1];
    src_row[i] = mod;
}

template <int NS, int ND>
void ModMatrix<NS, ND>::remove_mod(Source mod, Destination dest)
{
    Source *src_row = src_matrix[dest];
    for (size_t i = 0; i < SourceCount && src_row[i]; i++) {
        if (src_row[i] == mod) {
            for ( ; i < SourceCount - 1 && src_row[i] != NullSrc; i++)
                src_row[i] = src_row[i + 1];
            src_row[i] = NullSrc;
            break;
        }
    }
}


// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark ModBox<NS, ND> Definition

template <int NS, int ND>
ModBox<NS, ND>::ModBox(const Matrix& mat,
                       size_t        nsamp,
                       const float **mod_values)
: mMatrix(mat),
mSampleCount(nsamp)
{
    for (size_t i = 0; i < NS; i++)
        mModValues[i] = mod_values[i];
}

template <int NS, int ND>
ModBox<NS, ND>::ModBox(const ModBox& that)
: mMatrix(that.mMatrix),
mSampleCount(that.mSampleCount)
{
    for (size_t i = 0; i < NS; i++)
        mModValues[i] = that.mModValues[i];
}

template <int NS, int ND>
void ModBox<NS, ND>::set_values(Source src, const float *values)
{
    assert(src < NS);
    mModValues[src] = values;
}

template <int NS, int ND>
const float *ModBox<NS, ND>::get_values(Source src)
{
    assert(src < NS);
    return mModValues[src];
}

template <int NS, int ND>
void ModBox<NS, ND>::modulate(float       base,
                              Destination dest,
                              float      *values_out) const
{
    for (size_t i = 0; i < mSampleCount; i++)
        values_out[i] = base;
    for (size_t i = 0; i < NS; i++) {
        Source m = mMatrix.get_src(dest, i);
        if (!m)
            break;
        const float *src_values = mModValues[m];
        assert(src_values);
        for (size_t j = 0; j < mSampleCount; j++)
            values_out[j] += src_values[j];
    }
}

template <int NS, int ND>
void ModBox<NS, ND>::modulate_freq(float       base,
                                   Destination dest,
                                   float      *values_out) const
{
    for (size_t i = 0; i < mSampleCount; i++)
        values_out[i] = base;
    for (size_t i = 0; i < NS; i++) {
        Source m = mMatrix.get_src(dest, i);
        if (!m)
            break;
        const float *src_values = mModValues[m];
        assert(src_values);

        for (size_t j = 0; j < mSampleCount; j++)
            values_out[j] *= powf(2.0, src_values[j]);
    }
}

#endif /* defined(__MVS_4__ModMatrix__) */
