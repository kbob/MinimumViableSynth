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

struct ValueStringMap {
    int         value;
    const char *string;
};

class Param {

public:

    // Initialization
            Param&         name          (const char *);
            Param&         min_max       (float, float);
    virtual Param&         default_value (float);
            Param&         value_string  (int value, const char *string);
            Param&         value_strings (const ValueStringMap *);
            Param&         units         (AudioUnitParameterUnit);
            Param&         flag          (UInt32);

    // Get/Set
    virtual float          get_value     () const = 0;
    virtual OSStatus       set_value     (float new_value) = 0;

protected:

                           Param();
    virtual               ~Param();

    typedef std::vector<ValueStringMap> ValueStrings;

    ValueStrings           mValueStrings;

private:

    friend class           ParamSet;

    int                    mIndex;
    AudioUnitParameterInfo mInfo;

    Param(const Param&);                // disallow copy/assign
    void operator = (const Param&);

};

class FloatParam : public Param {

public:

                     FloatParam     ();

                     operator float () const { return mFloatValue; }

    virtual float    get_value      () const;
    virtual OSStatus set_value      (float new_value);

private:

    float            mFloatValue;

};

class EnumParamBase : public Param {

public:

    virtual Param&   default_value (float);

    virtual float    get_value     () const;
    virtual OSStatus set_value     (float new_value);

protected:

                     EnumParamBase ();

    int              mUnmappedValue;
    int              mMappedValue;
};

template <class E> class EnumParam : public EnumParamBase {

public:

    operator E () const { return (E)mMappedValue; }
    
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
    OSStatus                      set_param_value (size_t index, float value);

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
