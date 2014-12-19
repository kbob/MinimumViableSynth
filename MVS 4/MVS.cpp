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

//static CFStringRef kClumpName_Osc1   = CFSTR("Oscillator 1");
//static CFStringRef kClumpName_Osc2   = CFSTR("Oscillator 2");
//static CFStringRef kClumpName_Noise  = CFSTR("Noise Source");
//static CFStringRef kClumpName_Mixer  = CFSTR("Mixer");
//static CFStringRef kClumpName_Filter = CFSTR("Filter");
//static CFStringRef kClumpName_Amp    = CFSTR("Amplitude");

//#define DEFPNAME(param, name) \
//    static CFStringRef kParamName_##param = CFSTR(name);

//DEFPNAME(Osc1Waveform,        "Oscillator 1 Waveform");
//DEFPNAME(Osc1WaveSkew,        "Oscillator 1 Skew");
//DEFPNAME(Osc1VibratoDepth,    "Oscillator 1 Vibrato Depth");
//DEFPNAME(Osc1VibratoSpeed,    "Oscillator 1 Vibrato Speed");
//DEFPNAME(Osc1VibratoWaveform, "Oscillator 1 Vibrato Waveform");
//
//DEFPNAME(Osc2CoarseDetune,    "Oscillator 2 Coarse Detune");
//DEFPNAME(Osc2FineDetune,      "Oscillator 2 Fine Detune");
//DEFPNAME(Osc2Waveform,        "Oscillator 2 Waveform");
//DEFPNAME(Osc2WaveSkew,        "Oscillator 2 Skew");
//DEFPNAME(Osc2VibratoDepth,    "Oscillator 2 Vibrato Depth");
//DEFPNAME(Osc2VibratoSpeed,    "Oscillator 2 Vibrato Speed");
//DEFPNAME(Osc2VibratoWaveform, "Oscillator 2 Vibrato Waveform");
//
//DEFPNAME(NoiseType,           "Noise Type");
//DEFPNAME(NoiseAttackTime,     "Noise Attack Time");
//DEFPNAME(NoiseDecayTime,      "Noise Decay Time");
//DEFPNAME(NoiseSustainLevel,   "Noise Sustain Level");
//DEFPNAME(NoiseReleaseTime,    "Noise Release Time");
//
//DEFPNAME(Osc1Level,           "Oscillator 1 Level");
//DEFPNAME(Osc2Level,           "Oscillator 2 Level");
//DEFPNAME(NoiseLevel,          "Noise Level");
//
//DEFPNAME(AmpAttackTime,       "Amplitude Attack Time");
//DEFPNAME(AmpDecayTime,        "Amplitude Decay Time");
//DEFPNAME(AmpSustainLevel,     "Amplitude Sustain Level");
//DEFPNAME(AmpReleaseTime,      "Amplitude Release Time");

//static CFStringRef kMenuItem_Waveform_Saw      = CFSTR("Sawtooth");
//static CFStringRef kMenuItem_Waveform_Square   = CFSTR("Square/Pulse");
//static CFStringRef kMenuItem_Waveform_Triangle = CFSTR("Triangle");
//static CFStringRef kMenuItem_Waveform_Sine     = CFSTR("Sine");
//
//static CFStringRef kMenuItem_Noise_White       = CFSTR("White");
//static CFStringRef kMenuItem_Noise_Pink        = CFSTR("Pink");
//static CFStringRef kMenuItem_Noise_Red         = CFSTR("Red");

////////////////////////////////////////////////////////////////////////////////


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, MVS)

#pragma mark MVS Params

// XXX temporary
class Mixer {
public:
    enum {
        Mix,
        RingMod,
        HardSync
    };
};

// XXX temporary
class Filter {
public:
    enum {
        LowPass,
        HighPass,
        BandPass,
        BandReject
    };
};

// XXX temporary
class Assign {
public:
    enum {
        None,

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
//      FltKeyTrack,

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
    };
};

