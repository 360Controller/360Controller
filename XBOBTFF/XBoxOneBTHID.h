//
//  XBoxOneBTHID.h
//  XBoxBTFF
//
//  Created by C.W. Betts on 9/20/17.
//  Copyright © 2017 C.W. Betts. All rights reserved.
//

#ifndef XBoxOneBTHID_h
#define XBoxOneBTHID_h

#include <stdint.h>
#include <CoreFoundation/CFBase.h>

#pragma pack(push, 1)

typedef CF_OPTIONS(uint8_t, XBOBTActivationMask) {
	XBOBTActivationMaskRightRumble	= 1 << 0,
	XBOBTActivationMaskLeftRumble	= 1 << 1,
	XBOBTActivationMaskRTRumble		= 1 << 2,
	XBOBTActivationMaskLTRumble		= 1 << 3,
};

struct XboxOneBluetoothReport_t {
	uint8_t				reportID;		//!< 0x03
	XBOBTActivationMask	activationMask;
	uint8_t				ltMagnitude;	//!< 0x00 to 0x65, 0x65 is default
	uint8_t				rtMagnitude;
	uint8_t				leftMagnitude;
	uint8_t				rightMagnitude;
	uint8_t				duration;		//!< time in ms, 255 = 2.55s
	uint8_t				startDelay;
	uint8_t				loopCount;
};

#pragma pack(pop)

#endif /* XBoxOneBTHID_h */
