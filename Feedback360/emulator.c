/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    emulator.c - simulate a force feedback device
    
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
#include "emulator.h"

/*
 * This code could do with being improved. For example, the two motors could
 * allow for a "left" and "right" rumble in a car game.
 *
 * For debugging, you may use fprintf(stderr,...). The output will appear on
 * the console of whatever game is using the Apple Force Feedback Framework.
 */

#define LoopGranularity             500         // Microseconds
#define LoopTimerDelay              (LoopGranularity*0.000001f) // In seconds

static void Emulate_Timer(CFRunLoopTimerRef timer,void *context);

// Initialise the emulation of a force feedback device
void Emulate_Initialise(ForceEmulator *emulator,int effectCount,void (*SetForce)(void*,unsigned char,unsigned char),void *context)
{
    int i;
    CFRunLoopTimerContext callbackData;
    
    // Set callback
    emulator->SetForce=SetForce;
    emulator->context=context;
    // Create effects memory
    emulator->effects=(ForceEffect*)malloc(sizeof(ForceEffect)*effectCount);
    emulator->effectCount=effectCount;
    // Defaults
    emulator->enable=TRUE;
    emulator->pause=FALSE;
    emulator->gain=4;
    emulator->maxGain=4;
    emulator->activeEffect=-1;
    // Reset
    for(i=0;i<effectCount;i++) emulator->effects[i].inUse=FALSE;
    // Launch timer
    callbackData.version=0;
    callbackData.info=emulator;
    callbackData.retain=NULL;
    callbackData.release=NULL;
    callbackData.copyDescription=NULL;
    emulator->timer=CFRunLoopTimerCreate(NULL,LoopTimerDelay,LoopTimerDelay,0,0,Emulate_Timer,&callbackData);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(),emulator->timer,kCFRunLoopCommonModes);
}

// Close down the force feedback stuff
void Emulate_Finalise(ForceEmulator *emulator)
{
    CFRunLoopTimerInvalidate(emulator->timer);
    CFRelease(emulator->timer);
    free(emulator->effects);
    emulator->SetForce(emulator->context,0,0);
}

// Call the callback, if output is enabled
void Emulate_Callback(ForceEmulator *emulator,unsigned char large,unsigned char small)
{
    emulator->oldLarge=large;
    emulator->oldSmall=small;
    if(emulator->enable) emulator->SetForce(emulator->context,large,small);
}

// Create an effect - for the FF API
int Emulate_CreateEffect(ForceEmulator *emulator,const ForceParams *params)
{
    int i;
    
    for(i=0;i<emulator->effectCount;i++) {
        if(!emulator->effects[i].inUse) {
            emulator->effects[i].inUse=TRUE;
            if(!Emulate_ChangeEffect(emulator,i+1,params)) {
                emulator->effects[i].inUse=FALSE;
                return 0;
            }
            return i+1;
        }
    }
    return 0;
}

// Change an effect - for the FF API
bool Emulate_ChangeEffect(ForceEmulator *emulator,int index,const ForceParams *params)
{
    ForceEffect *effect=emulator->effects+index-1;
    effect->gain=params->gain;
    effect->maxLevel=params->maxLevel;
    effect->startCount=params->startDelay/LoopGranularity;
    effect->attackLevel=params->attackLevel;
    effect->attackCount=params->attackTime/LoopGranularity;
    effect->sustainLevel=params->sustainLevel;
    effect->sustainCount=params->sustainTime/LoopGranularity;
    effect->fadeLevel=params->fadeLevel;
    effect->fadeCount=params->fadeTime/LoopGranularity;
    return TRUE;
}

// Destroy an effect - for the FF API
void Emulate_DestroyEffect(ForceEmulator *emulator,int index)
{
    emulator->effects[index-1].inUse=FALSE;
    if(emulator->activeEffect==(index-1)) emulator->activeEffect=-1;
}

// Returns true if the effect index is in use
bool Emulate_IsValidEffect(ForceEmulator *emulator,int index)
{
    if((index<1)||(index>emulator->effectCount)) return FALSE;
    return emulator->effects[index-1].inUse;
}

// Returns true if the effect index is playing
bool Emulate_IsPlaying(ForceEmulator *emulator,int index)
{
    return emulator->activeEffect==(index-1);
}

// Returns true if the device has no effects
bool Emulate_IsEmpty(ForceEmulator *emulator)
{
    int i;
    
    for(i=0;i<emulator->effectCount;i++)
        if(emulator->effects[i].inUse)
            return FALSE;
    return TRUE;
}

// Resets the device
void Emulate_Reset(ForceEmulator *emulator)
{
    int i;
    
    for(i=0;i<emulator->effectCount;i++) emulator->effects[i].inUse=FALSE;
    Emulate_Callback(emulator,0,0);
}

// Starts an effect for the specified number of loops
void Emulate_Start(ForceEmulator *emulator,int index,int iterations)
{
    ForceEffect *effect;
    
//    fprintf(stderr,"Emulate_Start(%p,%i,%i)\n",emulator,index,iterations);
    effect=emulator->effects+index-1;
    effect->countStart=effect->startCount;
    effect->countAttack=effect->attackCount;
    effect->countSustain=effect->sustainCount;
    effect->countFade=effect->fadeCount;
    effect->countLoop=iterations;
    emulator->activeEffect=index-1;
}

