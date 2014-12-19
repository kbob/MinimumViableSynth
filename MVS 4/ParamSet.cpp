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


#pragma mark Param Initialization

Param::Param(Type t)
    : mType(t),
      mIndex(-1)
{
    assert(current_paramset);
    current_paramset->mIndex.push_back(this);
    memset(&mInfo, 0, sizeof mInfo);
    mInfo.flags = (kAudioUnitParameterFlag_IsWritable |
                   kAudioUnitParameterFlag_IsReadable);
}

Param& Param::name(const char *name)
{
    mIndex = next_param_index++;
    if (current_paramset->mIndex[mIndex] != this)
        fprintf(stderr, "Param::name(\"%s\"): expected \"%s %s\"\n",
                name, "", current_paramset->mIndex[mIndex]->mInfo.name);
    assert(current_paramset->mIndex[mIndex] == this);
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
    assert(mType == Int);
    ValueStringMap vsm = {value, string};
    mValueStrings.push_back(vsm);
    units(kAudioUnitParameterUnit_Indexed);
    return min_max(0, mValueStrings.size() - 1);
}

Param& Param::value_strings(const ValueStringMap *p)
{
    assert(mType == Int);
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


#pragma mark Param Assignment

void Param::set_value(float new_value)
{
    mValue = new_value;
}

#pragma mark FloatParam, IntParam

FloatParam::FloatParam()
    : Param(Param::Float)
{}

void FloatParam::set_value(float new_value)
{
    mValue = new_value;
}

IntParam::IntParam()
    : Param(Param::Int)
{}

void IntParam::set_value(float new_value)
{
    // AU Lab gives us indexed params as 0/127, 1/127, etc.
    // We scale them up here.
    if (!mValueStrings.empty()) {
        if (new_value < 1.0 && mValueStrings.size() > 1)
            new_value *= 127.0 / (mValueStrings.size() - 1);
    }
    mValue = new_value;
    mIntValue = (int)(new_value + 0.5);
}


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
    return mIndex[index]->mValue;
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

void ParamSet::set_param_value(size_t index, float new_value)
{
    mIndex[index]->set_value(new_value);
}
