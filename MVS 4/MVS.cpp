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

#include "AUMIDIDefs.h"
 
static const UInt32  kMaxActiveNotes = 500;
static const UInt32  kOversampleRatio = 4;
static const Float64 kDecimatorPassFreq = 20000.0;

struct ModBox {
    size_t       oversample_count;
    float const *wheel_values;
    float const *LFO1_values;
    float const *LFO2_values;
};

////////////////////////////////////////////////////////////////////////////////


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, MVS)

#pragma mark MVS Parameters

MVSParamSet::MVSParamSet()
{
    ValueStringMap AR_waveforms[] = {
        { Oscillator::Saw,      "Sawtooth"        },
        { Oscillator::Square,   "Square/Pulse"    },
        { Oscillator::Triangle, "Triangle"        },
        { Oscillator::Sine,     "Sine"            },
        { -1,                   NULL              }
    };

    ValueStringMap LF_waveforms[] = {
        { LFO::Triangle,        "Triangle"        },
        { LFO::UpSaw,           "Saw Up"          },
        { LFO::DnSaw,           "Saw Down"        },
        { LFO::Square,          "Square"          },
        { LFO::Random,          "Random"          },
        { LFO::SampleHold,      "Sample and Hold" },
        { -1,                   NULL              }
    };

    {
        ParamClump osc1("Oscillator 1", "Osc 1");

        o1_waveform.name("Waveform")
            .value_strings(AR_waveforms)
            .default_value(Oscillator::Saw);

        o1_width.name("Width")
            .min_max(0, 100)
            .default_value(0)
            .units(kAudioUnitParameterUnit_Percent);
    }

    {
        ParamClump osc2("Oscillator 2", "Osc 2");

        o2_coarse_detune.name("Coarse Detune")
            .min_max(-40, +40)
            .default_value(0)
            .units(kAudioUnitParameterUnit_MIDINoteNumber);

        o2_fine_detune.name("Fine Detune")
            .min_max(-100, +100)
            .default_value(0)
            .units(kAudioUnitParameterUnit_Cents);

        o2_waveform.name("Waveform")
            .value_strings(AR_waveforms)
            .default_value(Oscillator::Square);

        o2_width.name("Width")
            .min_max(0, 100)
            .default_value(0)
            .units(kAudioUnitParameterUnit_Percent);
    }

    {
        ParamClump noise("Noise Source", "Noise");

        noise_spectrum.name("Spectrum")
            .value_string(NoiseSource::White, "White")
            .value_string(NoiseSource::Pink,  "Pink")
            .value_string(NoiseSource::Red,   "Red")
            .default_value(NoiseSource::White);
    }

    {
        ParamClump mix("Mixer", "Mix");

//        mix_operator.name("Operator")
//            .value_string(Mixer::Mix,      "Mix")
//            .value_string(Mixer::RingMod,  "Ring Modulate")
//            .value_string(Mixer::HardSync, "Hard Sync")
//            .default_value(Mixer::Mix);

        mix_osc1_level.name("Oscillator 1 Level")
            .min_max(-40, 0)
            .default_value(0)
            .units(kAudioUnitParameterUnit_Decibels);

        mix_osc2_level.name("Oscillator 2 Level")
            .min_max(-40, 0)
            .default_value(-40)
            .units(kAudioUnitParameterUnit_Decibels);

        mix_noise_level.name("Noise Level")
            .min_max(-40, 0)
            .default_value(-40)
            .units(kAudioUnitParameterUnit_Decibels);
    }

//    {
//        ParamClump flt("Filter", "Filt");
//
//        flt_type.name("Type")
//            .value_string(Filter::LowPass,    "Low Pass")
//            .value_string(Filter::HighPass,   "High Pass")
//            .value_string(Filter::BandPass,   "Band Pass")
//            .value_string(Filter::BandReject, "Band Reject")
//            .default_value(Filter::LowPass);
//
//        flt_cutoff.name("Cutoff Frequency")
//            .min_max(20, 20000)
//            .default_value(20000)
//            .units(kAudioUnitParameterUnit_Hertz)
//            .flag(kAudioUnitParameterFlag_DisplayLogarithmic);
//
//        flt_resonance.name("Resonance")
//            .min_max(0, 4)
//            .default_value(0)
//            .units(kAudioUnitParameterUnit_LinearGain);
//
//        flt_drive.name("Drive")
//            .min_max(-24, 0)
//            .default_value(-24)
//            .units(kAudioUnitParameterUnit_Decibels);
//
//        flt_keytrack.name("Key Track")
//            .min_max(0, 100)
//            .default_value(0)
//            .units(kAudioUnitParameterUnit_Percent);
//    }

    {
        ParamClump amp("Amplifier", "Amp");

        amp_attack.name("Attack Time")
            .min_max(0, 9.999)
            .default_value(0.001)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);

        amp_decay.name("Decay Time")
            .min_max(0, 9.999)
            .default_value(0.100)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);

        amp_sustain.name("Sustain Level")
            .min_max(0, 1)
            .default_value(1)
            .units(kAudioUnitParameterUnit_LinearGain);

        amp_release.name("Release Time")
            .min_max(0, 9.999)
            .default_value(0.050)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);
    }