// Stops an effect
void Emulate_Stop(ForceEmulator *emulator,int index)
{
//    fprintf(stderr,"Emulate_Stop(%p,%i)\n",emulator,index);
    if((index-1)==emulator->activeEffect) {
        emulator->activeEffect=-1;
        Emulate_Callback(emulator,0,0);
    }
}

// Returns true if no effects are running
bool Emulate_IsStopped(ForceEmulator *emulator)
{
    return emulator->activeEffect==-1;
}

// Returns the number of used effects
UInt32 Emulate_Effects_Used(ForceEmulator *emulator)
{
    int i,j;
    
    for(i=j=0;i<emulator->effectCount;i++)
        if(emulator->effects[i].inUse)
            j++;
    return j;
}

// Returns the number of possible effects
UInt32 Emulate_Effects_Total(ForceEmulator *emulator)
{
    return emulator->effectCount;
}

// Enables or disables the force feedback
void Emulate_SetEnable(ForceEmulator *emulator,bool enable)
{
    bool old=emulator->enable;
    // Stop motors in case they were running
    if(old&&(!enable)) Emulate_Callback(emulator,0,0);
    emulator->enable=enable;
    // Resume motors at last computed value, as we'll have
    // been computing new values for them even if disabled
    if((!old)&&enable) Emulate_Callback(emulator,emulator->oldLarge,emulator->oldSmall);
}

// Returns true if enabled
bool Emulate_IsEnabled(ForceEmulator *emulator)
{
    return emulator->enable;
}

// Pauses the device
void Emulate_SetPaused(ForceEmulator *emulator,bool enable)
{
    bool old=emulator->pause;
    // Stop motors in case they were running
    if((!old)&&enable) Emulate_Callback(emulator,0,0);
    emulator->pause=enable;
    // Resume motors at last computed value, as we'll be
    // resuming feedback effects where we left off.
    if(old&&(!enable)) Emulate_Callback(emulator,emulator->oldLarge,emulator->oldSmall);
}

// Returns true if paused
bool Emulate_IsPaused(ForceEmulator *emulator)
{
    return emulator->pause;
}

// Sets the gain for the device
void Emulate_SetGain(ForceEmulator *emulator,int gain,int gainMax)
{
    if((emulator->gain<=0)||(emulator->maxGain<=0)) return;
    emulator->gain=gain;
    emulator->maxGain=gainMax;
    while(emulator->maxGain<4) {
        emulator->gain*=2;
        emulator->maxGain*=2;
    }
}

// Utility - scales a value
static inline int Emulate_Scale(int from,int to,int max,int val)
{
    int upper=max-1;
    return from+(((to-from)*(upper-val))/upper);
}

// Utility - computes the values for the large and small motors from the effect
static void Emulate_Force(ForceEmulator *emulator,int force,int maxGain)
{
    int adjusted;
    int bling;
    int small,large;
    
//    fprintf(stderr,"Emulate_Force(%p,%i)\n",emulator,force);
    adjusted=(force*emulator->gain)/emulator->maxGain;
    bling=maxGain/4;
    if(adjusted<bling) {
        // Only activate small motor
        large=0;
        small=(adjusted*0xff)/bling;
    } else {
        // Activate large motor
        large=((adjusted-bling)*0xff)/(bling*3);
        if(adjusted<(bling*3)) small=0;
        else {
            // Activate small motor too
            small=((adjusted-(bling*3))*0xff)/bling;
        }
    }
//    fprintf(stderr,"Computed %i,%i from adjusted=%i, bling=%i\n",large,small,adjusted,bling);
    Emulate_Callback(emulator,large,small);
}

// Handles the timer, to simulate an effect using the rumble motors
static void Emulate_Timer(CFRunLoopTimerRef timer,void *context)
{
    ForceEmulator *emulator;
    int i;
    ForceEffect *effect;
    int level;
    
    // Find effect
    emulator=(ForceEmulator*)context;
    i=emulator->activeEffect;
    if((i==-1)||emulator->pause) return;
    effect=emulator->effects+emulator->activeEffect;
    // Process each counter on the effect, to see what's happening
    if(effect->countStart>0) {
        // In the start (pre-effect) stage
        level=effect->attackLevel;
        effect->countStart--;
    } else {
        if(effect->countAttack>0) {
            // In the attack stage
            level=Emulate_Scale(effect->attackLevel,effect->sustainLevel,effect->attackCount,effect->countAttack);
            effect->countAttack--;
        } else {
            if(effect->countSustain>0) {
                // In the sustain stage
                level=effect->sustainLevel;
                effect->countSustain--;
            } else {
                if(effect->countFade>0) {
                    // In the fade stage
                    level=Emulate_Scale(effect->sustainLevel,effect->fadeLevel,effect->fadeCount,effect->countFade);
                    effect->countFade--;
                } else {
                    // Work out if we need to do it again
                    effect->countLoop--;
                    if(effect->countLoop>0) {
                        // Prepare for next loop
                        level=effect->attackLevel;
                        effect->countStart=effect->startCount;
                        effect->countAttack=effect->attackCount;
                        effect->countSustain=effect->sustainCount;
                        effect->countFade=effect->fadeCount;
                    } else {
                        // Effect finished
                        level=0;
                        emulator->activeEffect=-1;
                    }
                }
            }
        }
    }
    // Actually calculate what to do for the effect
    Emulate_Force(emulator,(level*effect->gain)/effect->maxLevel,effect->maxLevel);
}
