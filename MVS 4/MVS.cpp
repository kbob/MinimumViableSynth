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
 
static const UInt32  kMaxActiveNotes = 100;
static const UInt32  kOversampleRatio = 4;
static const Float64 kDecimatorPassFreq = 20000.0;


////////////////////////////////////////////////////////////////////////////////


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, MVS)

# pragma mark Utilities

static LFO::Polarity LFO_polarity(LFO::Waveform wf, Mod::Destination d)
{
    if (wf != LFO::Triangle && wf != LFO::SampleHold && wf != LFO::Random)
        return LFO::Unipolar;
    if (d == Mod::Osc1Freq || d == Mod::Osc1Width ||
        d == Mod::Osc2Freq || d == Mod::Osc2Width)
        return LFO::Bipolar;
    return LFO::Unipolar;
}

static inline void fill(float src, float *dest, size_t count)
{
    for (size_t i = 0; i < count; i++)
        dest[i] = src;
}

static inline float scale_dB40(float level)
{
    return level <= -40 ? 0 : powf(10, level / 20);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
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
            .min_max(0, 50)
            .default_value(50)
            .units(kAudioUnitParameterUnit_Percent);
    }

    {
        ParamClump osc2("Oscillator 2", "Osc 2");

        o2_coarse_detune.name("Coarse Detune")
            .min_max(-64, +63)
            .default_value(0)
            .units(kAudioUnitParameterUnit_MIDINoteNumber);

        o2_fine_detune.name("Fine Detune")
            .min_max(-128, +126)
            .default_value(0)
            .units(kAudioUnitParameterUnit_Cents);

        o2_waveform.name("Waveform")
            .value_strings(AR_waveforms)
            .default_value(Oscillator::Square);

        o2_width.name("Width")
            .min_max(0, 50)
            .default_value(50)
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

        mix_operator.name("Operator")
            .value_string(Mixer::Mix,      "Mix")
            .value_string(Mixer::RingMod,  "Ring Modulate")
            .value_string(Mixer::HardSync, "Hard Sync")
            .default_value(Mixer::Mix);

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
//            .value_string(Filter::Off,        "Off")
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
            .min_max(0, powf(9.999, 1/3.))
            .default_value(0.001)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);

        amp_decay.name("Decay Time")
            .min_max(0, powf(9.999, 1/3.))
            .default_value(0.100)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);

        amp_sustain.name("Sustain Level")
            .min_max(0, 1)
            .default_value(1)
            .units(kAudioUnitParameterUnit_LinearGain);

        amp_release.name("Release Time")
            .min_max(0, powf(9.999, 1/3.))
            .default_value(0.050)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);

        amp_amount.name("Level")
            .min_max(-40, 0)
            .default_value(0)
            .units(kAudioUnitParameterUnit_Decibels);
    }

    {
        ParamClump mw("Mod. Wheel", "MW");

        mw_destination.name("Destination")
            .value_string(Mod::Osc1Width,    "Osc 1 Width")
            .value_string(Mod::Osc2Width,    "Osc 2 Width")
            .value_string(Mod::NoiseLevel,    "Noise Level")
//            .value_string(Mod::FiltCutoff,    "Flt Cutoff")
//            .value_string(Mod::FiltResonance, "Flt Resonance")
//            .value_string(Mod::FiltDrive,     "Flt Drive")
            .value_string(Mod::LFO1Amount,    "LFO 1 Amount")
            .value_string(Mod::LFO1Speed,     "LFO 1 Speed")
            .value_string(Mod::LFO2Amount,    "LFO 2 Amount")
            .value_string(Mod::LFO2Speed,     "LFO 2 Speed")
            .value_string(Mod::AmpLevel,      "Env 1 Amount")
            .value_string(Mod::Env2Amount,    "Env 2 Amount")
            .value_string(Mod::NoDest,        "Off")
            .default_value(Mod::LFO1Amount)
            .assigns_mod(Mod::Wheel);

        mw_amount.name("Amount")
            .min_max(0, 1)
            .default_value(0.05)
            .units(kAudioUnitParameterUnit_Generic);
    }

    {
        ParamClump LFO1("LFO 1", "LFO 1");

        lfo1_waveform.name("Waveform")
            .value_strings(LF_waveforms)
            .default_value(LFO::Triangle);

        lfo1_speed.name("Speed")
            .min_max(log(0.1), log(50))
            .default_value(3.0)
            .units(kAudioUnitParameterUnit_Hertz)
            .flag(kAudioUnitParameterFlag_DisplayLogarithmic);
        ;

        lfo1_amount.name("Amount")
            .min_max(0, 1)
            .default_value(0)
            .units(kAudioUnitParameterUnit_Generic);

        lfo1_destination.name("Destination")
            .value_string(Mod::Osc1Freq,     "Osc 1 Frequency")
            .value_string(Mod::Osc1Width,    "Osc 1 Width")
            .value_string(Mod::Osc1Level,    "Osc 1 Level")
            .value_string(Mod::Osc2Freq,     "Osc 2 Frequency")
            .value_string(Mod::Osc2Width,    "Osc 2 Width")
            .value_string(Mod::Osc2Level,    "Osc 2 Level")
            .value_string(Mod::NoiseLevel,   "Noise Level")
//            .value_string(Mod::FltCutoff,    "Filt Cutoff")
//            .value_string(Mod::FltResonance, "Filt Resonance")
//            .value_string(Mod::FltDrive,     "Filt Drive")
            .value_string(Mod::AmpLevel,     "Env 1 Amount")
            .value_string(Mod::Env2Amount,   "Env 2 Amount")
            .value_string(Mod::NoDest,       "Off")
            .default_value(Mod::Osc1Freq)
            .assigns_mod(Mod::LFO1);
    }

    {
        ParamClump LFO1("LFO 2", "LFO 2");

        lfo2_waveform.name("Waveform")
            .value_strings(LF_waveforms)
            .default_value(LFO::UpSaw);

        lfo2_speed.name("Speed")
            .min_max(log(0.1), log(50))
            .default_value(3.0)
            .units(kAudioUnitParameterUnit_Hertz)
//            .flag(kAudioUnitParameterFlag_DisplayLogarithmic);
        ;

        lfo2_amount.name("Amount")
            .min_max(0, 1)
            .default_value(0.5)
            .units(kAudioUnitParameterUnit_Generic);

        lfo2_destination.name("Destination")
            .value_string(Mod::Osc1Freq,     "Osc 1 Frequency")
            .value_string(Mod::Osc1Width,    "Osc 1 Width")
            .value_string(Mod::Osc1Level,    "Osc 1 Level")
            .value_string(Mod::Osc2Freq,     "Osc 2 Frequency")
            .value_string(Mod::Osc2Width,    "Osc 2 Width")
            .value_string(Mod::Osc2Level,    "Osc 2 Level")
            .value_string(Mod::NoiseLevel,   "Noise Level")
//            .value_string(Mod::FltCutoff,    "Filt Cutoff")
//            .value_string(Mod::FltResonance, "Filt Resonance")
//            .value_string(Mod::FltDrive,     "Filt Drive")
            .value_string(Mod::LFO1Speed,    "LFO 1 Speed")
            .value_string(Mod::LFO1Amount,   "LFO 1 Amount")
            .value_string(Mod::AmpLevel,     "Env 1 Amount")
            .value_string(Mod::Env2Amount,   "Env 2 Amount")
            .value_string(Mod::NoDest,       "Off")
            .default_value(Mod::NoDest)
            .assigns_mod(Mod::LFO2);
    }

    {
        ParamClump env2("Envelope 2", "Env 2");

        env2_attack.name("Attack Time")
            .min_max(0, 9.999)
            .default_value(0.200)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);

        env2_decay.name("Decay Time")
            .min_max(0, 9.999)
            .default_value(0.300)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);

        env2_sustain.name("Sustain Level")
            .min_max(0, 1)
            .default_value(0.3)
            .units(kAudioUnitParameterUnit_LinearGain);

        env2_release.name("Release Time")
            .min_max(0, 9.999)
            .default_value(0.200)
            .units(kAudioUnitParameterUnit_Seconds)
            .flag(kAudioUnitParameterFlag_DisplayCubeRoot);

        env2_keytrack.name("Key Track")
            .min_max(0, 1)
            .default_value(1)
        .units(kAudioUnitParameterUnit_Generic);

        env2_amount.name("Amount")
            .min_max(-1, +1)
            .default_value(+1)
            .units(kAudioUnitParameterUnit_Generic);

        env2_destination.name("Destination")
            .value_string(Mod::Osc1Freq,     "Osc 1 Frequency")
            .value_string(Mod::Osc1Width,    "Osc 1 Width")
            .value_string(Mod::Osc1Level,    "Osc 1 Level")
            .value_string(Mod::Osc2Freq,     "Osc 2 Frequency")
            .value_string(Mod::Osc2Width,    "Osc 2 Width")
            .value_string(Mod::Osc2Level,    "Osc 2 Level")
            .value_string(Mod::NoiseLevel,   "Noise Level")