//    {
//        ParamClump mw("Mod. Wheel", "MW");
//
//        mw_assign.name("Assign")
//            .value_string(Assign::NoiseLevel,   "Noise Level")
//            .value_string(Assign::FltCutoff,    "Flt Cutoff")
//            .value_string(Assign::FltResonance, "Flt Resonance")
//            .value_string(Assign::FltDrive,     "Flt Drive")
//            .value_string(Assign::LFO1Amount,   "LFO 1 Amount")
//            .value_string(Assign::LFO1Speed,    "LFO 1 Speed")
//            .value_string(Assign::LFO2Amount,   "LFO 2 Amount")
//            .value_string(Assign::LFO2Speed,    "LFO 2 Speed")
//            .value_string(Assign::Env2Amount,   "Env 2 Amount")
//            .default_value(Assign::LFO1Amount);
//
//        mw_amount.name("Amount")
//            .min_max(0, 1)
//            .default_value(1)
//            .units(kAudioUnitParameterUnit_Generic);
//    }

    {
        ParamClump LFO1("LFO 1", "LFO 1");

        lfo1_waveform.name("Waveform")
            .value_strings(LF_waveforms)
            .default_value(LFO::Triangle);

        lfo1_speed.name("Speed")
            .min_max(0.1, 20)
            .default_value(3.0)
            .units(kAudioUnitParameterUnit_Hertz)
            .flag(kAudioUnitParameterFlag_DisplayLogarithmic);

        lfo1_amount.name("Amount")
            .min_max(0, 1)
            .default_value(0.5)
            .units(kAudioUnitParameterUnit_Generic);

        lfo1_assign.name("Assign")
            .value_string(Assign::Osc1Freq,     "Osc 1 Frequency")
//            .value_string(Assign::Osc2Width,    "Osc 1 Width")
            .value_string(Assign::Osc2Freq,     "Osc 2 Frequency")
//            .value_string(Assign::Osc2Width,    "Osc 2 Width")
//            .value_string(Assign::NoiseLevel,   "Noise Level")
//            .value_string(Assign::FltCutoff,    "Filt Cutoff")
//            .value_string(Assign::FltResonance, "Filt Resonance")
//            .value_string(Assign::FltDrive,     "Filt Drive")
//            .value_string(Assign::Env2Amount,   "Env 2 Amount")
            .value_string(Assign::None,         "Off")
            .default_value(Assign::Osc1Freq);
    }

    {
        ParamClump LFO1("LFO 2", "LFO 2");

        lfo2_waveform.name("Waveform")
            .value_strings(LF_waveforms)
            .default_value(LFO::UpSaw);

        lfo2_speed.name("Speed")
            .min_max(0.1, 20)
            .default_value(3.0)
            .units(kAudioUnitParameterUnit_Hertz)
            .flag(kAudioUnitParameterFlag_DisplayLogarithmic);

        lfo2_amount.name("Amount")
            .min_max(0, 1)
            .default_value(0.5)
            .units(kAudioUnitParameterUnit_Generic);

        lfo2_assign.name("Assign")
            .value_string(Assign::Osc1Freq,     "Osc 1 Frequency")
//            .value_string(Assign::Osc2Width,    "Osc 1 Width")
            .value_string(Assign::Osc2Freq,     "Osc 2 Frequency")
//            .value_string(Assign::Osc2Width,    "Osc 2 Width")
//            .value_string(Assign::NoiseLevel,   "Noise Level")
//            .value_string(Assign::FltCutoff,    "Filt Cutoff")
//            .value_string(Assign::FltResonance, "Filt Resonance")
//            .value_string(Assign::FltDrive,     "Filt Drive")
//            .value_string(Assign::Env2Amount,   "Env 2 Amount")
            .value_string(Assign::None,         "Off")
            .default_value(Assign::None);
    }

