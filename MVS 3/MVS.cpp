//
//  MVS.cpp
//  MVS - Minimum Viable Synth
//
//  Created by Bob Miller on 11/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

//  This is the Minimum Viable Synth.  It has a single sawtooth
//  oscillator and an ADSR envelope generator.  The ADSR parameters
//  are available as AudioUnitParameters and can also be mapped to
//  MIDI controls.

#include "MVS.h"
 
static const UInt32  kMaxActiveNotes = 18;
static const UInt32  kOversampleRatio = 4;
static const Float64 kDecimatorPassFreq = 20000.0;

static CFStringRef kParamName_Osc1Waveform   = CFSTR("Oscillator 1 Waveform");
static CFStringRef kParamName_Osc1WaveMod    = CFSTR("Oscillator 1 Modifier");
static CFStringRef kParamName_AmpAttackTime  = CFSTR("Amplitude Attack Time");
static CFStringRef kParamName_AmpDecayTime   = CFSTR("Amplitude Decay Time");
static CFStringRef kParamName_AmpSustainLevel= CFSTR("Amplitude Sustain Level");
static CFStringRef kParamName_AmpReleaseTime = CFSTR("Amplitude Release Time");

static CFStringRef kMenuItem_Waveform_Saw      = CFSTR ("Sawtooth");
static CFStringRef kMenuItem_Waveform_Square   = CFSTR ("Square/Pulse");
static CFStringRef kMenuItem_Waveform_Triangle = CFSTR ("Triangle");
static CFStringRef kMenuItem_Waveform_Sine     = CFSTR ("Sine");

////////////////////////////////////////////////////////////////////////////////

