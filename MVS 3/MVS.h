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
#include "NoiseSource.h"
#include "Oscillator.h"
#include "Envelope.h"

static const UInt32 kNumNotes = 12;

enum ParameterClump {
    kParamClump_Osc1 = kAudioUnitClumpID_System + 1,
    kParamClump_Osc2,
    kParamClump_Noise,
    kParamClump_Mixer,
    kParamClump_Filter,
    kParamClump_Amplitude,
    kNumberOfParamClumps
};

// Define constants to identify the parameters;
// define the total number of parameters.

enum Parameter {

    // Oscillator 1
    kParameter_Osc1Waveform        =  0,
    kParameter_Osc1WaveSkew        =  1,
    kParameter_Osc1VibratoDepth    =  2,
    kParameter_Osc1VibratoSpeed    =  3,
    kParameter_Osc1VibratoWaveform =  4,
//    kParameter_Osc1VibratoSkew     =  ?,

//    // Oscillator 2 (coming soon)
//    kParameter_Osc2Detune          =  ?,
//    kParameter_Osc2Waveform        =  ?,
//    kParameter_Osc2WaveSkew        =  ?,
//    kParameter_Osc2VibratoDepth    =  ?,
//    kParameter_Osc2VibratoSpeed    = ??,
//    kParameter_Osc2VibratoWaveform = ??,
//    kParameter_Osc2VibratoSkew     = ??,

    // Noise Source
    kParameter_NoiseType           =  5,
    kParameter_NoiseAttackTime     =  6,
    kParameter_NoiseDecayTime      =  7,
    kParameter_NoiseSustainLevel   =  8,
    kParameter_NoiseReleaseTime    =  9,

    // Mixer
    //    kParameter_MixType             = ??,  // Mix, RingMod, HardSync
    kParameter_Osc1Level           = 10,
    kParameter_Osc2Level           = 11,
    kParameter_NoiseLevel          = 12,


//    // Filter (coming soon)
//    kParameter_FilterType          = ??;
//    kParameter_FiltCutoff          = ??,
//    kParameter_FiltResonance       = ??,
//    kParameter_FiltDrive           = ??,
//    kParameter_FiltKeytrack        = ??,

//    kParameter_FiltEnvCutoffDepth  = ??,
//    kParameter_FiltEnvResDepth     = ??,
//    kParameter_FiltEnvDriveDepth   = ??,
//    kParameter_FiltAttackTime      = ??,
//    kParameter_FiltDecayTime       = ??,
//    kParameter_FiltSustainlevel    = ??,
//    kParameter_FiltReleaseTime     = ??,

//    kParameter_FiltLFOCutoffDepth  = ??,
//    kParameter_FiltLFOResDepth     = ??,
//    kParameter_FiltLFODriveDepth   = ??,
//    kParameter_FiltLFOSpeed        = ??,
//    kParameter_FiltLFOWaveform     = ??,
//    kParameter_FiltLFOSkew         = ??,

    // Amplitude
    kParameter_AmpAttackTime       = 13,
    kParameter_AmpDecayTime        = 14,
    kParameter_AmpSustainLevel     = 15,
    kParameter_AmpReleaseTime      = 16,
    kNumberOfParameters
};

enum Waveform {
    kWaveform_Saw,
    kWaveform_Square,
    kWaveform_Triangle,
    kWaveform_Sine,
    kNumberOfWaveforms
};

enum NoiseType {
    kNoiseType_White,
    kNoiseType_Pink,
    kNoiseType_Red,
    kNumberOfNoiseTypes
};

// Define constants to identify factory presets.
enum Preset {
    kPreset_Default = 0,
    kNumberOfPresets
};

// Define the presets.
static AUPreset kPresets [kNumberOfPresets] = {
    { kPreset_Default, CFSTR("Factory Default") },
};


class MVSNote : public SynthNote {

public:
                     MVSNote();
    virtual         ~MVSNote() {}

            void     SetOversampleParams(
                                UInt32 ratio,
                                Float32 **bufPtr);

    virtual bool     Attack    (const MusicDeviceNoteParams &inParams);

    virtual void     Release   (UInt32 inFrame);
    virtual void     FastRelease(UInt32 inFrame);
    virtual Float32  Amplitude ();
    virtual OSStatus Render    (UInt64            inAbsoluteSampleFrame,
                                UInt32            inNumFrames,
                                AudioBufferList **inBufferList,
                                UInt32            inOutBusCount);
    virtual Float64  SampleRate();

private:
    UInt32           mOversampleRatio;
    Float32        **mOversampleBufPtr;
    Oscillator       mOsc1;
    Oscillator       mOsc1LFO;
    NoiseSource      mNoise;
    Envelope         mNoiseEnv;
    Envelope         mAmpEnv;

    void             FillWithConstant(
                                Float32           k,
                                Float32          *buf,
                                UInt32            count);

    void             CVtoPhase (Float64           baseFreq,
                                Float32           cvDepth,
                                Float32          *buf,
                                UInt32            count);

    // enum converters
    Oscillator::Type OscillatorType(
                                Waveform          waveform);
    NoiseSource::Type NoiseType(NoiseType         ntype);


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
    Float32           *mOversampleBufPtr;
    Decimator          mDecimator;
    MVSNote            mNotes[kNumNotes];

};

#endif /* !__MVS__ */
