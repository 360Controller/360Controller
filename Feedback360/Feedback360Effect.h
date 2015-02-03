/*
    MICE Xbox 360 Controller driver for Mac OS X
    Force Feedback module
    Copyright (C) 2013 David Ryskalczyk
    based on xi, Copyright (C) 2011 Masahiko Morii

    Feedback360Effect.cpp - Main code for the FF plugin
    
    This file is part of Xbox360Controller.

    Xbox360Controller is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Xbox360Controller is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Xbox360Controller; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

//	Force Feedback Driver for XInput

#ifndef Feedback360_Feedback360Effect_h
#define Feedback360_Feedback360Effect_h

#include <IOKit/IOCFPlugIn.h>
#include <ForceFeedback/IOForceFeedbackLib.h>
#include <math.h>
#include <string.h>
#include <algorithm>

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

#define SCALE_MAX (LONG)255

double CurrentTimeUsingMach();

class Feedback360Effect
{
public:
    Feedback360Effect(FFEffectDownloadID theHand);
    Feedback360Effect(const Feedback360Effect &src);

    LONG Calc(LONG *LeftLevel, LONG *RightLevel);

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
    double			StartTime;

    double			LastTime;
    DWORD           Index;

private:
    Feedback360Effect();
    void CalcEnvelope(ULONG Duration, ULONG CurrentPos, LONG *NormalRate, LONG *AttackLevel, LONG *FadeLevel);
    void CalcForce(ULONG Duration, ULONG CurrentPos, LONG NormalRate, LONG AttackLevel, LONG FadeLevel, LONG * NormalLevel);
};

#endif
