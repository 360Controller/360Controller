
//	Force Feedback Driver for XInput

#ifndef Feedback360_Feedback360Effect_h
#define Feedback360_Feedback360Effect_h

#include <IOKit/IOCFPlugin.h>
#include <ForceFeedback/IOForceFeedbackLib.h>
#include	<math.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

//----------------------------------------------------------------------------------------------
//	Effects
//----------------------------------------------------------------------------------------------

#define	CONSTANT_FORCE	0x00
#define	RAMP_FORCE		0x01
#define	SQUARE			0x02
#define	SINE			0x03
#define	TRIANGLE		0x04
#define	SAWTOOTH_UP		0x05
#define	SAWTOOTH_DOWN	0x06
#define	SPRING			0x07
#define	DAMPER			0x08
#define	INERTIA			0x09
#define	FRICTION		0x0A
#define	CUSTOM_FORCE	0x0B

#define SCALE_MAX 255


class Feedback360Effect
{

public:

	Feedback360Effect();

    void Calc(int *LeftLevel, int *RightLevel);

	CFUUIDRef		Type;
    FFEffectDownloadID Handle;

	FFEFFECT		DiEffect;
    FFENVELOPE		DiEnvelope;
	FFCONSTANTFORCE	DiConstantForce;
    FFCUSTOMFORCE   DiCustomForce;
	FFPERIODIC		DiPeriodic;
	FFRAMPFORCE		DiRampforce;


	DWORD			Status;
	DWORD			PlayCount;
    DWORD           Index;
	CFAbsoluteTime	StartTime;

private:

    void CalcEnvelope(ULONG Duration, ULONG CurrentPos, LONG *NormalRate, LONG *AttackLevel, LONG *FadeLevel);
	void CalcForce(ULONG Duration, ULONG CurrentPos, LONG NormalRate, LONG AttackLevel, LONG FadeLevel, LONG * NormalLevel);
    
protected:
    
};

#endif