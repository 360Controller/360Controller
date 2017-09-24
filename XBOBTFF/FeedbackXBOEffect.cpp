//
//  FeedbackXBOEffect.cpp
//  XBOBTFF
//
//  Created by C.W. Betts on 9/20/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "FeedbackXBOEffect.hpp"

using std::max;
using std::min;

//----------------------------------------------------------------------------------------------
// CEffect
//----------------------------------------------------------------------------------------------
FeedbackXBOEffect::FeedbackXBOEffect() : Type(NULL), Status(0), PlayCount(0),
StartTime(0), Index(0), LastTime(0), Handle(0), DiEffect({0}), DiEnvelope({0}),
DiCustomForce({0}), DiConstantForce({0}), DiPeriodic({0}), DiRampforce({0})
{
	
}

FeedbackXBOEffect::FeedbackXBOEffect(FFEffectDownloadID theHand) : FeedbackXBOEffect()
{
	Handle = theHand;
}

FeedbackXBOEffect::FeedbackXBOEffect(const FeedbackXBOEffect &src) : Type(src.Type),
Handle(src.Handle), Status(src.Status), PlayCount(src.PlayCount),
StartTime(src.StartTime), Index(src.Index), LastTime(src.LastTime)
{
	memcpy(&DiEffect, &src.DiEffect, sizeof(FFEFFECT));
	memcpy(&DiEnvelope, &src.DiEnvelope, sizeof(FFENVELOPE));
	memcpy(&DiCustomForce, &src.DiCustomForce, sizeof(FFCUSTOMFORCE));
	memcpy(&DiConstantForce, &src.DiConstantForce, sizeof(FFCONSTANTFORCE));
	memcpy(&DiPeriodic, &src.DiPeriodic, sizeof(FFPERIODIC));
	memcpy(&DiRampforce, &src.DiRampforce, sizeof(FFRAMPFORCE));
}