//XXX temporary
class LFO {
public:
    enum Waveform {
        Triangle,
        UpSaw,
        DnSaw,
        Square,
        Random,
        SampleHold,
    };
};

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

    {
        ParamClump flt("Filter", "Filt");

        flt_type.name("Type")
            .value_string(Filter::LowPass,    "Low Pass")
            .value_string(Filter::HighPass,   "High Pass")
            .value_string(Filter::BandPass,   "Band Pass")
            .value_string(Filter::BandReject, "Band Reject")
            .default_value(Filter::LowPass);

        flt_cutoff.name("Cutoff Frequency")
            .min_max(20, 20000)
            .default_value(20000)
            .units(kAudioUnitParameterUnit_Hertz)
            .flag(kAudioUnitParameterFlag_DisplayLogarithmic);

        flt_resonance.name("Resonance")
            .min_max(0, 4)
            .default_value(0)
            .units(kAudioUnitParameterUnit_LinearGain);

        flt_drive.name("Drive")
            .min_max(-24, 0)
            .default_value(-24)
            .units(kAudioUnitParameterUnit_Decibels);

        flt_keytrack.name("Key Track")
            .min_max(0, 100)
            .default_value(0)
            .units(kAudioUnitParameterUnit_Percent);
    }

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

    {
        ParamClump mw("Mod. Wheel", "MW");

        mw_assign.name("Assign")
            .value_string(Assign::NoiseLevel,   "Noise Level")
            .value_string(Assign::FltCutoff,    "Flt Cutoff")
            .value_string(Assign::FltResonance, "Flt Resonance")
            .value_string(Assign::FltDrive,     "Flt Drive")
            .value_string(Assign::LFO1Amount,   "LFO 1 Amount")
            .value_string(Assign::LFO1Speed,    "LFO 1 Speed")
            .value_string(Assign::LFO2Amount,   "LFO 2 Amount")
            .value_string(Assign::LFO2Speed,    "LFO 2 Speed")
            .value_string(Assign::Env2Amount,   "Env 2 Amount")
            .default_value(Assign::LFO1Amount);

        mw_amount.name("Amount")
            .min_max(0, 1)
            .default_value(1)
            .units(kAudioUnitParameterUnit_Generic);
    }

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
            .value_string(Assign::Osc2Width,    "Osc 1 Width")
            .value_string(Assign::Osc2Freq,     "Osc 2 Frequency")
            .value_string(Assign::Osc2Width,    "Osc 2 Width")
            .value_string(Assign::NoiseLevel,   "Noise Level")
            .value_string(Assign::FltCutoff,    "Filt Cutoff")
            .value_string(Assign::FltResonance, "Filt Resonance")
            .value_string(Assign::FltDrive,     "Filt Drive")
            .value_string(Assign::Env2Amount,   "Env 2 Amount")
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
            .value_string(Assign::Osc2Width,    "Osc 1 Width")
            .value_string(Assign::Osc2Freq,     "Osc 2 Frequency")
            .value_string(Assign::Osc2Width,    "Osc 2 Width")
            .value_string(Assign::NoiseLevel,   "Noise Level")
            .value_string(Assign::FltCutoff,    "Filt Cutoff")
            .value_string(Assign::FltResonance, "Filt Resonance")
            .value_string(Assign::FltDrive,     "Filt Drive")
            .value_string(Assign::Env2Amount,   "Env 2 Amount")
            .value_string(Assign::None,         "Off")
            .default_value(Assign::None);
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

        env2_amount.name("Amount")
            .min_max(-1, +1)
            .default_value(+1)
            .units(kAudioUnitParameterUnit_Generic);

        env2_assign.name("Assign")
            .value_string(Assign::Osc1Freq,     "Osc 1 Frequency")
            .value_string(Assign::Osc2Width,    "Osc 1 Width")
            .value_string(Assign::Osc1Level,    "Osc 1 Level")
            .value_string(Assign::Osc2Freq,     "Osc 2 Frequency")
            .value_string(Assign::Osc2Width,    "Osc 2 Width")
            .value_string(Assign::Osc2Level,    "Osc 2 Level")
            .value_string(Assign::NoiseLevel,   "Noise Level")
            .value_string(Assign::FltCutoff,    "Filt Cutoff")
            .value_string(Assign::FltResonance, "Filt Resonance")
            .value_string(Assign::FltDrive,     "Filt Drive")
            .value_string(Assign::LFO1Amount,   "LFO 1 Amount")
            .value_string(Assign::LFO1Speed,    "LFO 1 Speed")
            .value_string(Assign::LFO1Amount,   "LFO 2 Amount")
            .value_string(Assign::LFO1Speed,    "LFO 2 Speed")
            .value_string(Assign::None,         "Off")
            .default_value(Assign::None);
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
#if 1
    AUElement *gp = Globals();
    gp->UseIndexedParameters((int)mParams.size());
    mParams.set_defaults();
    for (size_t i = 0; i < mParams.size(); i++)
        gp->SetParameter((AudioUnitParameterID)i, mParams.param_value(i));
