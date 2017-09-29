//
//  FeedbackXBOEffect.hpp
//  XBOBTFF
//
//  Created by C.W. Betts on 9/20/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#ifndef FeedbackXBOEffect_hpp
#define FeedbackXBOEffect_hpp

#include <ForceFeedback/IOForceFeedbackLib.h>
#include <math.h>
#include <string.h>
#include <algorithm>

//----------------------------------------------------------------------------------------------
//    Effects
//----------------------------------------------------------------------------------------------

#define CONSTANT_FORCE	0x00
#define RAMP_FORCE		0x01
#define SQUARE			0x02
#define SINE			0x03
#define TRIANGLE		0x04
#define SAWTOOTH_UP		0x05
#define SAWTOOTH_DOWN	0x06
#define SPRING			0x07
#define DAMPER			0x08
#define INERTIA			0x09
#define FRICTION		0x0A
#define CUSTOM_FORCE	0x0B

#define SCALE_MAX (LONG)101

double CurrentTimeUsingMach();

class FeedbackXBOEffect
{
public:
	FeedbackXBOEffect(FFEffectDownloadID theHand);
	FeedbackXBOEffect(const FeedbackXBOEffect &src);
	
	LONG Calc(LONG *LeftLevel, LONG *RightLevel, LONG *ltLevel, LONG *rtLevel);
	
	CFUUIDRef			Type;
	FFEffectDownloadID	Handle;
	
	FFEFFECT		DiEffect;
	FFENVELOPE		DiEnvelope;
	FFCONSTANTFORCE	DiConstantForce;
	FFCUSTOMFORCE	DiCustomForce;
	FFPERIODIC		DiPeriodic;
	FFRAMPFORCE		DiRampforce;
	
	DWORD			Status;
	DWORD			PlayCount;
	double			StartTime;
	
	double			LastTime;
	DWORD			Index;
	
private:
	FeedbackXBOEffect();
	void CalcEnvelope(ULONG Duration, ULONG CurrentPos, LONG *NormalRate, LONG *AttackLevel, LONG *FadeLevel);
	void CalcForce(ULONG Duration, ULONG CurrentPos, LONG NormalRate, LONG AttackLevel, LONG FadeLevel, LONG * NormalLevel);
};

#endif /* FeedbackXBOEffect_hpp */