//----------------------------------------------------------------------------------------------
// Calc
//----------------------------------------------------------------------------------------------
LONG FeedbackXBOEffect::Calc(LONG *LeftLevel, LONG *RightLevel, LONG *ltLevel, LONG *rtLevel)
{
	CFTimeInterval Duration = 0;
	if(DiEffect.dwDuration != FF_INFINITE) {
		Duration = max(1., DiEffect.dwDuration / 1000.) / 1000.;
	} else {
		Duration = DBL_MAX;
	}
	double BeginTime = StartTime + ( DiEffect.dwStartDelay / 1000. / 1000.);
	double EndTime  = DBL_MAX;
	if (PlayCount != -1)
	{
		EndTime = BeginTime + Duration * PlayCount;
	}
	double CurrentTime = CurrentTimeUsingMach();
	
	if (Status == FFEGES_PLAYING && BeginTime <= CurrentTime && CurrentTime <= EndTime)
	{
		// Used for force calculation
		LONG NormalLevel;
		LONG WorkLeftLevel;
		LONG WorkRightLevel;
		LONG WorkLTLevel;
		LONG WorkRTLevel;

		// Used for envelope calculation
		LONG NormalRate;
		LONG AttackLevel;
		LONG FadeLevel;
		
		CalcEnvelope((ULONG)(Duration*1000)
					 ,(ULONG)(fmod(CurrentTime - BeginTime, Duration)*1000)
					 ,&NormalRate
					 ,&AttackLevel
					 ,&FadeLevel);
		
		// CustomForce allows setting each channel separately
		if(CFEqual(Type, kFFEffectType_CustomForce_ID)) {
			if((CurrentTimeUsingMach() - LastTime)*1000*1000 < DiCustomForce.dwSamplePeriod) {
				return -1;
			}
			else {
				if (DiCustomForce.cChannels == 2) {
					WorkLeftLevel = ((DiCustomForce.rglForceData[2*Index] * NormalRate + AttackLevel + FadeLevel) / 100) * DiEffect.dwGain / 10000;
					WorkRightLevel = ((DiCustomForce.rglForceData[2*Index + 1] * NormalRate + AttackLevel + FadeLevel) / 100) * DiEffect.dwGain / 10000;
					WorkLTLevel = 0;
					WorkRTLevel = 0;
					//fprintf(stderr, "L:%d; R:%d\n", WorkLeftLevel, WorkRightLevel);
					Index = (Index + 1) % (DiCustomForce.cSamples/2);
					LastTime = CurrentTimeUsingMach();
				} else {
					WorkLeftLevel = ((DiCustomForce.rglForceData[4*Index] * NormalRate + AttackLevel + FadeLevel) / 100) * DiEffect.dwGain / 10000;
					WorkRightLevel = ((DiCustomForce.rglForceData[4*Index + 1] * NormalRate + AttackLevel + FadeLevel) / 100) * DiEffect.dwGain / 10000;
					WorkLTLevel = ((DiCustomForce.rglForceData[4*Index + 2] * NormalRate + AttackLevel + FadeLevel) / 100) * DiEffect.dwGain / 10000;
					WorkRTLevel = ((DiCustomForce.rglForceData[4*Index + 3] * NormalRate + AttackLevel + FadeLevel) / 100) * DiEffect.dwGain / 10000;
					//fprintf(stderr, "L:%d; R:%d\n", WorkLeftLevel, WorkRightLevel);
					Index = (Index + 1) % (DiCustomForce.cSamples/4);
					LastTime = CurrentTimeUsingMach();
				}
			}
		}
		// Regular commands treat controller as a single output (both channels are together as one)
		else {
			CalcForce(
					  (ULONG)(Duration*1000)
					  ,(ULONG)(fmod(CurrentTime - BeginTime, Duration)*1000)
					  ,NormalRate
					  ,AttackLevel
					  ,FadeLevel
					  ,&NormalLevel );
			//fprintf(stderr, "DeltaT %f\n", CurrentTime - BeginTime);
			//fprintf(stderr, "Duration %f; NormalRate: %d; AttackLevel: %d; FadeLevel: %d\n", Duration, NormalRate, AttackLevel, FadeLevel);
			
			WorkLeftLevel = (NormalLevel > 0) ? NormalLevel : -NormalLevel;
			WorkRightLevel = (NormalLevel > 0) ? NormalLevel : -NormalLevel;
			WorkLTLevel = 0;
			WorkRTLevel = 0;
		}
		WorkLeftLevel = min( SCALE_MAX, WorkLeftLevel * SCALE_MAX / 10000 );
		WorkRightLevel = min( SCALE_MAX, WorkRightLevel * SCALE_MAX / 10000 );
		WorkLTLevel = min( SCALE_MAX, WorkLTLevel * SCALE_MAX / 10000 );
		WorkRTLevel = min( SCALE_MAX, WorkRTLevel * SCALE_MAX / 10000 );

		*LeftLevel = *LeftLevel + WorkLeftLevel;
		*RightLevel = *RightLevel + WorkRightLevel;
		*ltLevel = *ltLevel + WorkLTLevel;
		*rtLevel = *rtLevel + WorkRTLevel;
	}
	return 0;
}

//----------------------------------------------------------------------------------------------
// CalcEnvelope
//----------------------------------------------------------------------------------------------
void FeedbackXBOEffect::CalcEnvelope(ULONG Duration, ULONG CurrentPos, LONG *NormalRate, LONG *AttackLevel, LONG *FadeLevel)
{
	if( ( DiEffect.dwFlags & FFEP_ENVELOPE ) && DiEffect.lpEnvelope != NULL )
	{
		// Calculate attack factor
		LONG	AttackRate	= 0;
		ULONG	AttackTime	= max( (DWORD)1, DiEnvelope.dwAttackTime / 1000 );
		if (CurrentPos < AttackTime)
		{
			AttackRate	= ( AttackTime - CurrentPos ) * 100 / AttackTime;
		}
		
		// Calculate fade factor
		LONG	FadeRate	= 0;
		ULONG	FadeTime	= max( (DWORD)1, DiEnvelope.dwFadeTime / 1000 );
		ULONG	FadePos		= Duration - FadeTime;
		if (FadePos < CurrentPos)
		{
			FadeRate	= ( CurrentPos - FadePos ) * 100 / FadeTime;
		}
		
		*NormalRate		= 100 - AttackRate - FadeRate;
		*AttackLevel	= DiEnvelope.dwAttackLevel * AttackRate;
		*FadeLevel		= DiEnvelope.dwFadeLevel * FadeRate;
	} else {
		*NormalRate		= 100;
		*AttackLevel	= 0;
		*FadeLevel		= 0;
	}
}