#else

    AUElement *gp = Globals();
    gp->UseIndexedParameters(kNumberOfParameters);

    // Oscillator 1
    gp->SetParameter(kParameter_Osc1Waveform,        kWaveform_Saw);
    gp->SetParameter(kParameter_Osc1VibratoDepth,    0.000);
    gp->SetParameter(kParameter_Osc1VibratoSpeed,    3.000);
    gp->SetParameter(kParameter_Osc1VibratoWaveform, kWaveform_Sine);

    // Oscillator 2
    gp->SetParameter(kParameter_Osc2CoarseDetune,    0.000);
    gp->SetParameter(kParameter_Osc2FineDetune,      0.000);
    gp->SetParameter(kParameter_Osc2Waveform,        kWaveform_Square);
    gp->SetParameter(kParameter_Osc2VibratoDepth,    0.000);
    gp->SetParameter(kParameter_Osc2VibratoSpeed,    3.000);
    gp->SetParameter(kParameter_Osc2VibratoWaveform, kWaveform_Sine);

    // Noise Source
    gp->SetParameter(kParameter_NoiseType,           kNoiseType_White);
    gp->SetParameter(kParameter_NoiseAttackTime,     0.001);
    gp->SetParameter(kParameter_NoiseDecayTime,      0.100);
    gp->SetParameter(kParameter_NoiseSustainLevel,   0.040);
    gp->SetParameter(kParameter_NoiseReleaseTime,    0.050);

    // Mixer
    gp->SetParameter(kParameter_Osc1Level,           0.000);
    gp->SetParameter(kParameter_Osc2Level,         -40.000);
    gp->SetParameter(kParameter_NoiseLevel,        -40.000);

    // Amplitude Envelope
    gp->SetParameter(kParameter_AmpAttackTime,       0.001);
    gp->SetParameter(kParameter_AmpDecayTime,        0.100);
    gp->SetParameter(kParameter_AmpSustainLevel,     1.000);
    gp->SetParameter(kParameter_AmpReleaseTime,      0.050);