//    {
//        ParamClump env2("Envelope 2", "Env 2");
//
//        env2_attack.name("Attack Time")
//            .min_max(0, 9.999)
//            .default_value(0.200)
//            .units(kAudioUnitParameterUnit_Seconds)
//            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);
//
//        env2_decay.name("Decay Time")
//            .min_max(0, 9.999)
//            .default_value(0.300)
//            .units(kAudioUnitParameterUnit_Seconds)
//            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);
//
//        env2_sustain.name("Sustain Level")
//            .min_max(0, 1)
//            .default_value(0.3)
//            .units(kAudioUnitParameterUnit_LinearGain);
//
//        env2_release.name("Release Time")
//            .min_max(0, 9.999)
//            .default_value(0.200)
//            .units(kAudioUnitParameterUnit_Seconds)
//            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);
//
//        env2_amount.name("Amount")
//            .min_max(-1, +1)
//            .default_value(+1)
//            .units(kAudioUnitParameterUnit_Generic);
//
//        env2_assign.name("Assign")
//            .value_string(Assign::Osc1Freq,     "Osc 1 Frequency")
//            .value_string(Assign::Osc2Width,    "Osc 1 Width")
//            .value_string(Assign::Osc1Level,    "Osc 1 Level")
//            .value_string(Assign::Osc2Freq,     "Osc 2 Frequency")
//            .value_string(Assign::Osc2Width,    "Osc 2 Width")
//            .value_string(Assign::Osc2Level,    "Osc 2 Level")
//            .value_string(Assign::NoiseLevel,   "Noise Level")
//            .value_string(Assign::FltCutoff,    "Filt Cutoff")
//            .value_string(Assign::FltResonance, "Filt Resonance")
//            .value_string(Assign::FltDrive,     "Filt Drive")
//            .value_string(Assign::LFO1Amount,   "LFO 1 Amount")
//            .value_string(Assign::LFO1Speed,    "LFO 1 Speed")
//            .value_string(Assign::LFO1Amount,   "LFO 2 Amount")
//            .value_string(Assign::LFO1Speed,    "LFO 2 Speed")
//            .value_string(Assign::None,         "Off")
//            .default_value(Assign::None);
//    }
}


#pragma mark MVS Methods

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

    AUElement *glob = Globals();
    glob->UseIndexedParameters((int)mParams.size());
    mParams.set_defaults();
    for (size_t i = 0; i < mParams.size(); i++)
        glob->SetParameter((AudioUnitParameterID)i, mParams.param_value(i));

    SetAFactoryPresetAsCurrent(kPresets[kPreset_Default]);

    for (size_t i = 0; i < kNumNotes; i++) {
        mNotes[i].SetOversampleParams(kOversampleRatio, &mOversampleBufPtr);
        mNotes[i].AffixParams(&mParams);
        mNotes[i].AffixModBox(&mModBoxPtr);
    }
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

    Float64 baseSampleRate = GetOutput(0)->GetStreamFormat().mSampleRate;
    mDecimator.initialize(baseSampleRate, kDecimatorPassFreq, kOversampleRatio);
    mModWheel.initialize(mDecimator.oversampleRate());

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
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    if (inParameterID >= mParams.size())
        return kAudioUnitErr_InvalidParameter;

    outParameterInfo = mParams.param_info(inParameterID);
    return noErr;
}

OSStatus MVS::GetParameterValueStrings(AudioUnitScope       inScope,
                                       AudioUnitParameterID inParameterID,
                                       CFArrayRef          *outStrings)
{
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    CFArrayRef array = mParams.param_value_strings(inParameterID);
    if (!array)
        return kAudioUnitErr_InvalidParameter;

    if (outStrings)
        *outStrings = array;

    return noErr;
}