//            .value_string(Mod::FltCutoff,    "Filt Cutoff")
//            .value_string(Mod::FltResonance, "Filt Resonance")
//            .value_string(Mod::FltDrive,     "Filt Drive")
#if FULLY_IMPLEMENTED
            .value_string(Mod::LFO1Amount,   "LFO 1 Amount")
            .value_string(Mod::LFO2Amount,   "LFO 2 Amount")
#endif
            .value_string(Mod::NoDest,       "Off")
            .default_value(Mod::NoDest)
            .assigns_mod(Mod::Env2);
    }
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

    // Set the four modulators' default outputs.
    mModMatrix.assign(Mod::Wheel, Mod::LFO1Amount);
    mModMatrix.assign(Mod::LFO1, Mod::Osc1Freq);
    mModMatrix.assign(Mod::LFO2, Mod::NoDest);
    mModMatrix.assign(Mod::Env2, Mod::NoDest);

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
    double oversample_rate = mDecimator.oversampleRate();

    mModWheel.initialize(oversample_rate);
    mLFO1.initialize(oversample_rate);
    mLFO2.initialize(oversample_rate);

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
    float value = mParams.param_value(inID);

    int mod_src = mParams.param_mod_source(inID);
    if (mod_src) {
        mModMatrix.assign(mod_src, mParams.param_mod_dest(inID));
    }

    return AUMonotimbralInstrumentBase::SetParameter(inID,
                                                     inScope,
                                                     inElement,
                                                     value,
                                                     inBufferOffsetInFrames);
}