#endif

    SetAFactoryPresetAsCurrent(kPresets[kPreset_Default]);

    for (size_t i = 0; i < kNumNotes; i++) {
        mNotes[i].SetOversampleParams(kOversampleRatio, &mOversampleBufPtr);
        mNotes[i].AffixParams(&mParams);
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//      MVS::~MVS
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MVS::~MVS()
{}

//void MVS::CreateParameters()
//{
//    static const ValueStringMap AR_osc_waveform_strings[] = {
//        { Oscillator::Saw,      "Saw"           },
//        { Oscillator::Square,   "Square/Pulse"  },
//        { Oscillator::Triangle, "Triangle"      },
//        { Oscillator::Sine,     "Sine"          },
//        { -1,                   NULL            }
//    };
//
//    static const ValueStringMap noise_spectrum_strings[] = {
//        { NoiseSource::White,   "White"         },
//        { NoiseSource::Pink,    "Pink"          },
//        { NoiseSource::Red,     "Red"           },
//        { -1,                   NULL            }
//    };
//
//    static const ValueStringMap mix_operator_strings[] = {
//        { Mixer::Mix,           "Mix"           },
//        { Mixer::RingMod,       "Ring Modulate" },
//        { Mixer::HardSync,      "Hard Sync"     },
//        { -1,                   NULL            }
//    };
//
//    static const ValueStringMap filter_type_strings[] = {
//        { Filter::LowPass,      "Low Pass"      },
//        { Filter::HighPass,     "High Pass"     },
//        { Filter::BandPass,     "Band Pass"     },
//        { Filter::BandReject,   "Band Reject"   },
//        { -1,                   NULL            }
//    };
//
//
//    Clump *osc1 = mParams.addClump("Oscillator 1", "Osc1");
//
//    osc1->addParam(&mOsc1Waveform);
//    mOsc1Waveform
//    osc1->addParam("Waveform")
//        .value_strings(AR_osc_waveform_strings)
//        .default_value(Oscillator::Saw);
//
//    osc1->addParam("Width")
//        .min_max(0, 1)
//        .default_value(0)
//        .units(kAudioUnitParameterUnit_Percent);
//
//
//    Clump *osc2 = mParams.addClump("Oscillator 2", "Osc2");
//
//    osc2->addParam("Coarse Detune")
//        .min_max(-40, +40)
//        .default_value(0)
//        .units(kAudioUnitParameterUnit_MIDINoteNumber);
//
//    osc2->addParam("Fine Detune")
//        .min_max(-100, +100)
//        .default_value(0)
//        .units(kAudioUnitParameterUnit_Cents);
//
//    osc2->addParam("Waveform")
//        .value_strings(AR_osc_waveform_strings)
//        .default_value(Oscillator::Square);
//
//    osc2->addParam("Width")
//        .min_max(0, 1)
//        .default_value(0)
//        .units(kAudioUnitParameterUnit_Percent);
//
//
//    Clump *noise = mParams.addClump("Noise Source", "Noise");
//
//    noise->addParam("Spectrum")
//        .value_strings(noise_spectrum_strings)
//        .default_value(NoiseSource::White);
//
//
//    Clump *mixer = mParams.addClump("Mixer", "Mix");
//
//    mixer->addParam("Operator")
//        .value_strings(mix_operator_strings)
//        .default_value(Mixer::Mix);
//
//    mixer->addParam("Oscillator 1 Level")
//        .min_max(-40, 0)
//        .default_value(0)
//        .units(kAudioUnitParameterUnit_Decibels);
//
//    mixer->addParam("Oscillator 2 Level")
//        .min_max(-40, 0)
//        .default_value(-40)
//        .units(kAudioUnitParameterUnit_Decibels);
//
//    mixer->addParam("Noise Level")
//        .min_max(-40, 0)
//        .default_value(-40)
//        .units(kAudioUnitParameterUnit_Decibels);
//
//
//    Clump *filter = mParams.addClump("Filter", "Flt");
//
//    filter->addParam("Type")
//        .value_strings(filter_type_strings)
//        .default_value(Filter::LowPass);
//   
//    filter->addParam("Cutoff Frequency")
//        .min_max(20, 20000)
//        .default_value(20000)
//        .units(kAudioUnitParameterUnit_Hertz);
//
//    filter->addParam("Resonance")
//        .min_max(0, 4)
//        .default_value(0)
//        .units(kAudioUnitParameterUnit_LinearGain);
//
//   filter->addParam("Drive")
//        .min_max(-24, 0)
//        .default_value(-24)
//        .units(kAudioUnitParameterUnit_Decibels);
//
//
//    Clump *amp = mParams.addClump("Amplifier", "Amp");
//
//    amp->addParam("Attack Time")
//        .min_max(0, 9.999)
//        .default_value(0.001)
//        .units(kAudioUnitParameterUnit_Seconds)
//        .flag(kAudioUnitParameterFlag_DisplayCubeRoot);
//   
//    amp->addParam("Decay Time")
//        .min_max(0, 9.999)
//        .default_value(0.100)
//        .units(kAudioUnitParameterUnit_Seconds)
//        .flag(kAudioUnitParameterFlag_DisplayCubeRoot);
//   
//    amp->addParam("Sustain Level")
//        .min_max(0, 1)
//        .default_value(1)
//        .units(kAudioUnitParameterUnit_LinearGain);
//   
//    amp->addParam("Release Time")
//    .min_max(0, 9.999)
//    .default_value(0.050)
//    .units(kAudioUnitParameterUnit_Seconds)
//    .flag(kAudioUnitParameterFlag_DisplayCubeRoot);
//
//    // LFO 1...
//    // LFO 2...
//    // envelope 2...
//
//}

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
#if 1

    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    if (inParameterID >= mParams.size())
        return kAudioUnitErr_InvalidParameter;

    outParameterInfo = mParams.param_info(inParameterID);
    return noErr;
    
#else
    AudioUnitParameterInfo &info = outParameterInfo;
    info.flags = (kAudioUnitParameterFlag_IsWritable |
                  kAudioUnitParameterFlag_IsReadable);

    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;


    switch (inParameterID) {

    case kParameter_Osc1Waveform:
        FillInParameterName(info, kParamName_Osc1Waveform, false);
        HasClump(info, kParamClump_Osc1);
        info.unit         = kAudioUnitParameterUnit_Indexed;
        info.minValue     = 0;
        info.maxValue     = kNumberOfWaveforms - 1;
        info.defaultValue = kWaveform_Saw;
        info.flags       |= 0;
        break;

    case kParameter_Osc1WaveSkew:
        FillInParameterName(info, kParamName_Osc1WaveSkew, false);
        HasClump(info, kParamClump_Osc1);
        info.unit         = kAudioUnitParameterUnit_Percent;
        info.minValue     =   0.0;
        info.maxValue     = 100.0;
        info.defaultValue =   0.0;
        info.flags       |= 0;
        break;

    case kParameter_Osc1VibratoDepth:
        FillInParameterName(info, kParamName_Osc1VibratoDepth, false);
        HasClump(info, kParamClump_Osc1);
        info.unit         = kAudioUnitParameterUnit_Cents;
        info.minValue     = 0;
        info.maxValue     = 600;
        info.defaultValue = 0;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    case kParameter_Osc1VibratoSpeed:
        FillInParameterName(info, kParamName_Osc1VibratoSpeed, false);
        HasClump(info, kParamClump_Osc1);
        info.unit         = kAudioUnitParameterUnit_Hertz;
        info.minValue     =  0.1;
        info.maxValue     = 25.0;
        info.defaultValue =  3.0;
        info.flags       |= kAudioUnitParameterFlag_DisplayLogarithmic;
        break;

    case kParameter_Osc1VibratoWaveform:
        FillInParameterName(info, kParamName_Osc1VibratoWaveform, false);
        HasClump(info, kParamClump_Osc1);
        info.unit         = kAudioUnitParameterUnit_Indexed;
        info.minValue     = 0;
        info.maxValue     = kNumberOfWaveforms - 1;
        info.defaultValue = kWaveform_Sine;
        info.flags       |= 0;
        break;

    case kParameter_Osc2CoarseDetune:
        FillInParameterName(info, kParamName_Osc2CoarseDetune, false);
        HasClump(info, kParamClump_Osc2);
        info.unit         = kAudioUnitParameterUnit_MIDINoteNumber;
        info.minValue     = -60;
        info.maxValue     = +60;
        info.defaultValue = 0;
        info.flags       |= 0;
        break;

    case kParameter_Osc2FineDetune:
        FillInParameterName(info, kParamName_Osc2FineDetune, false);
        HasClump(info, kParamClump_Osc2);
        info.unit         = kAudioUnitParameterUnit_Cents;
        info.minValue     = -100;
        info.maxValue     = +100;
        info.defaultValue = 0;
        info.flags       |= 0;
        break;
        
    case kParameter_Osc2Waveform:
        FillInParameterName(info, kParamName_Osc2Waveform, false);
        HasClump(info, kParamClump_Osc2);
        info.unit         = kAudioUnitParameterUnit_Indexed;
        info.minValue     = 0;
        info.maxValue     = kNumberOfWaveforms - 1;
        info.defaultValue = kWaveform_Square;
        info.flags       |= 0;
        break;

    case kParameter_Osc2WaveSkew:
        FillInParameterName(info, kParamName_Osc2WaveSkew, false);
        HasClump(info, kParamClump_Osc2);
        info.unit         = kAudioUnitParameterUnit_Percent;
        info.minValue     =   0.0;
        info.maxValue     = 100.0;
        info.defaultValue =   0.0;
        info.flags       |= 0;
        break;

    case kParameter_Osc2VibratoDepth:
        FillInParameterName(info, kParamName_Osc2VibratoDepth, false);
        HasClump(info, kParamClump_Osc2);
        info.unit         = kAudioUnitParameterUnit_Cents;
        info.minValue     = 0;
        info.maxValue     = 600;
        info.defaultValue = 0;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    case kParameter_Osc2VibratoSpeed:
        FillInParameterName(info, kParamName_Osc2VibratoSpeed, false);
        HasClump(info, kParamClump_Osc2);
        info.unit         = kAudioUnitParameterUnit_Hertz;
        info.minValue     =  0.1;
        info.maxValue     = 25.0;
        info.defaultValue =  3.0;
        info.flags       |= kAudioUnitParameterFlag_DisplayLogarithmic;
        break;

    case kParameter_Osc2VibratoWaveform:
        FillInParameterName(info, kParamName_Osc2VibratoWaveform, false);
        HasClump(info, kParamClump_Osc2);
        info.unit         = kAudioUnitParameterUnit_Indexed;
        info.minValue     = 0;
        info.maxValue     = kNumberOfWaveforms - 1;
        info.defaultValue = kWaveform_Sine;
        info.flags       |= 0;
        break;
        
    case kParameter_NoiseType:
        FillInParameterName(info, kParamName_NoiseType, false);
        HasClump(info, kParamClump_Noise);
        info.unit         = kAudioUnitParameterUnit_Indexed;
        info.minValue     = 0;
        info.maxValue     = kNumberOfNoiseTypes;
        info.defaultValue = kNoiseType_White;
        info.flags       |= 0;
        break;

    case kParameter_NoiseAttackTime:
        FillInParameterName(info, kParamName_NoiseAttackTime, false);
        HasClump(info, kParamClump_Noise);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     = 0.000;
        info.maxValue     = 9.999;
        info.defaultValue = 0.001;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    case kParameter_NoiseDecayTime:
        FillInParameterName(info, kParamName_NoiseDecayTime, false);
        HasClump(info, kParamClump_Noise);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     =  0.000;
        info.maxValue     =  9.999;
        info.defaultValue =  0.100;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    case kParameter_NoiseSustainLevel:
        FillInParameterName(info, kParamName_NoiseSustainLevel, false);
        HasClump(info, kParamClump_Noise);
        info.unit         = kAudioUnitParameterUnit_LinearGain;
        info.minValue     = 0.0;
        info.maxValue     = 1.0;
        info.defaultValue = 1.0;
        info.flags       |= 0;
        break;

    case kParameter_NoiseReleaseTime:
        FillInParameterName(info, kParamName_NoiseReleaseTime, false);
        HasClump(info, kParamClump_Noise);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     = 0.000;
        info.maxValue     = 9.999;
        info.defaultValue = 0.050;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    case kParameter_Osc1Level:
        FillInParameterName(info, kParamName_Osc1Level, false);
        HasClump(info, kParamClump_Mixer);
        info.unit         = kAudioUnitParameterUnit_Decibels;
        info.minValue     = -40;
        info.maxValue     =   0;
        info.defaultValue =   0;
        info.flags       |= 0;
       break;

    case kParameter_Osc2Level:
        FillInParameterName(info, kParamName_Osc2Level, false);
        HasClump(info, kParamClump_Mixer);
        info.unit         = kAudioUnitParameterUnit_Decibels;
        info.minValue     = -40;
        info.maxValue     =   0;
        info.defaultValue = -40;
        info.flags       |= 0;
       break;

    case kParameter_NoiseLevel:
        FillInParameterName(info, kParamName_NoiseLevel, false);
        HasClump(info, kParamClump_Mixer);
        info.unit         = kAudioUnitParameterUnit_Decibels;
        info.minValue     = -40;
        info.maxValue     =   0;
        info.defaultValue = -40;
        info.flags       |= 0;
        break;

    case kParameter_AmpAttackTime:
        FillInParameterName(info, kParamName_AmpAttackTime, false);
        HasClump(info, kParamClump_Amplitude);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     = 0.000;
        info.maxValue     = 9.999;
        info.defaultValue = 0.001;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    case kParameter_AmpDecayTime:
        FillInParameterName(info, kParamName_AmpDecayTime, false);
        HasClump(info, kParamClump_Amplitude);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     =  0.000;
        info.maxValue     =  9.999;
        info.defaultValue =  0.100;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    case kParameter_AmpSustainLevel:
        FillInParameterName(info, kParamName_AmpSustainLevel, false);
        HasClump(info, kParamClump_Amplitude);
        info.unit         = kAudioUnitParameterUnit_LinearGain;
        info.minValue     = 0.0;
        info.maxValue     = 1.0;
        info.defaultValue = 1.0;
        info.flags       |= 0;
        break;

    case kParameter_AmpReleaseTime:
        FillInParameterName(info, kParamName_AmpReleaseTime, false);
        HasClump(info, kParamClump_Amplitude);
        info.unit         = kAudioUnitParameterUnit_Seconds;
        info.minValue     = 0.000;
        info.maxValue     = 9.999;
        info.defaultValue = 0.050;
        info.flags       |= kAudioUnitParameterFlag_DisplayCubeRoot;
        break;

    default:
        return kAudioUnitErr_InvalidParameter;
    }

    return noErr;

#endif

}

