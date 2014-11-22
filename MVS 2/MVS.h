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

#include "Oscillator.h"
#include "Envelope.h"

static const UInt32 kNumNotes = 12;

// Define constants to identify the parameters;
// define the total number of parameters.
enum Parameter {
    kParameter_Osc1Waveform    = 0,
    kParameter_Osc1WaveMod     = 1,
    kParameter_AmpAttackTime   = 3,
    kParameter_AmpDecayTime    = 4,
    kParameter_AmpSustainLevel = 5,
    kParameter_AmpReleaseTime  = 6,
    kNumberOfParameters
};

enum Waveform {
    kWaveform_Sine,
    kWaveform_Saw,
    kWaveform_Pulse,
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
    virtual         ~MVSNote() {}

    virtual bool     Attack(const MusicDeviceNoteParams &inParams);

    virtual void     Release(UInt32 inFrame);
    virtual void     FastRelease(UInt32 inFrame);
    virtual Float32  Amplitude();
    virtual OSStatus Render(UInt64            inAbsoluteSampleFrame,
                            UInt32            inNumFrames,
                            AudioBufferList **nBufferList,
                            UInt32            inOutBusCount);

private:
    Oscillator mOsc1;
    Envelope   mAmpEnv;

};

class MVS : public AUMonotimbralInstrumentBase {

public:
                       MVS(AudioUnit inComponentInstance);
    virtual           ~MVS();
                                
    virtual OSStatus   Initialize();
    virtual void       Cleanup();
    virtual OSStatus   Version() { return kMVSVersion; }

    virtual AUElement *CreateElement(AudioUnitScope  scope,
                                     AudioUnitElement element);

    virtual OSStatus   GetParameterInfo(
                                AudioUnitScope          inScope,
                                AudioUnitParameterID    inParameterID,
                                AudioUnitParameterInfo &outParameterInfo);

    virtual	OSStatus   GetParameterValueStrings(
                                AudioUnitScope       inScope,
                                AudioUnitParameterID inParameterID,
                                CFArrayRef          *outStrings);

    virtual OSStatus   SetParameter(
                                AudioUnitParameterID    inID,
                                AudioUnitScope          inScope,
                                AudioUnitElement        inElement,
                                AudioUnitParameterValue inValue,
                                UInt32                  inBufferOffsetInFrames);

private:
    MVSNote mNotes[kNumNotes];

};

#endif /* !__MVS__ */