OSStatus MVS::HandleControlChange(UInt8	    inChannel,
                                  UInt8 	inController,
                                  UInt8 	inValue,
                                  UInt32	inStartFrame)
{
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
    const float *mod_values[Mod::SourceCount] = {
        NULL, wheelBuf, LFO1Buf, LFO2Buf, NULL
    };
    MVSModBox modbox(mModMatrix, oversampleFrameCount, mod_values);
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

void MVS::RunModulators(MVSModBox& modbox)
{
    const size_t nsamp = modbox.sampleCount();
    float *wheel_values = const_cast<float *>(modbox.get_values(Mod::Wheel));
    float *LFO1_values  = const_cast<float *>(modbox.get_values(Mod::LFO1));
    float *LFO2_values  = const_cast<float *>(modbox.get_values(Mod::LFO2));

    float freq[nsamp], depth[nsamp];

    Mod::Destination l1dest = mParams.lfo1_destination;
    Mod::Destination l2dest = mParams.lfo2_destination;
    LFO::Polarity l1polarity = LFO_polarity(mParams.lfo1_waveform, l1dest);
    LFO::Polarity l2polarity = LFO_polarity(mParams.lfo2_waveform, l2dest);

    // Who modulates whom?
    bool lfo1_mods_itself = (l1dest == Mod::LFO1Amount  ||
                             l1dest == Mod::LFO1Speed);
    bool lfo1_mods_lfo2   = (l1dest == Mod::LFO2Amount  ||
                             l1dest == Mod::LFO2Speed);

    bool lfo2_mods_itself = (l2dest == Mod::LFO2Amount  ||
                             l2dest == Mod::LFO2Speed);
    bool lfo2_mods_lfo1   = (l2dest == Mod::LFO1Amount  ||
                             l2dest == Mod::LFO1Speed);

    // If there is a modulation cycle, mute all cycle members.

    bool lfo1_in_cycle = lfo1_mods_itself;
    bool lfo2_in_cycle = lfo2_mods_itself;
    if (lfo1_mods_lfo2 && lfo2_mods_lfo1)
        lfo1_in_cycle = lfo2_in_cycle = true;

    // Mod Wheel is never modulated.
    mModWheel.generate_scaled(mParams.mw_amount, wheel_values, nsamp);

    if (lfo2_mods_lfo1) {

        // Do LFO2.

        if (lfo2_in_cycle) {
            fill(0, LFO2_values, nsamp);
        }
        else {
            modbox.modulate_freq(mParams.lfo2_speed, Mod::LFO2Speed, freq);
            modbox.modulate(mParams.lfo2_amount, Mod::LFO2Amount, depth);
            mLFO2.generate(mParams.lfo2_waveform,
                           l2polarity,
                           freq,
                           depth,
                           LFO2_values,
                           nsamp);
        }

        // Do LFO1.

        if (lfo1_in_cycle)
            fill(0, LFO1_values, nsamp);
        else {
            modbox.modulate_freq(mParams.lfo1_speed, Mod::LFO1Speed, freq);
            modbox.modulate(mParams.lfo1_amount, Mod::LFO1Amount, depth);
            mLFO1.generate(mParams.lfo1_waveform,
                           l1polarity,
                           freq,
                           depth,
                           LFO1_values,
                           nsamp);
        }
    } else {

        // Do LFO1.

        if (lfo1_in_cycle)
            fill(0, LFO1_values, nsamp);
        else {
            mModBoxPtr->modulate(mParams.lfo1_amount,   Mod::LFO1Amount, depth);
            mModBoxPtr->modulate_freq(mParams.lfo1_speed, Mod::LFO1Speed, freq);
            mLFO1.generate(mParams.lfo1_waveform,
                           l1polarity,
                           freq,
                           depth,
                           LFO1_values,
                           nsamp);
        }

        // Do LFO2.

        if (lfo2_in_cycle)
            fill(0, LFO2_values, nsamp);
        else {
            modbox.modulate_freq(mParams.lfo2_speed, Mod::LFO2Speed, freq);
            modbox.modulate(mParams.lfo2_amount, Mod::LFO2Amount, depth);
            mLFO2.generate(mParams.lfo2_waveform,
                           l2polarity,
                           freq,
                           depth,
                           LFO2_values,
                           nsamp);
        }
    }
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

void MVSNote::AffixModBox(const MVSModBox **boxPtrPtr)
{
    mModBoxPtrPtr = boxPtrPtr;
}

bool MVSNote::Attack(const MusicDeviceNoteParams &inParams)
{
    Float64 sampleRate   = SampleRate();

#if FULLY_IMPLEMENTED
    mEnv1.initialize(sampleRate);
    mEnv2.initialize(sampleRate);
    mAmplifier.initialize(sampleRate);
#else
    float maxLevel     = powf(inParams.mVelocity/127.0, 3.0);
    float attackTime   = mParams->amp_attack;
    float decayTime    = mParams->amp_decay;
    float sustainLevel = mParams->amp_sustain;
    float releaseTime  = mParams->amp_release;

    mAmpEnv.initialize(sampleRate,
                       maxLevel,
                       attackTime, decayTime, sustainLevel, releaseTime,
                       Envelope::Exponential);
#endif
    mOsc1.initialize(sampleRate);
    mOsc2.initialize(sampleRate);
    mNoise.initialize(sampleRate);
    mMixer.initialize(sampleRate);
    return true;
}

void MVSNote::Release(UInt32 inFrame)
{
    mAmpEnv.release();
    mEnv2.release();
    SynthNote::Release(inFrame);
}

void MVSNote::FastRelease(UInt32 inFrame) // voice is being stolen.
{
    mAmpEnv.release();
    mEnv2.release();
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
    const MVSParamSet *params = mParams;

    // Copy the monophonic modbox.
    MVSModBox     modbox      = **mModBoxPtrPtr;
    size_t        nsamp       = modbox.sampleCount();
    typedef float buf[nsamp];

    // Initialize the polyphonic modbox by adding a buffer for env2.
    buf           env2_values;
    modbox.set_values(Mod::Env2, env2_values);

    // Get the (oversampled) output buffer.
    float        *render_out  = *mOversampleBufPtr;

    float         sample_rate = SampleRate();
    float         frequency   = Frequency();
    float         norm_freq   = frequency / sample_rate;


    // - - - - - - -        Envelope 1               - - - - - - - - - -

    buf env1_values;
    UInt32 end;
    {
        buf attack, decay, sustain, release, amount;

        modbox.modulate(params->amp_attack,  Mod::AmpAttack,  attack);
        modbox.modulate(params->amp_decay,   Mod::AmpDecay,   decay);
        modbox.modulate(params->amp_sustain, Mod::AmpSustain, sustain);
        modbox.modulate(params->amp_release, Mod::AmpRelease, release);
        modbox.modulate(params->amp_amount,  Mod::AmpLevel,   amount);
#if FULLY_IMPLEMENTED
        end = mEnv1.generate(attack,
                             decay,
                             sustain,
                             release,
                             amount,
                             env1_values,
                             nsamp);
#else
        end = mAmpEnv.generate(env1_values, (UInt32)nsamp);
#endif
    }

    // - - - - - - -        Envelope 2               - - - - - - - - - -

    {
        buf attack, decay, sustain, release, amount;

        modbox.modulate(params->env2_attack,  Mod::Env2Attack,  attack);
        modbox.modulate(params->env2_decay,   Mod::Env2Decay,   decay);
        modbox.modulate(params->env2_sustain, Mod::Env2Sustain, sustain);
        modbox.modulate(params->env2_release, Mod::Env2Release, release);
        modbox.modulate(scale_dB40(params->env2_amount),  Mod::Env2Amount,  amount);
#if FULLY_IMPLEMENTED
        mEnv2.generate(attack,
                       decay,
                       sustain,
                       release,
                       amount,
                       env2_values,
                       nsamp);
#else
        fill(0, env2_values, nsamp);
#endif
    }

    // - - - - - - -        Oscillator 2             - - - - - - - - - -

    buf osc2_out;
    buf osc_sync;
    {
        buf freq, width;
        float coarse_detune = params->o2_coarse_detune;
        float fine_detune   = params->o2_fine_detune;
        float detune        = (coarse_detune + fine_detune / 100) / 12;
        float base_freq     = norm_freq * powf(2.0, detune);
        float base_width    = params->o2_width / 100;

        modbox.modulate_freq(base_freq, Mod::Osc2Freq, freq);
        modbox.modulate(base_width, Mod::Osc2Width, width);
        if (params->mix_operator == Mixer::HardSync) {
            buf sync_in;
            sync_in[0] = nsamp + 1; // no sync events
            mOsc2.generate_with_sync(params->o2_waveform,
                                     freq,
                                     width,
                                     osc2_out,
                                     sync_in,
                                     osc_sync,
                                     nsamp);
        } else {
            mOsc2.generate_modulated(params->o2_waveform,
                                     freq,
                                     width,
                                     osc2_out,
                                     nsamp);
        }
    }

    // - - - - - - -        Oscillator 1             - - - - - - - - - -

    buf osc1_out;
    {
        buf freq, width;
        float base_freq = norm_freq;
        float base_width = params->o1_width / 100;

        modbox.modulate_freq(base_freq, Mod::Osc1Freq, freq);
        modbox.modulate(base_width, Mod::Osc1Width, width);
        if (params->mix_operator == Mixer::HardSync) {
            buf sync_out_unused;
            mOsc1.generate_with_sync(params->o1_waveform,
                                     freq,
                                     width,
                                     osc1_out,
                                     osc_sync,
                                     sync_out_unused,
                                     nsamp);
        } else {
            mOsc1.generate_modulated(params->o1_waveform,
                                     freq,
                                     width,
                                     osc1_out,
                                     nsamp);
        }
    }

    // - - - - - - -        Noise Source             - - - - - - - - - -

    buf noise_out;
    mNoise.generate(params->noise_spectrum, noise_out, nsamp);

    // - - - - - - -        Mixer                    - - - - - - - - - -

    buf mixer_out;
    {
        buf osc1_level, osc2_level, noise_level;
        float osc1_base_level  = scale_dB40(params->mix_osc1_level);
        float osc2_base_level  = scale_dB40(params->mix_osc2_level);
        float noise_base_level = scale_dB40(params->mix_noise_level);

        modbox.modulate(osc1_base_level, Mod::Osc1Level, osc1_level);
        modbox.modulate(osc2_base_level, Mod::Osc2Level, osc2_level);
        modbox.modulate(noise_base_level, Mod::NoiseLevel, noise_level);
        mMixer.generate(params->mix_operator,
                        osc1_out,
                        osc1_level,
                        osc2_out,
                        osc2_level,
                        noise_out,
                        noise_level,
                        mixer_out,
                        nsamp);
    }

#if FULLY_IMPLEMENTED
    // - - - - - - -        Filter                   - - - - - - - - - -

    buf filter_out;
    {
        buf cutoff, resonance, drive;

        modbox.modulate(params->flt_cutoff,    Mod:FltCutoff, cutoff);
        modbox.modulate(params->flt_resonance, Mod:Resonance, cutoff);
        modbox.modulate(params->flt_drive,     Mod:Drive,     cutoff);
        mFilter.generate(params->flt_type,
                         cutoff,
                         resonance,
                         drive,
                         filter_out,
                         nsamp);
    }
#else
    float *filter_out = mixer_out;
#endif

    // - - - - - - -        Amplifier                - - - - - - - - - -

#if FULLY_IMPLEMENTED
    mAmplifier.generate_sum(filter_out, env1_values, render_out);
#else
    for (size_t i = 0; i < nsamp; i++)
        render_out[i] += filter_out[i] * env1_values[i];
#endif

    if (end != 0xFFFFFFFF)
        NoteEnded(end);

    return noErr;
}

OSStatus MVSNote::XXX_Render(UInt64            inAbsoluteSampleFrame,
                         UInt32            inNumFrames,
                         AudioBufferList **inBufferList,
                         UInt32            inOutBusCount)
{
    // Copy the monophonic modbox.
    MVSModBox modbox = **mModBoxPtrPtr;
    size_t nsamp = modbox.sampleCount();

    // Set the polyphonic modulator.
    float env2_values[nsamp];
    modbox.set_values(Mod::Env2, env2_values);

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
#if 0
    Float32 osc1buf[frameCount];
    memset(osc1buf, 0, toneFrameCount * sizeof osc1buf[0]);

    if (o1level)
        mOsc1.generate(o1type, Frequency(), osc1skew, osc1buf, toneFrameCount);
#else
    assert(nsamp == frameCount);
    Float32 osc1buf[nsamp];
    memset(osc1buf, 0, sizeof osc1buf);
    {
        float freq_in[nsamp];
        float width_in[nsamp];
        modbox.modulate_freq(Frequency() / SampleRate(), Mod::Osc1Freq, freq_in);
        modbox.modulate(osc1skew, Mod::Osc1Width, width_in);
        mOsc1.generate_modulated(o1type, freq_in, width_in, osc1buf, toneFrameCount);
    }
#endif

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