void FeedbackXBOEffect::CalcForce(ULONG Duration, ULONG CurrentPos, LONG NormalRate, LONG AttackLevel, LONG FadeLevel, LONG * NormalLevel)
{
	LONG Magnitude = 0;
	LONG Period;
	LONG R;
	LONG Rate;
	
	if (CFEqual(Type, kFFEffectType_ConstantForce_ID)) {
		Magnitude	= DiConstantForce.lMagnitude;
		Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
	}
	else if (CFEqual(Type, kFFEffectType_Square_ID)) {
		Period	= max( (DWORD)1, ( DiPeriodic.dwPeriod / 1000 ) );
		R		= ( CurrentPos%Period) * 360 / Period;
		R	= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
		
		Magnitude	= DiPeriodic.dwMagnitude;
		Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
		
		if (180 <= R)
		{
			Magnitude = Magnitude * -1;
		}
		
		Magnitude	= Magnitude + DiPeriodic.lOffset;
	}
	else if (CFEqual(Type, kFFEffectType_Sine_ID)) {
		Period	= max( (DWORD)1, ( DiPeriodic.dwPeriod / 1000 ) );
		R		= (CurrentPos%Period) * 360 / Period;
		R		= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
		
		Magnitude	= DiPeriodic.dwMagnitude;
		Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
		
		Magnitude	= ( int)( Magnitude * sin( R * M_PI / 180.0 ) );
		
		Magnitude	= Magnitude + DiPeriodic.lOffset;
	}
	else if (CFEqual(Type, kFFEffectType_Triangle_ID)) {
		Period	= max( (DWORD)1, ( DiPeriodic.dwPeriod / 1000 ) );
		R		= (CurrentPos%Period) * 360 / Period;
		R		= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
		
		Magnitude	= DiPeriodic.dwMagnitude;
		Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
		
		if (0 <= R && R < 90)
		{
			Magnitude	= -Magnitude * ( 90 - R ) / 90;
		}
		if (90 <= R && R < 180)
		{
			Magnitude	= Magnitude * ( R - 90 ) / 90;
		}
		if (180 <= R && R < 270)
		{
			Magnitude	= Magnitude * ( 90 - ( R - 180 ) ) / 90;
		}
		if (270 <= R && R < 360)
		{
			Magnitude	= -Magnitude * ( R - 270 ) / 90;
		}
		
		Magnitude	= Magnitude + DiPeriodic.lOffset;
	}
	else if(CFEqual(Type, kFFEffectType_SawtoothUp_ID)) {
		Period	= max( (DWORD)1, ( DiPeriodic.dwPeriod / 1000 ) );
		R		= (CurrentPos%Period) * 360 / Period;
		R		= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
		
		Magnitude	= DiPeriodic.dwMagnitude;
		Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
		
		if (0 <= R && R < 180)
		{
			Magnitude	= -Magnitude * ( 180 - R ) / 180;
		}
		if (180 <= R && R < 360)
		{
			Magnitude	= Magnitude * ( R - 180 ) / 180;
		}
		
		Magnitude	= Magnitude + DiPeriodic.lOffset;
	}
	else if (CFEqual(Type, kFFEffectType_SawtoothDown_ID)) {
		Period	= max( (DWORD)1, ( DiPeriodic.dwPeriod / 1000 ) );
		R		= (CurrentPos%Period) * 360 / Period;
		R		= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
		
		Magnitude	= DiPeriodic.dwMagnitude;
		Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
		if( 0 <= R && R < 180 )
		{
			Magnitude	= Magnitude * ( 180 - R ) / 180;
		}
		if( 180 <= R && R < 360 )
		{
			Magnitude	= -Magnitude * ( R - 180 ) / 180;
		}
		
		Magnitude	= Magnitude + DiPeriodic.lOffset;
	}
	else if (CFEqual(Type, kFFEffectType_RampForce_ID)) {
		Rate		= ( Duration - CurrentPos ) * 100
		/ Duration;//MAX( 1, DiEffect.dwDuration / 1000 );
		
		Magnitude	= ( DiRampforce.lStart * Rate
					   + DiRampforce.lEnd * ( 100 - Rate ) ) / 100;
		Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
	}
	
	*NormalLevel = Magnitude * (LONG)DiEffect.dwGain / 10000;
}