OSStatus MVS::CopyClumpName(AudioUnitScope          inScope,
                            UInt32                  inClumpID,
                            UInt32                  inDesiredNameLength,
                            CFStringRef            *outClumpName)
{
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    const char *name = mParams.clump_name(inClumpID);
    if (!name)
        return kAudioUnitErr_InvalidParameter;
    *outClumpName = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
                                                    name,
                                                    kCFStringEncodingUTF8,
                                                    kCFAllocatorNull);
    return noErr;
}

OSStatus MVS::SetParameter(AudioUnitParameterID    inID,
                           AudioUnitScope          inScope,
                           AudioUnitElement        inElement,
                           AudioUnitParameterValue inValue,
                           UInt32                  inBufferOffsetInFrames)
{
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    if (inID >= mParams.size())
        return kAudioUnitErr_InvalidParameter;

    // This may change the value, so set the parameter's value, then
    // read it back.
    OSStatus err = mParams.set_param_value(inID, inValue);
    if (err != noErr)
        return err;

    return AUMonotimbralInstrumentBase::SetParameter(inID,
                                                     inScope,
                                                     inElement,
                                                     mParams.param_value(inID),
                                                     inBufferOffsetInFrames);
}

OSStatus MVS::HandleControlChange(UInt8	    inChannel,
                                  UInt8 	inController,
                                  UInt8 	inValue,
                                  UInt32	inStartFrame)
{
    printf("HandleCC: CC=%u\n", inController);
    if (inController == kMidiController_ModWheel)
        mModWheel.set_raw_MSB(inValue);
    else if (inController == kMidiController_ModWheel + 32)
        mModWheel.set_raw_LSB(inValue);
    return AUInstrumentBase::HandleControlChange(inChannel,
                                                 inController,
                                                 inValue,
                                                 inStartFrame);
}


