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
enum {
    kParameter_AmpAttackTime   = 0,
    kParameter_AmpDecayTime    = 1,
    kParameter_AmpSustainLevel = 2,
    kParameter_AmpReleaseTime  = 3,
    kNumberOfParameters
};

// Define constants to identify factory presets.
enum {
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

private:
    MVSNote mNotes[kNumNotes];

};

#endif /* !__MVS__ */
