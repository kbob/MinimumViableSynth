//
//  ParamSet.cpp
//  MVS
//
//  Created by Bob Miller on 12/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include "ParamSet.h"

static int         next_clump_ID = kAudioUnitClumpID_System + 1;
static int         next_param_index = 0;
static ParamSet   *current_paramset;
static ParamClump *current_clump;

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark Param Base Class

Param::Param()
    : mIndex(-1),
      mModIndex(0)
{
    assert(current_paramset);
    current_paramset->mIndex.push_back(this);
    memset(&mInfo, 0, sizeof mInfo);
    mInfo.flags = (kAudioUnitParameterFlag_IsWritable |
                   kAudioUnitParameterFlag_IsReadable);
}

Param::~Param()
{}

Param& Param::name(const char *name)
{
    mIndex = next_param_index++;
    if (current_paramset->mIndex[mIndex] != this) {
        fprintf(stderr, "Param::name(\"%s\"): expected \"%s %s\"\n",
                name, "", current_paramset->mIndex[mIndex]->mInfo.name);
        assert(current_paramset->mIndex[mIndex] == this);
    }
    mInfo.flags |= kAudioUnitParameterFlag_HasCFNameString;
    strlcpy(mInfo.name, name, sizeof mInfo.name);
    mInfo.cfNameString = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
                                                         mInfo.name,
                                                         kCFStringEncodingUTF8,
                                                         kCFAllocatorNull);
    if (current_clump) {
        mInfo.clumpID = current_clump->id();
        mInfo.flags |= kAudioUnitParameterFlag_HasClump;
    }
    return *this;
}

Param& Param::min_max(float min_value, float max_value)
{
    mInfo.minValue = min_value;
    mInfo.maxValue = max_value;
    return *this;
}

Param& Param::default_value(float value)
{
    mInfo.defaultValue = value;
    return *this;
}

Param& Param::value_string(int value, const char *string)
{
    ValueStringMap vsm = {value, string};
    mValueStrings.push_back(vsm);
    units(kAudioUnitParameterUnit_Indexed);
    return min_max(0, mValueStrings.size() - 1);
}

Param& Param::value_strings(const ValueStringMap *p)
{
    for ( ; p->string; p++)
        value_string(p->value, p->string);
    return *this;
}

Param& Param::units(AudioUnitParameterUnit units)
{
    mInfo.unit = units;
    return *this;
}

Param& Param::flag(UInt32 flag)
{
    mInfo.flags |= flag;
    return *this;
}

Param& Param::assigns_mod(int modulator)
{
    mModIndex = modulator;
    return *this;
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark FloatParam Class

FloatParam::FloatParam()
: mFloatValue(0)
{}

float FloatParam::get_value() const
{
    return mFloatValue;
}

OSStatus FloatParam::set_value(float new_value)
{
    mFloatValue = new_value;
    return noErr;
}

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark EnumParam Classes

// An enumerated parameter has "mapped values" which the renderer uses
// and "unmapped values" which the host uses.  E.g., waveform
// params have Oscillator::Saw as the first value, so the mapped value
// is Oscillator::Saw, and the unmapped value is 0.

EnumParamBase::EnumParamBase()
: mUnmappedValue(0),
  mMappedValue(0)
{}

Param& EnumParamBase::default_value(float value)
{
    int ival = (int)(value + 0.5);
    for (size_t i = 0; i < mValueStrings.size(); i++) {
        if (ival == mValueStrings[i].value) {
            Param::default_value((float)i);
            return *this;
        }
    }
    assert(false && "illegal default value");
    return *this;
}

float EnumParamBase::get_value() const
{
    return mUnmappedValue;
}

OSStatus EnumParamBase::set_value(float new_value)
{
    // AU Lab gives us indexed params as 0/127, 1/127, etc.
    // We scale them up here.

    size_t size = mValueStrings.size();
    if (new_value < 1.0 && size > 1)
        new_value *= 127.0 / (size - 1);
    int intval = (int)(new_value + 0.5);
    if (intval < 0 || intval >= size) {
        printf("%s:%u: param %u.%s, new_value=%g size=%zu intval=%d\n",
               __func__, __LINE__,
               mInfo.clumpID,
               CFStringGetCStringPtr(mInfo.cfNameString,
                                     kCFStringEncodingUTF8),
               new_value, size, intval);
        return noErr;
    }

    mUnmappedValue = intval;
    mMappedValue = mValueStrings[intval].value;
    return noErr;
}

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark ParamClump

ParamClump::ParamClump(const char *name, const char *abbrev)
    : mID(next_clump_ID++),
      mName(name),
      mAbbrev(abbrev)
{
    assert(!current_clump);
    current_clump = this;
    assert(current_paramset);
    current_paramset->mClumpIndex.push_back(*this);
}

ParamClump::~ParamClump()
{
    if (current_clump == this)
        current_clump = NULL;
}

UInt32 ParamClump::id() const
{
    return mID;
}

const char *ParamClump::name() const
{
    return mName;
}

const char *ParamClump::abbrev() const
{
    return mAbbrev;
}

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
#pragma mark ParamSet

ParamSet::ParamSet()
{
    current_paramset = this;
    next_clump_ID = kAudioUnitClumpID_System + 1;
    next_param_index = 0;
    current_clump = NULL;
}

size_t ParamSet::size() const
{
    return mIndex.size();
}

float ParamSet::param_value(size_t index) const
{
    return mIndex[index]->get_value();
}

const AudioUnitParameterInfo& ParamSet::param_info(size_t index) const
{
    return mIndex[index]->mInfo;
}

const CFArrayRef ParamSet::param_value_strings(size_t index) const
{
    if (index >= mIndex.size())
        return NULL;
    Param *param = mIndex[index];
    Param::ValueStrings *p = &param->mValueStrings;
    if (p->empty())
        return NULL;
    CFMutableArrayRef array = CFArrayCreateMutable(kCFAllocatorDefault,
                                                   p->size(),
                                                   NULL);
    for (size_t i = 0; i < p->size(); i++) {
        CFStringRef string = CFStringCreateWithCString(kCFAllocatorDefault,
                                                       (*p)[i].string,
                                                       kCFStringEncodingUTF8);
        CFArrayAppendValue(array, string);
    }
    return array;
}

const char *ParamSet::clump_name(UInt32 id) const
{
    size_t index = id - (kAudioUnitClumpID_System + 1);
    if (index < mClumpIndex.size())
        return mClumpIndex[index].name();
    return NULL;
}

int ParamSet::param_mod_source(size_t index) const
{
    if (index >= mIndex.size())
        return 0;
    return mIndex[index]->mModIndex;
}

int ParamSet::param_mod_dest(size_t index) const
{
    if (EnumParamBase *eparam = dynamic_cast<EnumParamBase *>(mIndex[index]))
        return eparam->mMappedValue;
    return 0;
}

void ParamSet::set_defaults()
{
    for (size_t i = 0; i < size(); i++) {
        Param *p = mIndex[i];
        if (!p->mInfo.name[0])
            fprintf(stderr, "Parameter %zu name not set\n", i);
        assert(p->mInfo.name[0]);
    }
    for (ParamVec::iterator p = mIndex.begin(); p != mIndex.end(); ++p)
        (*p)->set_value((*p)->mInfo.defaultValue);
}

OSStatus ParamSet::set_param_value(size_t index, float new_value)
{
    if (index >= mIndex.size())
        return kAudioUnitErr_InvalidProperty;
    return mIndex[index]->set_value(new_value);
}
