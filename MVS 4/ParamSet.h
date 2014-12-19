//
//  ParamSet.h
//  MVS
//
//  Created by Bob Miller on 12/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS__ParamSet__
#define __MVS__ParamSet__

#include <vector>

#include <stdio.h>

//#include "AUInstrumentBase.h"

//class Clump;
//class Param;
//typedef std::vector<Clump> ClumpVector;
//typedef std::vector<Param> ParamVector;

struct ValueStringMap {
    int         value;
    const char *string;
};

class Param {

public:

    // Initialization
    Param&                 name          (const char *);
    Param&                 min_max       (float, float);
    Param&                 default_value (float);
    Param&                 value_string  (int value, const char *string);
    Param&                 value_strings (const ValueStringMap *);
    Param&                 units         (AudioUnitParameterUnit);
    Param&                 flag          (UInt32);

    virtual void           set_value     (float new_value) = 0;

protected:

    enum Type {
        Float,
        Int,
    };

    typedef std::vector<ValueStringMap> ValueStrings;

    float                  mValue;
    ValueStrings           mValueStrings;

    Param(Type t);

private:

    friend class           ParamSet;

    Type                   mType;
    int                    mIndex;
    AudioUnitParameterInfo mInfo;

};

class FloatParam : public Param {

public:

                 FloatParam     ();

                 operator float () const { return mValue; }

    virtual void set_value      (float new_value);


};

class IntParam : public Param {

public:

                 IntParam     ();

                 operator int () const { return mValue; }

    virtual void set_value    (float new_value);

protected:

    int mIntValue;

};

template <class E> class EnumParam : public IntParam {

public:

    operator E () const
    {
        return (E)mValueStrings[mIntValue].value;
    }
    
};

class ParamClump {

public:

                ParamClump (const char *name, const char *abbrev);
               ~ParamClump ();

    const char *name       () const;
    const char *abbrev     () const;
    UInt32      id         () const;

private:

    UInt32      mID;
    const char *mName;
    const char *mAbbrev;

};

class ParamSet {

public:

    size_t size() const;

    float                         param_value            (size_t index) const;
    const AudioUnitParameterInfo& param_info             (size_t index) const;
    const CFArrayRef              param_value_strings    (size_t index) const;
    const char                   *clump_name             (UInt32 id)    const;

    void                          set_defaults    ();
    void                          set_param_value (size_t index, float value);

protected:

    ParamSet();

private:

    friend class Param;
    friend class ParamClump;

    typedef std::vector<Param *>    ParamVec;
    typedef std::vector<ParamClump> ClumpVec;

    ParamVec mIndex;
    ClumpVec mClumpIndex;

};

#endif /* defined(__MVS__ParamSet__) */