#pragma mark MVS Methods

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, MVS)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//      MVS::MVS
//
// This synth has No inputs, One output
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MVS::MVS(AudioUnit inComponentInstance)
: AUMonotimbralInstrumentBase(inComponentInstance, 0, 1),
  mOversampleBufPtr(NULL)
{
    CreateElements();

    Globals()->UseIndexedParameters(kNumberOfParameters);
    Globals()->SetParameter(kParameter_Osc1Waveform,    kWaveform_Saw);
    Globals()->SetParameter(kParameter_AmpAttackTime,   0.001);
    Globals()->SetParameter(kParameter_AmpDecayTime,    0.100);
    Globals()->SetParameter(kParameter_AmpSustainLevel, 1.000);
    Globals()->SetParameter(kParameter_AmpReleaseTime,  0.050);

    SetAFactoryPresetAsCurrent(kPresets[kPreset_Default]);

    for (size_t i = 0; i < kNumNotes; i++)
        mNotes[i].SetOversampleParams(kOversampleRatio, &mOversampleBufPtr);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//      MVS::~MVS
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MVS::~MVS()
{}

void MVS::Cleanup()
{}

OSStatus MVS::Initialize()
{       
    AUMonotimbralInstrumentBase::Initialize();
        
    SetNotes(kNumNotes, kMaxActiveNotes, mNotes, sizeof(MVSNote));

    Float64 sampleRate = GetOutput(0)->GetStreamFormat().mSampleRate;
    mDecimator.initialize(sampleRate, kDecimatorPassFreq, kOversampleRatio);

    return noErr;
}

AUElement* MVS::CreateElement(AudioUnitScope   scope,
                              AudioUnitElement element)
{
    switch (scope) {

    case kAudioUnitScope_Group:
        return new SynthGroupElement(this, element, new MidiControls);

    case kAudioUnitScope_Part:
        return new SynthPartElement(this, element);

    default:
        return AUBase::CreateElement(scope, element);
    }
}

OSStatus MVS::GetParameterInfo(AudioUnitScope          inScope,
                               AudioUnitParameterID    inParameterID,
                               AudioUnitParameterInfo &outParameterInfo)
{
    AudioUnitParameterInfo &info = outParameterInfo;
    info.flags = (kAudioUnitParameterFlag_IsWritable |
                  kAudioUnitParameterFlag_IsReadable);

    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    switch (inParameterID) {

    case kParameter_Osc1Waveform:
        AUBase::FillInParameterName(info, kParamName_Osc1Waveform, false);
        info.unit         = kAudioUnitParameterUnit_Indexed;
        info.minValue     = 0;
        info.maxValue     = kNumberOfWaveforms - 1;
        info.defaultValue = kWaveform_Saw;
        break;

    case kParameter_Osc1WaveMod:
        AUBase::FillInParameterName(info, kParamName_Osc1WaveMod, false);
        info.unit         = kAudioUnitParameterUnit_Percent;
        info.minValue     =   0.0;
        info.maxValue     = 100.0;
        info.defaultValue =   0.0;
//      info.flags       |= kAudioUnitParameterFlag_DisplayLogarithmic;
        break;

    case kParameter_AmpAttackTime:
        AUBase::FillInParameterName(info, kParamName_AmpAttackTime, false);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     = 0.001;
        info.maxValue     = 2.00;
        info.defaultValue = 0.001;
//      info.flags       |= kAudioUnitParameterFlag_DisplayLogarithmic;
        break;

    case kParameter_AmpDecayTime:
        AUBase::FillInParameterName(info, kParamName_AmpDecayTime, false);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     = 0.001;
        info.maxValue     = 2.00;
        info.defaultValue = 0.10;
//      info.flags       |= kAudioUnitParameterFlag_DisplayLogarithmic;
        break;

    case kParameter_AmpSustainLevel:
        AUBase::FillInParameterName(info,
                                    kParamName_AmpSustainLevel,
                                    false);
        info.unit         = kAudioUnitParameterUnit_LinearGain;
        info.minValue     = 0.0;
        info.maxValue     = 1.0;
        info.defaultValue = 1.0;
        info.flags       |= 0;
        break;

    case kParameter_AmpReleaseTime:
        AUBase::FillInParameterName(info, kParamName_AmpReleaseTime, false);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     = 0.001;
        info.maxValue     = 2.000;
        info.defaultValue = 0.050;
//        info.flags       |= kAudioUnitParameterFlag_DisplayLogarithmic;
        break;

    default:
        return kAudioUnitErr_InvalidParameter;
    }

    return noErr;
}

OSStatus MVS::GetParameterValueStrings(AudioUnitScope       inScope,
                                       AudioUnitParameterID inParameterID,
                                       CFArrayRef          *outStrings)
{
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    if (inParameterID == kParameter_Osc1Waveform) {
        if (outStrings == NULL)
            return noErr;

        // Defines an array that contains the pop-up menu item names.
        CFStringRef strings [] = {
            kMenuItem_Waveform_Saw,
            kMenuItem_Waveform_Square,
            kMenuItem_Waveform_Triangle,
            kMenuItem_Waveform_Sine,
        };

        // Create a new immutable array containing the menu item names
        // and place the array in the outStrings output parameter.
        *outStrings = CFArrayCreate (
                                     NULL,
                                     (const void **) strings,
                                     sizeof strings / sizeof strings[0],
                                     NULL
                                     );
        return noErr;
    }
    return kAudioUnitErr_InvalidParameter;

}

OSStatus MVS::SetParameter(AudioUnitParameterID    inID,
                           AudioUnitScope          inScope,
                           AudioUnitElement        inElement,
                           AudioUnitParameterValue inValue,
                           UInt32                  inBufferOffsetInFrames)
{
    // Controller sends CC values 0/127, 1/127, 2/127.  We map those to [0..2].
    if (inID == kParameter_Osc1Waveform && inValue < 1.0)
        inValue *= 127.0 / (kNumberOfWaveforms - 1);

    return AUMonotimbralInstrumentBase::SetParameter(inID,
                                                     inScope,
                                                     inElement,
                                                     inValue,
                                                     inBufferOffsetInFrames);
}

OSStatus MVS::Render(AudioUnitRenderActionFlags &ioActionFlags,
                     const AudioTimeStamp       &inTimeStamp,
                     UInt32                      inNumberFrames)
{
    PerformEvents(inTimeStamp);

    AUScope &outputs = Outputs();
    UInt32 numOutputs = outputs.GetNumberOfElements();
    for (UInt32 j = 0; j < numOutputs; j++)
        GetOutput(j)->PrepareBuffer(inNumberFrames);	// AUBase::DoRenderBus() only does this for the first output element

    // Allocate and clear oversampleBuf.
    UInt32 oversampleFrameCount = kOversampleRatio * inNumberFrames;
    Float32 oversampleBuf[oversampleFrameCount];
    memset(&oversampleBuf, 0, sizeof oversampleBuf);
    mOversampleBufPtr = oversampleBuf;

    UInt32 numGroups = Groups().GetNumberOfElements();
    for (UInt32 j = 0; j < numGroups; j++) {
        SynthGroupElement *group = (SynthGroupElement*)Groups().GetElement(j);
        OSStatus err = group->Render((SInt64)inTimeStamp.mSampleTime, inNumberFrames, outputs);
        if (err) return err;
    }
    mOversampleBufPtr = NULL;

    // Decimate oversampleBuf into each bus's output buf.
    Float32 *firstOutputBuf = NULL;
    for (UInt32 j = 0; j < numOutputs; j++) {
        AudioBufferList& bufferList = GetOutput(j)->GetBufferList();
        for (UInt32 k = 0; k < bufferList.mNumberBuffers; k++) {
            if (!firstOutputBuf) {
                firstOutputBuf = (Float32 *)bufferList.mBuffers[k].mData;
                mDecimator.decimate(oversampleBuf,
                                    firstOutputBuf,
                                    inNumberFrames);
            } else
                memcpy(bufferList.mBuffers[k].mData,
                       firstOutputBuf,
                       bufferList.mBuffers[k].mDataByteSize);
        }
    }
    mAbsoluteSampleFrame += inNumberFrames;
    return noErr;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark MVSNote Methods

MVSNote::MVSNote()
: mOversampleRatio(0),
  mOversampleBufPtr(NULL)
{}

void MVSNote::SetOversampleParams(UInt32 ratio, Float32 **bufPtr)
{
    mOversampleRatio  = ratio;
    mOversampleBufPtr = bufPtr;
}

bool MVSNote::Attack(const MusicDeviceNoteParams &inParams)
{
    double sampleRate  = SampleRate();
    Float32 maxLevel   = powf(inParams.mVelocity/127.0, 3.0);

    Float32 waveform   = GetGlobalParameter(kParameter_Osc1Waveform);

    Oscillator::Type otype = Oscillator::Saw;
    switch ((Waveform)(int)(waveform + 0.5f)) {

        case kWaveform_Saw:
            otype = Oscillator::Saw;
            break;

        case kWaveform_Square:
            otype = Oscillator::Square;
            break;

        case kWaveform_Triangle:
            otype = Oscillator::Triangle;
            break;

        case kWaveform_Sine:
            otype = Oscillator::Sine;
            break;

        default:
            // ???
            break;
    }

    float attackTime   = GetGlobalParameter(kParameter_AmpAttackTime);
    float decayTime    = GetGlobalParameter(kParameter_AmpDecayTime);
    float sustainLevel = GetGlobalParameter(kParameter_AmpSustainLevel);
    float releaseTime  = GetGlobalParameter(kParameter_AmpReleaseTime);

    mOsc1.initialize(sampleRate, otype);
    mAmpEnv.initialize(sampleRate,
                       maxLevel,
                       attackTime, decayTime, sustainLevel, releaseTime);
    return true;
}

void MVSNote::Release(UInt32 inFrame)
{
    mAmpEnv.release();
    SynthNote::Release(inFrame);
}

void MVSNote::FastRelease(UInt32 inFrame) // voice is being stolen.
{
    mAmpEnv.release();
    SynthNote::Release(inFrame);
}

Float32 MVSNote::Amplitude()
{
    return mAmpEnv.amplitude();
}

Float64 MVSNote::SampleRate()
{
    return mOversampleRatio * SynthNote::SampleRate();
}

OSStatus MVSNote::Render(UInt64            inAbsoluteSampleFrame,
                         UInt32            inNumFrames,
                         AudioBufferList **inBufferList,
                         UInt32            inOutBusCount)
{
    Float32 *outBuf     = *mOversampleBufPtr;
    UInt32   frameCount = inNumFrames * mOversampleRatio;

    // Get parameters.
    Float32 osc1mod    = GetGlobalParameter(kParameter_Osc1WaveMod) / 100.0;

    Float32 osc1buf[frameCount], ampbuf[frameCount];
    UInt32 end = mAmpEnv.generate(ampbuf, frameCount);
    if (end != 0xFFFFFFFF)
        frameCount = end;
    memset(osc1buf, 0, frameCount * sizeof *osc1buf);
    mOsc1.generate(Frequency(), osc1mod, osc1buf, frameCount);

    for (UInt32 i = 0; i < frameCount; i++) {
        Float32 out = osc1buf[i] * ampbuf[i];
        outBuf[i] += out;
    }

    if (end != 0xFFFFFFFF)
        NoteEnded(end);

    return noErr;
}

