//
//  MVS.h
//  MVS
//
//  Created by Bob Miller on 11/16/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS__
#define __MVS__

#include "AUInstrumentBase.h"
#include "MVSVersion.h"

static const UInt32 kNumNotes = 12;

struct MVSNote : public SynthNote
{
	virtual					~MVSNote() {}

	virtual bool			Attack(const MusicDeviceNoteParams &inParams)
								{ 
#if DEBUG_PRINT
									printf("MVSNote::Attack %p %d\n", this, GetState());
#endif
									double sampleRate = SampleRate();
									phase = 0.;
									amp = 0.;
									maxamp = 0.4 * pow(inParams.mVelocity/127., 3.); 
									up_slope = maxamp / (0.1 * sampleRate);
									dn_slope = -maxamp / (0.9 * sampleRate);
									fast_dn_slope = -maxamp / (0.005 * sampleRate);
									return true;
								}
	virtual void			Kill(UInt32 inFrame); // voice is being stolen.
	virtual void			Release(UInt32 inFrame);
	virtual void			FastRelease(UInt32 inFrame);
	virtual Float32			Amplitude() { return amp; } // used for finding quietest note for voice stealing.
	virtual OSStatus		Render(UInt64 inAbsoluteSampleFrame, UInt32 inNumFrames, AudioBufferList** inBufferList, UInt32 inOutBusCount);

	double phase, amp, maxamp;
	double up_slope, dn_slope, fast_dn_slope;
};

class MVS : public AUMonotimbralInstrumentBase
{
public:
								MVS(AudioUnit inComponentInstance);
	virtual						~MVS();
				
	virtual OSStatus			Initialize();
	virtual void				Cleanup();
	virtual OSStatus			Version() { return kMVSVersion; }

	virtual AUElement*			CreateElement(			AudioUnitScope					scope,
											  AudioUnitElement				element);

	virtual OSStatus			GetParameterInfo(		AudioUnitScope					inScope,
														AudioUnitParameterID			inParameterID,
														AudioUnitParameterInfo &		outParameterInfo);

	MidiControls*				GetControls( MusicDeviceGroupID inChannel)
	{
		SynthGroupElement *group = GetElForGroupID(inChannel);
		return (MidiControls *) group->GetMIDIControlHandler();
	}
	
private:
	
	MVSNote mNotes[kNumNotes];
};

#endif /* !__MVS__ */
