//
//  MVS.cpp
//  MVS - Minimum Viable Synth
//
//  Created by Bob Miller on 11/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

/*
	This is a test implementation of a sin wave synth using AUInstrumentBase
    classes
	
	It illustrates a basic usage of these classes
	
	It artificially limits the number of notes at one time to 12, so the
    note-stealing algorithm is used - you should know how this works!
	
	Most of the work you need to do is defining a Note class (see MVSNote).
    AUInstrument manages the creation and destruction of notes, the various
    stages of a note's lifetime.

	Alot of printfs have been left in (but are if'def out).  These can be
    useful as you figure out how this all fits together. This is true in the
    AUInstrumentBase classes as well; simply define DEBUG_PRINT to 1 and this
    turns all this on.

	The project also defines CA_AUTO_MIDI_MAP (OTHER_C_FLAGS). This adds all
    the code that is needed to map MIDI messages to specific parameter changes.
    This can be seen in AU Lab's MIDI Editor window
	CA_AUTO_MIDI_MAP is implemented in AUMIDIBase.cpp/.h
*/

#include "MVS.h"
 
static const UInt32 kMaxActiveNotes = 8;

static CFStringRef kParamName_AmpAttackTime  = CFSTR("Amplitude Attack Time");
static CFStringRef kParamName_AmpDecayTime   = CFSTR("Amplitude Decay Time");
static CFStringRef kParamName_AmpSustainLevel= CFSTR("Amplitude Sustain Level");
static CFStringRef kParamName_AmpReleaseTime = CFSTR("Amplitude Release Time");

////////////////////////////////////////////////////////////////////////////////

