/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    emulator.h - Interface to fake FF API effects using rumble motors
    
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
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <Carbon/Carbon.h>

typedef enum {
    fsStop,
    fsPlay,
} ForceState;

typedef struct {
    bool inUse;
    ForceState state;
    // Information
    int gain;
    int maxLevel;
    int startCount;
    int attackLevel;
    int attackCount;
    int sustainLevel;
    int sustainCount;
    int fadeLevel;
    int fadeCount;
    // Runtime
    int countStart;
    int countAttack;
    int countSustain;
    int countFade;
    int countLoop;
} ForceEffect;

typedef struct {
    // Settings
    int gain;           // 0-maxLevel
    int maxLevel;
    int startDelay;     // Microseconds
    // Envelope for effect
    int attackLevel;    // 0-maxLevel
    int attackTime;     // Microseconds
    int sustainLevel;   // 0-maxLevel
    int sustainTime;    // Microseconds
    int fadeLevel;      // 0-maxLevel
    int fadeTime;       // Microseconds
} ForceParams;

typedef struct {
    // Callback
    void (*SetForce)(void *context,unsigned char large,unsigned char small);
    void *context;
    // Options
    int gain,maxGain;
    bool enable,pause;
    // Effect entries
    ForceEffect *effects;
    int effectCount;
    // Runtime
    CFRunLoopTimerRef timer;
    unsigned char oldLarge,oldSmall;
    int activeEffect;
} ForceEmulator;

// Constructor/destructor
void Emulate_Initialise(ForceEmulator *emulator,int effectCount,void (*SetForce)(void*,unsigned char,unsigned char),void *context);
void Emulate_Finalise(ForceEmulator *emulator);

// Options
void Emulate_SetGain(ForceEmulator *emulator,int gain,int gainMax);
void Emulate_SetEnable(ForceEmulator *emulator,bool enable);
bool Emulate_IsEnabled(ForceEmulator *emulator);
void Emulate_SetPaused(ForceEmulator *emulator,bool enable);
bool Emulate_IsPaused(ForceEmulator *emulator);

// Using effects
int Emulate_CreateEffect(ForceEmulator *emulator,const ForceParams *params);
bool Emulate_ChangeEffect(ForceEmulator *emulator,int index,const ForceParams *params);
void Emulate_DestroyEffect(ForceEmulator *emulator,int index);

bool Emulate_IsValidEffect(ForceEmulator *emulator,int index);
bool Emulate_IsPlaying(ForceEmulator *emulator,int index);

bool Emulate_IsEmpty(ForceEmulator *emulator);

void Emulate_Reset(ForceEmulator *emulator);

void Emulate_Start(ForceEmulator *emulator,int index,int iterations);
void Emulate_Stop(ForceEmulator *emulator,int index);
bool Emulate_IsStopped(ForceEmulator *emulator);

UInt32 Emulate_Effects_Used(ForceEmulator *emulator);
UInt32 Emulate_Effects_Total(ForceEmulator *emulator);

// Callbacks
void Emulate_Callback(ForceEmulator *emulator,unsigned char large,unsigned char small);
