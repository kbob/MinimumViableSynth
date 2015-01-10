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

#include "Amplifier.h"
#include "Decimator.h"
#include "Envelope.h"
#include "Filter.h"
#include "LFO.h"
#include "Mixer.h"
#include "ModMatrix.h"
#include "ModWheel.h"
#include "NoiseSource.h"
#include "Oscillator.h"
#include "ParamSet.h"

static const UInt32 kNumNotes = 24;

// Define constants to identify factory presets.
enum Preset {
    kPreset_Default = 0,
    kNumberOfPresets
};

// Define the presets.
static AUPreset kPresets [kNumberOfPresets] = {
    { kPreset_Default, CFSTR("Factory Default") },
};

class Mod {

public:

    enum Source {
        NullSource,             // must skip zero

        Wheel,
        LFO1,
        LFO2,
        Env2,

        SourceCount             // must be last
    };

    enum Destination {
        NoDest,                 // must skip zero

        Osc1Freq,
        Osc1Width,

        Osc2Freq,
        Osc2Width,

        Osc1Level,
        Osc2Level,
        NoiseLevel,

        FltCutoff,
        FltResonance,
        FltDrive,
        FltKeyTrack,

        AmpLevel,
        AmpAttack,
        AmpDecay,
        AmpSustain,
        AmpRelease,

        LFO1Amount,
        LFO1Speed,

        LFO2Amount,
        LFO2Speed,

        Env2Attack,
        Env2Decay,
        Env2Sustain,
        Env2Release,
        Env2Amount,

        DestinationCount        // must be last
    };

};

typedef ModMatrix<Mod::SourceCount, Mod::DestinationCount> MVSModMatrix;
typedef ModBox   <Mod::SourceCount, Mod::DestinationCount> MVSModBox;

class MVSParamSet : public ParamSet {

public:

    // All the synth's user parameters are declared here.
    // The MVSParamSet constructor loads all their settings
    // (name string, min/max, default value, etc.).

    MVSParamSet();

    EnumParam<Oscillator::Waveform> o1_waveform;
    FloatParam                      o1_width;

    FloatParam                      o2_coarse_detune;
    FloatParam                      o2_fine_detune;
    EnumParam<Oscillator::Waveform> o2_waveform;
    FloatParam                      o2_width;

    EnumParam<NoiseSource::Type>    noise_spectrum;

    EnumParam<Mixer::Operator>      mix_operator;
    FloatParam                      mix_osc1_level;
    FloatParam                      mix_osc2_level;
    FloatParam                      mix_noise_level;

    EnumParam<Filter::Type>         flt_type;
    FloatParam                      flt_cutoff;
    FloatParam                      flt_resonance;
    FloatParam                      flt_drive;
    FloatParam                      flt_keytrack;

    FloatParam                      amp_attack;
    FloatParam                      amp_decay;
    FloatParam                      amp_sustain;
    FloatParam                      amp_release;
    FloatParam                      amp_amount;

    EnumParam<Mod::Destination>     mw_destination;
    FloatParam                      mw_amount;

    EnumParam<LFO::Waveform>        lfo1_waveform;
    FloatParam                      lfo1_speed;
    FloatParam                      lfo1_amount;
    EnumParam<Mod::Destination>     lfo1_destination;

    EnumParam<LFO::Waveform>        lfo2_waveform;
    FloatParam                      lfo2_speed;
    FloatParam                      lfo2_amount;
    EnumParam<Mod::Destination>     lfo2_destination;

    FloatParam                      env2_attack;
    FloatParam                      env2_decay;
    FloatParam                      env2_sustain;
    FloatParam                      env2_release;
    FloatParam                      env2_keytrack;
    FloatParam                      env2_amount;
    EnumParam<Mod::Destination>     env2_destination;
};

class MVSNote : public SynthNote {

public:

                       MVSNote();
    virtual           ~MVSNote() {}

            void       SetOversampleParams(
                                  UInt32 ratio,
                                  Float32 **bufPtr);
            void       AffixParams(const MVSParamSet *params);
            void       AffixModBox(const MVSModBox **boxPtrPtr);

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
    MVSModBox  const **mModBoxPtrPtr;
    MVSParamSet const *mParams;
    float              mGain;
    Envelope           mEnv1;
    Envelope           mEnv2;
    Oscillator         mOsc1;
    Oscillator         mOsc2;
    NoiseSource        mNoise;
    Mixer              mMixer;
    Filter             mFilter;
    Amplifier          mAmplifier;

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

    virtual OSStatus   GetPresets(
                                CFArrayRef             *outData) const;

    virtual OSStatus   NewFactoryPresetSet(
                                const AUPreset         &inNewFactoryPreset);

    virtual OSStatus   RestoreState(
                                CFPropertyListRef		inData);

    // override AUInstrumentBase to collect ModWheel changes.
    virtual OSStatus   HandleControlChange(
                                UInt8	                inChannel,
                                UInt8 	                inController,
                                UInt8 	                inValue,
                                UInt32	                inStartFrame);

    virtual OSStatus   Render  (AudioUnitRenderActionFlags &ioActionFlags,
                                const AudioTimeStamp       &inTimeStamp,
                                UInt32                     inNumberFrames);

    virtual void       RunModulators(MVSModBox&         ioModBox);

private:
    MVSParamSet        mParams;
    unsigned           mOversampleRatio;
    Float32           *mOversampleBufPtr;
    MVSModMatrix       mModMatrix;
    MVSModBox const   *mModBoxPtr;
    Decimator          mDecimator;
    ModWheel           mModWheel;
    LFO                mLFO1;
    LFO                mLFO2;

    MVSNote            mNotes[kNumNotes];

};

#endif /* !__MVS__ */