OSStatus MVS::Render(AudioUnitRenderActionFlags &ioActionFlags,
                     const AudioTimeStamp       &inTimeStamp,
                     UInt32                      inNumberFrames)
{
    PerformEvents(inTimeStamp);

    AUScope &outputs = Outputs();
    UInt32 numOutputs = outputs.GetNumberOfElements();
    for (UInt32 j = 0; j < numOutputs; j++)
        GetOutput(j)->PrepareBuffer(inNumberFrames); // AUBase::DoRenderBus()
                                // only does this for the first output element

    // Allocate and clear oversampleBuf.
    UInt32 oversampleFrameCount = kOversampleRatio * inNumberFrames;
    Float32 oversampleBuf[oversampleFrameCount];
    memset(&oversampleBuf, 0, sizeof oversampleBuf);
    mOversampleBufPtr = oversampleBuf;

    // Allocate modulation buffers.
    Float32 wheelBuf[oversampleFrameCount];
    Float32 LFO1Buf[oversampleFrameCount];
    Float32 LFO2Buf[oversampleFrameCount];

    // Create the ModBox.
    ModBox modbox = {
        .wheel_values = wheelBuf,
        .LFO1_values  = LFO1Buf,
        .LFO2_values  = LFO2Buf,
    };
    mModBoxPtr = &modbox;

    RunModulators(modbox);

    UInt32 numGroups = Groups().GetNumberOfElements();
    for (UInt32 j = 0; j < numGroups; j++) {
        SynthGroupElement *group = (SynthGroupElement*)Groups().GetElement(j);
        OSStatus err = group->Render((SInt64)inTimeStamp.mSampleTime,
                                     inNumberFrames,
                                     outputs);
        if (err) return err;
    }
    mOversampleBufPtr = NULL;
    mModBoxPtr = NULL;

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

void MVS::RunModulators(ModBox& ioWork)
{
    mModWheel.generate(const_cast<float *>(ioWork.wheel_values),
                       ioWork.oversample_count);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark MVSNote Methods

MVSNote::MVSNote()
: mOversampleRatio(0),
  mOversampleBufPtr(NULL),
  mParams(NULL)
{}

void MVSNote::SetOversampleParams(UInt32 ratio, Float32 **bufPtr)
{
    mOversampleRatio  = ratio;
    mOversampleBufPtr = bufPtr;
}

void MVSNote::AffixParams(const MVSParamSet *params)
{
    mParams = params;
}

void MVSNote::AffixModBox(const ModBox **boxPtrPtr)
{
    mModBoxPtrPtr = boxPtrPtr;
}

bool MVSNote::Attack(const MusicDeviceNoteParams &inParams)
{
    Float64 sampleRate   = SampleRate();
    Float32 maxLevel     = powf(inParams.mVelocity/127.0, 3.0);

    float attackTime   = mParams->amp_attack;
    float decayTime    = mParams->amp_decay;
    float sustainLevel = mParams->amp_sustain;
    float releaseTime  = mParams->amp_release;

    mOsc1.initialize(sampleRate);
    mOsc2.initialize(sampleRate);
    mNoise.initialize(sampleRate);
    mAmpEnv.initialize(sampleRate,
                       maxLevel,
                       attackTime, decayTime, sustainLevel, releaseTime,
                       Envelope::Exponential);
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
    const ModBox *modbox = *mModBoxPtrPtr;

    Float32 *outBuf     = *mOversampleBufPtr;
    UInt32   frameCount = inNumFrames * mOversampleRatio;

    // Get parameters.
    const MVSParamSet *params = mParams;
    Float32 osc1skew     = params->o1_width / 100;
    Float32 o2coarsetune = params->o2_coarse_detune;
    Float32 o2finetune   = params->o2_fine_detune;
    Float32 osc2skew     = params->o2_width / 100;
    int ntyp             = params->noise_spectrum;
    Float32 o1level      = params->mix_osc1_level;
    Float32 o2level      = params->mix_osc2_level;
    Float32 nlevel       = params->mix_noise_level;

    Oscillator::Waveform o1type = params->o1_waveform;
    Oscillator::Waveform o2type = params->o2_waveform;

    // Convert parameters.
    Float32 osc2detune = o2coarsetune + o2finetune / 100.0;
    NoiseSource::Type ntype = (NoiseSource::Type)ntyp;
    o1level = o1level <= -40 ? 0 : powf(10.0, o1level / 20.0);
    o2level = o2level <= -40 ? 0 : powf(10.0, o2level / 20.0);
    nlevel  = nlevel  <= -40 ? 0 : powf(10.0, nlevel / 20.0);

    // Generate envelope.  If note terminates, truncate frame count.
    UInt32 toneFrameCount = frameCount;
    Float32 ampbuf[frameCount];
    UInt32 end = mAmpEnv.generate(ampbuf, frameCount);
    if (end != 0xFFFFFFFF)
        toneFrameCount = end;

    // Generate Oscillator 1.
    Float32 osc1buf[frameCount];
    memset(osc1buf, 0, toneFrameCount * sizeof osc1buf[0]);

    if (o1level)
        mOsc1.generate(o1type, Frequency(), osc1skew, osc1buf, toneFrameCount);

    // Generate Oscillator 2.
    Float32 osc2buf[frameCount];
    memset(osc2buf, 0, toneFrameCount * sizeof osc2buf[0]);
    Float64 o2freq = Frequency() * pow(2.0, osc2detune / 12.0);
    if (o2level)
        mOsc2.generate(o2type, o2freq, osc2skew, osc2buf, toneFrameCount);

    // Generate noise.
    Float32 noiseBuf[frameCount];
    UInt32 noiseFrameCount = 0;
    if (nlevel) {
        noiseFrameCount = frameCount;
        mNoise.generate(ntype, noiseBuf, noiseFrameCount);
    }

    // Mix.
    Float32 mixbuf[frameCount];
    UInt32 n1 = frameCount;
    if (n1 > toneFrameCount)
        n1 = toneFrameCount;
    UInt32 n0 = n1;
    if (n0 > noiseFrameCount)
        n0 = noiseFrameCount;
    for (UInt32 i = 0; i < n0; i++) {
        Float32 out = o1level * osc1buf[i];
        out += o2level * osc2buf[i];
        out += nlevel * noiseBuf[i];
        mixbuf[i] = out;
    }
    for (UInt32 i = n0; i < n1; i++) {
        Float32 out = o1level * osc1buf[i];
        out += o2level * osc2buf[i];
        mixbuf[i] = out;
    }

    // Amplitude
    for (UInt32 i = 0; i < n1; i++) {
        Float32 out = mixbuf[i] * ampbuf[i];
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
