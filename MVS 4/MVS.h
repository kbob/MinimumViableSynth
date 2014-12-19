//
//  MVS.h
//  MVS - Minimum Viable Synth
//
//  Created by Bob Miller on 11/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS__
#define __MVS__

#include "AUInstrumentBase.h"
#include "MVSVersion.h"

#include "Decimator.h"
#include "Envelope.h"
#include "NoiseSource.h"
#include "Oscillator.h"
#include "ParamSet.h"

static const UInt32 kNumNotes = 500;

// Define constants to identify factory presets.
enum Preset {
    kPreset_Default = 0,
    kNumberOfPresets
};

// Define the presets.
static AUPreset kPresets [kNumberOfPresets] = {
    { kPreset_Default, CFSTR("Factory Default") },
};

class MVSParamSet : public ParamSet {

public:

    // All the synth's user parameters are declared here.
    // The MVSParamSet constructor loads all their settings
    // (name string, min/max, default value, etc.).

    MVSParamSet();

    EnumParam<Oscillator::Type>   o1_waveform;
    FloatParam o1_width;

    FloatParam o2_coarse_detune;
    FloatParam o2_fine_detune;
    IntParam   o2_waveform;
    FloatParam o2_width;

    IntParam   noise_spectrum;

    IntParam   mix_operator;
    FloatParam mix_osc1_level;
    FloatParam mix_osc2_level;
    FloatParam mix_noise_level;

    IntParam   flt_type;
    FloatParam flt_cutoff;
    FloatParam flt_resonance;
    FloatParam flt_drive;
    FloatParam flt_keytrack;

    FloatParam amp_attack;
    FloatParam amp_decay;
    FloatParam amp_sustain;
    FloatParam amp_release;

    IntParam   mw_assign;
    FloatParam mw_amount;

    IntParam   lfo1_waveform;
    FloatParam lfo1_speed;
    FloatParam lfo1_amount;
    IntParam   lfo1_assign;

    IntParam   lfo2_waveform;
    FloatParam lfo2_speed;
    FloatParam lfo2_amount;
    IntParam   lfo2_assign;

    FloatParam env2_attack;
    FloatParam env2_decay;
    FloatParam env2_sustain;
    FloatParam env2_release;
    FloatParam env2_amount;
    IntParam   env2_assign;
};

class MVSNote : public SynthNote {

public:

                       MVSNote();
    virtual           ~MVSNote() {}

            void       SetOversampleParams(
                                  UInt32 ratio,
                                  Float32 **bufPtr);
            void       AffixParams(const MVSParamSet *params);

    virtual bool       Attack    (const MusicDeviceNoteParams &inParams);

    virtual void       Release   (UInt32 inFrame);
    virtual void       FastRelease(UInt32 inFrame);
    virtual Float32    Amplitude ();
    virtual OSStatus   Render    (UInt64            inAbsoluteSampleFrame,
                                  UInt32            inNumFrames,
                                  AudioBufferList **inBufferList,
                                  UInt32            inOutBusCount);
    virtual Float64    SampleRate();

private:

    UInt32             mOversampleRatio;
    Float32          **mOversampleBufPtr;
    MVSParamSet const *mParams;
    Oscillator         mOsc1;
    Oscillator         mOsc2;
    NoiseSource        mNoise;
    Envelope           mAmpEnv;

    void               FillWithConstant(
                                  Float32           k,
                                  Float32          *buf,
                                  UInt32            count);

    void               CVtoPhase (Float64           baseFreq,
                                  Float32           cvDepth,
                                  Float32          *buf,
                                  UInt32            count);

};

class MVS : public AUMonotimbralInstrumentBase {

public:
                       MVS     (AudioUnit inComponentInstance);
    virtual           ~MVS     ();
                                
    virtual OSStatus   Initialize();
    virtual void       Cleanup ();
    virtual OSStatus   Version () { return kMVSVersion; }

    virtual AUElement *CreateElement(
                                AudioUnitScope          scope,
                                AudioUnitElement        element);

    virtual OSStatus   GetParameterInfo(
                                AudioUnitScope          inScope,
                                AudioUnitParameterID    inParameterID,
                                AudioUnitParameterInfo &outParameterInfo);

    virtual OSStatus   GetParameterValueStrings(
                                AudioUnitScope          inScope,
                                AudioUnitParameterID    inParameterID,
                                CFArrayRef             *outStrings);

    virtual OSStatus   CopyClumpName(
                                AudioUnitScope          inScope,
                                UInt32                  inClumpID,
                                UInt32                  inDesiredNameLength,
                                CFStringRef            *outClumpName);

    virtual OSStatus   SetParameter(
                                AudioUnitParameterID    inID,
                                AudioUnitScope          inScope,
                                AudioUnitElement        inElement,
                                AudioUnitParameterValue inValue,
                                UInt32                  inBufferOffsetInFrames);

    virtual OSStatus   Render  (AudioUnitRenderActionFlags &ioActionFlags,
                                const AudioTimeStamp       &inTimeStamp,
                                UInt32                     inNumberFrames);

private:
    MVSParamSet        mParams;
    Float32           *mOversampleBufPtr;
    Decimator          mDecimator;
    MVSNote            mNotes[kNumNotes];

    void               CreateParameters();

};

#endif /* !__MVS__ */
