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
 
static const UInt32  kMaxActiveNotes = 500;
static const UInt32  kOversampleRatio = 4;
static const Float64 kDecimatorPassFreq = 20000.0;

#define DEFPNAME(param, name) \
    static CFStringRef kParamName_##param = CFSTR(name);

DEFPNAME(Osc1Waveform,        "Oscillator 1 Waveform");
DEFPNAME(Osc1WaveMod,         "Oscillator 1 Modifier");
DEFPNAME(Osc1VibratoDepth,    "Oscillator 1 Vibrato Depth");
DEFPNAME(Osc1VibratoSpeed,    "Oscillator 1 Vibrato Speed");
DEFPNAME(Osc1VibratoWaveform, "Oscillator 1 Vibrato Waveform");

DEFPNAME(AmpAttackTime  , "Amplitude Attack Time");
DEFPNAME(AmpDecayTime   , "Amplitude Decay Time");
DEFPNAME(AmpSustainLevel, "Amplitude Sustain Level");
DEFPNAME(AmpReleaseTime , "Amplitude Release Time");

static CFStringRef kMenuItem_Waveform_Saw      = CFSTR("Sawtooth");
static CFStringRef kMenuItem_Waveform_Square   = CFSTR("Square/Pulse");
static CFStringRef kMenuItem_Waveform_Triangle = CFSTR("Triangle");
static CFStringRef kMenuItem_Waveform_Sine     = CFSTR("Sine");

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
    Globals()->SetParameter(kParameter_Osc1Waveform,        kWaveform_Saw);
    Globals()->SetParameter(kParameter_Osc1VibratoDepth,    0.000);
    Globals()->SetParameter(kParameter_Osc1VibratoSpeed,    3.000);
    Globals()->SetParameter(kParameter_Osc1VibratoWaveform, kWaveform_Sine);
    Globals()->SetParameter(kParameter_AmpAttackTime,       0.001);
    Globals()->SetParameter(kParameter_AmpDecayTime,        0.100);
    Globals()->SetParameter(kParameter_AmpSustainLevel,     1.000);
    Globals()->SetParameter(kParameter_AmpReleaseTime,      0.050);

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
        break;

    case kParameter_Osc1VibratoDepth:
        AUBase::FillInParameterName(info, kParamName_Osc1VibratoDepth, false);
        info.unit         = kAudioUnitParameterUnit_Cents;
        info.minValue     = 0;
        info.maxValue     = 600;
        info.defaultValue = 0;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;

        break;
        
    case kParameter_Osc1VibratoSpeed:
        AUBase::FillInParameterName(info, kParamName_Osc1VibratoSpeed, false);
        info.unit         = kAudioUnitParameterUnit_Hertz;
        info.minValue     =  0.1;
        info.maxValue     = 25.0;
        info.defaultValue =  3.0;
        info.flags       |= kAudioUnitParameterFlag_DisplayLogarithmic;

        break;
        
    case kParameter_Osc1VibratoWaveform:
        AUBase::FillInParameterName(info,
                                    kParamName_Osc1VibratoWaveform,
                                    false);
        info.unit         = kAudioUnitParameterUnit_Indexed;
        info.minValue     = 0;
        info.maxValue     = kNumberOfWaveforms - 1;
        info.defaultValue = kWaveform_Sine;
        break;

    case kParameter_AmpAttackTime:
        AUBase::FillInParameterName(info, kParamName_AmpAttackTime, false);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     =  0.000;
        info.maxValue     = 10.00;
        info.defaultValue =  0.001;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    case kParameter_AmpDecayTime:
        AUBase::FillInParameterName(info, kParamName_AmpDecayTime, false);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     =  0.0;
        info.maxValue     = 10.000;
        info.defaultValue =  0.100;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
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
        info.minValue     =  0.000;
        info.maxValue     = 10.000;
        info.defaultValue =  0.050;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
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

    if (inParameterID == kParameter_Osc1Waveform ||
        inParameterID == kParameter_Osc1VibratoWaveform) {
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
    if (inID == kParameter_Osc1VibratoWaveform && inValue < 1.0)
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

    Float32 o1wf       = GetGlobalParameter(kParameter_Osc1Waveform);
    Float32 o1vwf      = GetGlobalParameter(kParameter_Osc1VibratoWaveform);

    float attackTime   = GetGlobalParameter(kParameter_AmpAttackTime);
    float decayTime    = GetGlobalParameter(kParameter_AmpDecayTime);
    float sustainLevel = GetGlobalParameter(kParameter_AmpSustainLevel);
    float releaseTime  = GetGlobalParameter(kParameter_AmpReleaseTime);

    Oscillator::Type o1type    = OscillatorType((Waveform)(o1wf + 0.5));
    Oscillator::Type o1LFOtype = OscillatorType((Waveform)(o1vwf + 0.5));


    mOsc1LFO.initialize(sampleRate, o1LFOtype);
    mOsc1.initialize(sampleRate, o1type);
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
    Float32 osc1vibdep = GetGlobalParameter(kParameter_Osc1VibratoDepth) / 1200;
    Float32 osc1vibspd = GetGlobalParameter(kParameter_Osc1VibratoSpeed);

    // Generate envelope.  If note terminates, truncate frame count.
    Float32 ampbuf[frameCount];
    UInt32 end = mAmpEnv.generate(ampbuf, frameCount);
    if (end != 0xFFFFFFFF)
        frameCount = end;

    // Generate Oscillator 1.
    Float32 osc1buf[frameCount];
    memset(osc1buf, 0, sizeof osc1buf);
    if (osc1vibdep) {
        Float32 osc1CVbuf[frameCount];
        memset(osc1CVbuf, 0, frameCount * sizeof *osc1buf);
        mOsc1LFO.generate(osc1vibspd, 0, osc1CVbuf, frameCount);
        CVtoPhase(Frequency(), osc1vibdep, osc1CVbuf, frameCount);
        mOsc1.generate_modulated(osc1mod, osc1buf, osc1CVbuf, frameCount);
    } else {
        mOsc1.generate(Frequency(), osc1mod, osc1buf, frameCount);
    }

    for (UInt32 i = 0; i < frameCount; i++) {
        Float32 out = osc1buf[i] * ampbuf[i];
        outBuf[i] += out;
    }

    if (end != 0xFFFFFFFF)
        NoteEnded(end);

    return noErr;
}

void MVSNote::FillWithConstant(Float32 k, Float32 *buf, UInt32 count)
{
    for (UInt32 i = 0; i < count; i++)
        buf[i] = k;
}

void MVSNote::CVtoPhase(Float64  baseFreq,
                        Float32  cvDepth,
                        Float32 *buf,
                        UInt32   count)
{
    Float32 baseInc = baseFreq / SampleRate();
    for (UInt32 i = 0; i < count; i++) {
        Float32 cv = buf[i];
        Float32 inc = baseInc * (1 + cvDepth * cv);
        buf[i] = inc;
    }
}

Oscillator::Type MVSNote::OscillatorType(Waveform waveform)
{
    switch (waveform) {

        case kWaveform_Saw:
            return Oscillator::Saw;
            break;

        case kWaveform_Square:
            return Oscillator::Square;
            break;

        case kWaveform_Triangle:
            return Oscillator::Triangle;
            break;

        case kWaveform_Sine:
            return Oscillator::Sine;
            break;

        default:
            return Oscillator::Saw;     // ???
            break;
    }
}