OSStatus MVS::GetParameterValueStrings(AudioUnitScope       inScope,
                                       AudioUnitParameterID inParameterID,
                                       CFArrayRef          *outStrings)
{
#if 1
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    CFArrayRef array = mParams.param_value_strings(inParameterID);
    if (!array)
        return kAudioUnitErr_InvalidParameter;

    if (outStrings)
        *outStrings = array;

    return noErr;
#else
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    if (inParameterID == kParameter_Osc1Waveform ||
        inParameterID == kParameter_Osc1VibratoWaveform ||
        inParameterID == kParameter_Osc2Waveform ||
        inParameterID == kParameter_Osc2VibratoWaveform) {
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
    if (inParameterID == kParameter_NoiseType) {
        if (outStrings == NULL)
            return noErr;

        // Defines an array that contains the pop-up menu item names.
        CFStringRef strings [] = {
            kMenuItem_Noise_White,
            kMenuItem_Noise_Pink,
            kMenuItem_Noise_Red,
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
#endif
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

//    switch (inClumpID) {
//       case kParamClump_Osc1:
//            *outClumpName = kClumpName_Osc1;
//            break;
//
//        case kParamClump_Osc2:
//            *outClumpName = kClumpName_Osc2;
//            break;
//
//        case kParamClump_Noise:
//            *outClumpName = kClumpName_Noise;
//            break;
//
//        case kParamClump_Mixer:
//            *outClumpName = kClumpName_Mixer;
//            break;
//
//        case kParamClump_Filter:
//            *outClumpName = kClumpName_Filter;
//            break;
//
//        case kParamClump_Amplitude:
//            *outClumpName = kClumpName_Amp;
//            break;
//
//        default:
//            return kAudioUnitErr_InvalidPropertyValue;
//    }
//    return noErr;
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
    mParams.set_param_value(inID, inValue);

    return AUMonotimbralInstrumentBase::SetParameter(inID,
                                                     inScope,
                                                     inElement,
                                                     mParams.param_value(inID),
                                                     inBufferOffsetInFrames);

//    // Controller sends CC values 0/127, 1/127, 2/127.  We map those to [0..2].
//    float value = mParams.scaleValue(in)
//    if (inID == kParameter_Osc1Waveform && inValue < 1.0)
//        inValue *= 127.0 / (kNumberOfWaveforms - 1);
//    if (inID == kParameter_Osc1VibratoWaveform && inValue < 1.0)
//        inValue *= 127.0 / (kNumberOfWaveforms - 1);
//    if (inID == kParameter_Osc2Waveform && inValue < 1.0)
//        inValue *= 127.0 / (kNumberOfWaveforms - 1);
//    if (inID == kParameter_Osc2VibratoWaveform && inValue < 1.0)
//        inValue *= 127.0 / (kNumberOfWaveforms - 1);
//    if (inID == kParameter_NoiseType && inValue < 1.0)
//        inValue *= 127.0 / (kNumberOfNoiseTypes - 1);
//
//    return AUMonotimbralInstrumentBase::SetParameter(inID,
//                                                     inScope,
//                                                     inElement,
//                                                     inValue,
//                                                     inBufferOffsetInFrames);
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

    UInt32 numGroups = Groups().GetNumberOfElements();
    for (UInt32 j = 0; j < numGroups; j++) {
        SynthGroupElement *group = (SynthGroupElement*)Groups().GetElement(j);
        OSStatus err = group->Render((SInt64)inTimeStamp.mSampleTime,
                                     inNumberFrames,
                                     outputs);
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

bool MVSNote::Attack(const MusicDeviceNoteParams &inParams)
{
    Float64 sampleRate   = SampleRate();
    Float32 maxLevel     = powf(inParams.mVelocity/127.0, 3.0);

#if 0
    Float32 noiseAttack  = GetGlobalParameter(kParameter_NoiseAttackTime);
    Float32 noiseDecay   = GetGlobalParameter(kParameter_NoiseDecayTime);
    Float32 noiseSustain = GetGlobalParameter(kParameter_NoiseSustainLevel);
    Float32 noiseRelease = GetGlobalParameter(kParameter_NoiseReleaseTime);

    Float32 attackTime   = GetGlobalParameter(kParameter_AmpAttackTime);
    Float32 decayTime    = GetGlobalParameter(kParameter_AmpDecayTime);
    Float32 sustainLevel = GetGlobalParameter(kParameter_AmpSustainLevel);
    Float32 releaseTime  = GetGlobalParameter(kParameter_AmpReleaseTime);
#else
    float attackTime   = mParams->amp_attack;
    float decayTime    = mParams->amp_decay;
    float sustainLevel = mParams->amp_sustain;
    float releaseTime  = mParams->amp_release;
#endif
//    attackTime = 0.01;
//    decayTime = 0.1;
//    sustainLevel = 1;
//    releaseTime = 0.05;
//
//    noiseAttack = 0.01;
//    noiseDecay = 0.1;
//    noiseSustain = 1;
//    noiseRelease = 0.05;

    printf("Attack\n");
    printf("    sample rate = %g\n", sampleRate);
    printf("    A D S R = %g %g %g %g\n", attackTime, decayTime, sustainLevel, releaseTime);
//    printf("   nA D S R = %g %g %g %g\n", noiseAttack, noiseDecay, noiseSustain, noiseRelease);
    printf("\n");

//    mOsc1LFO.initialize(sampleRate);
    mOsc1.initialize(sampleRate);
//    mOsc2LFO.initialize(sampleRate);
    mOsc2.initialize(sampleRate);
    mNoise.initialize(sampleRate);
//    mNoiseEnv.initialize(sampleRate,
//                         1.0,
//                         noiseAttack, noiseDecay, noiseSustain, noiseRelease,
//                         Envelope::Exponential);
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
    Float32 *outBuf     = *mOversampleBufPtr;
    UInt32   frameCount = inNumFrames * mOversampleRatio;

    // Get parameters.
#if 1
    Float32 osc1skew   = mParams->o1_width / 100;
    Float32 o2coarsetune = mParams->o2_coarse_detune;
    Float32 o2finetune = mParams->o2_fine_detune;
    Float32 osc2skew   = mParams->o2_width / 100;
    int ntyp           = mParams->noise_spectrum;
    Float32 o1level    = mParams->mix_osc1_level;
    Float32 o2level    = mParams->mix_osc2_level;
    Float32 nlevel     = mParams->mix_noise_level;
    int     o1wf       = mParams->o1_waveform;
    int     o2wf       = mParams->o2_waveform;
#else
    Float32 osc1skew   = GetGlobalParameter(kParameter_Osc1WaveSkew) / 100.0;
    Float32 osc1vibdep = GetGlobalParameter(kParameter_Osc1VibratoDepth) / 1200;
    Float32 osc1vibspd = GetGlobalParameter(kParameter_Osc1VibratoSpeed);
    Float32 o2coarsetune = GetGlobalParameter(kParameter_Osc2CoarseDetune);
    Float32 o2finetune = GetGlobalParameter(kParameter_Osc2FineDetune);
    Float32 osc2skew   = GetGlobalParameter(kParameter_Osc2WaveSkew) / 100.0;
    Float32 osc2vibdep = GetGlobalParameter(kParameter_Osc2VibratoDepth) / 1200;
    Float32 osc2vibspd = GetGlobalParameter(kParameter_Osc2VibratoSpeed);
    Float32 ntyp       = GetGlobalParameter(kParameter_NoiseType);
    Float32 o1level    = GetGlobalParameter(kParameter_Osc1Level);
    Float32 o2level    = GetGlobalParameter(kParameter_Osc2Level);
    Float32 nlevel     = GetGlobalParameter(kParameter_NoiseLevel);
    int     o1wf       = mParams->o1_waveform;
    //    Float32 o1vwf      = GetGlobalParameter(kParameter_Osc1VibratoWaveform);
    int     o2wf       = mParams->o2_waveform;
    //    Float32 o2vwf      = GetGlobalParameter(kParameter_Osc2VibratoWaveform);
#endif

#if 1
    Oscillator::Type o1type    = (Oscillator::Type)o1wf;
    Oscillator::Type o2type    = (Oscillator::Type)o2wf;
#else
    Oscillator::Type o1type    = OscillatorType((Waveform)(o1wf + 0.5));
    Oscillator::Type o1LFOtype = OscillatorType((Waveform)(o1vwf + 0.5));
    Oscillator::Type o2type    = OscillatorType((Waveform)(o2wf + 0.5));
    Oscillator::Type o2LFOtype = OscillatorType((Waveform)(o2vwf + 0.5));
#endif

    printf("MVSNote::Render\n");
    printf("    o1  = f=%g s=%g\n", Frequency(), osc1skew);
    printf("    o2  = t=%g/%g s=%g\n", o2coarsetune, o2finetune, osc2skew);
    printf("    n   = %d\n", ntyp);
    printf("    mix = 1=%g 2=%g n=%g\n", o1level, o2level, nlevel);
    printf("\n");

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
//    Float32 noiseEnv[frameCount];
    UInt32 noiseFrameCount = 0;
    if (nlevel) {
        noiseFrameCount = frameCount;
//        UInt32 noiseEnd = mNoiseEnv.generate(noiseEnv, frameCount);
//        if (noiseEnd != 0xFFFFFFFF)
//            noiseFrameCount = noiseEnd;
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

//Oscillator::Type MVSNote::OscillatorType(Waveform waveform)
//{
//    switch (waveform) {
//
//    case kWaveform_Saw:
//        return Oscillator::Saw;
//        break;
//
//    case kWaveform_Square:
//        return Oscillator::Square;
//        break;
//
//    case kWaveform_Triangle:
//        return Oscillator::Triangle;
//        break;
//
//    case kWaveform_Sine:
//        return Oscillator::Sine;
//        break;
//
//    default:
//        return Oscillator::Saw;
//        break;
//    }
//}

//NoiseSource::Type MVSNote::NoiseType(enum NoiseType ntype)
//{
//    switch (ntype) {
//
//    case kNoiseType_White:
//        return NoiseSource::White;
//        break;
//
//    case kNoiseType_Pink:
//        return NoiseSource::Pink;
//        break;
//
//    case kNoiseType_Red:
//        return NoiseSource::Red;
//        break;
//
//    default:
//        return NoiseSource::White;
//    }
//}
