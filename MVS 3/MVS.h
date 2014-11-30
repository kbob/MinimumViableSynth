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
#include "Oscillator.h"
#include "Envelope.h"

static const UInt32 kNumNotes = 12;

// Define constants to identify the parameters;
// define the total number of parameters.
enum Parameter {
    kParameter_Osc1Waveform        = 0,
    kParameter_Osc1WaveMod         = 1,
    kParameter_Osc1VibratoDepth    = 2,
    kParameter_Osc1VibratoSpeed    = 3,
    kParameter_Osc1VibratoWaveform = 4,
    kParameter_AmpAttackTime       = 5,
    kParameter_AmpDecayTime        = 6,
    kParameter_AmpSustainLevel     = 7,
    kParameter_AmpReleaseTime      = 8,
    kNumberOfParameters
};

enum Waveform {
    kWaveform_Saw,
    kWaveform_Square,
    kWaveform_Triangle,
    kWaveform_Sine,
    kNumberOfWaveforms
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
    Envelope         mAmpEnv;

    void             FillWithConstant(
                                Float32           k,
                                Float32          *buf,
                                UInt32            count);

    void             CVtoPhase (Float64           baseFreq,
                                Float32           cvDepth,
                                Float32          *buf,
                                UInt32            count);

    Oscillator::Type OscillatorType(
                                    Waveform waveform);



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