#pragma mark MVS Methods

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, MVS)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	MVS::MVS
//
// This synth has No inputs, One output
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MVS::MVS(AudioUnit inComponentInstance)
	: AUMonotimbralInstrumentBase(inComponentInstance, 0, 1)
{
	CreateElements();

	Globals()->UseIndexedParameters(kNumberOfParameters);
    Globals()->SetParameter(kParameter_AmpAttackTime,   0.05);
    Globals()->SetParameter(kParameter_AmpDecayTime,    0.05);
    Globals()->SetParameter(kParameter_AmpSustainLevel, 1.00);
    Globals()->SetParameter(kParameter_AmpReleaseTime,  0.05);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	MVS::~MVS
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MVS::~MVS()
{}

void MVS::Cleanup()
{
#if DEBUG_PRINT
	printf("MVS::Cleanup\n");
#endif
}

OSStatus MVS::Initialize()
{	
#if DEBUG_PRINT
	printf("->MVS::Initialize\n");
#endif
	AUMonotimbralInstrumentBase::Initialize();
	
	SetNotes(kNumNotes, kMaxActiveNotes, mNotes, sizeof(MVSNote));
#if DEBUG_PRINT
	printf("<-MVS::Initialize\n");
#endif
	
	return noErr;
}

AUElement* MVS::CreateElement(AudioUnitScope					scope,
									AudioUnitElement				element)
{
	switch (scope)
	{
		case kAudioUnitScope_Group :
			return new SynthGroupElement(this, element, new MidiControls);
		case kAudioUnitScope_Part :
			return new SynthPartElement(this, element);
		default :
			return AUBase::CreateElement(scope, element);
	}
}

OSStatus MVS::GetParameterInfo(AudioUnitScope		   inScope,
                               AudioUnitParameterID	   inParameterID,
                               AudioUnitParameterInfo &outParameterInfo)
{
    AudioUnitParameterInfo &info = outParameterInfo;
    info.flags = (kAudioUnitParameterFlag_IsWritable |
                  kAudioUnitParameterFlag_IsReadable);

	if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidScope;

    switch (inParameterID) {

        case kParameter_AmpAttackTime:
            AUBase::FillInParameterName(info, kParamName_AmpAttackTime, false);
            info.unit         = kAudioUnitParameterUnit_Seconds;
            info.minValue     = 0.0;
            info.maxValue     = 2.00;
            info.defaultValue = 0.05;
            info.flags       |= kAudioUnitParameterFlag_DisplayExponential;
            break;

        case kParameter_AmpDecayTime:
            AUBase::FillInParameterName(info, kParamName_AmpDecayTime, false);
            info.unit         = kAudioUnitParameterUnit_Seconds;
            info.minValue     = 0.00;
            info.maxValue     = 2.00;
            info.defaultValue = 0.05;
            info.flags       |= kAudioUnitParameterFlag_DisplayExponential;
            break;

        case kParameter_AmpSustainLevel:
            AUBase::FillInParameterName(info,
                                        kParamName_AmpSustainLevel,
                                        false);
            info.unit         = kAudioUnitParameterUnit_Decibels;
            info.minValue     = -20.0;
            info.maxValue     =   0.0;
            info.defaultValue =   0.0;
            info.flags       |=     0;
            break;

        case kParameter_AmpReleaseTime:
            AUBase::FillInParameterName(info, kParamName_AmpReleaseTime, false);
            info.unit         = kAudioUnitParameterUnit_Seconds;
            info.minValue     = 0.0;
            info.maxValue     = 2.0;
            info.defaultValue = 0.05;
            info.flags |= kAudioUnitParameterFlag_DisplayExponential;
            break;

        default:
            return kAudioUnitErr_InvalidParameter;
    }

	return noErr;
}


#pragma mark MVSNote Methods

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void			MVSNote::Release(UInt32 inFrame)
{
	SynthNote::Release(inFrame);
#if DEBUG_PRINT
	printf("MVSNote::Release %p %d\n", this, GetState());
#endif
}

void			MVSNote::FastRelease(UInt32 inFrame) // voice is being stolen.
{
	SynthNote::Release(inFrame);
#if DEBUG_PRINT
	printf("MVSNote::Release %p %d\n", this, GetState());
#endif
}

void			MVSNote::Kill(UInt32 inFrame) // voice is being stolen.
{
	SynthNote::Kill(inFrame);
#if DEBUG_PRINT
	printf("MVSNote::Kill %p %d\n", this, GetState());
#endif
}

OSStatus MVSNote::Render(UInt64            inAbsoluteSampleFrame,
                         UInt32            inNumFrames,
                         AudioBufferList **inBufferList,
                         UInt32            inOutBusCount)
{
	float *left, *right;
/* ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~	
	Changes to this parameter (kGlobalVolumeParam) are not being de-zippered; 
	Left as an exercise for the reader
 ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ */

	// MVSNote only writes into the first bus regardless of what is handed to us.
	const int bus0 = 0;
	int numChans = inBufferList[bus0]->mNumberBuffers;
	if (numChans > 2) return -1;

	left = (float*)inBufferList[bus0]->mBuffers[0].mData;
	right = numChans == 2 ? (float*)inBufferList[bus0]->mBuffers[1].mData : 0;

    Float32 osc1buf[inNumFrames], ampbuf[inNumFrames];
    for (size_t i = 0; i < inNumFrames; i++) {
        osc1buf[i] = ampbuf[i] = 0;
    }
    SynthNoteState state = GetState();
    if (state == kNoteState_Released || state == kNoteState_FastReleased)
        mAmpEnv.release();
    UInt32 end = mAmpEnv.generate(ampbuf, inNumFrames);
    UInt32 frameCount = inNumFrames;
    if (end != 0xFFFFFFFF)
        frameCount = end;
    assert(frameCount <= inNumFrames);
    mOsc1.generate(Frequency(), osc1buf, frameCount);
    if (right) {
        for (UInt32 i = 0; i < frameCount; i++) {
            Float32 out = osc1buf[i] * ampbuf[i];
            left[i] += out;
            right[i] += out;
        }
    } else {
        for (UInt32 i = 0; i < frameCount; i++) {
            Float32 out = osc1buf[i] * ampbuf[i];
            left[i] += out;
        }
    }

    if (end != 0xFFFFFFFF)
        NoteEnded(end);

	return noErr;
}

