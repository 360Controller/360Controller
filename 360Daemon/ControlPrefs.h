//
//  ControlPrefs.h
//  360Daemon
//
//  Created by Colin Munro on 8/11/07.
//  Copyright 2007 Colin Munro. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#define DOM_DAEMON      CFSTR("com.mice.driver.Xbox360Controller.daemon")
#define DOM_CONTROLLERS CFSTR("com.mice.driver.Xbox360Controller.devices")

#define D_SHOWONCE      @"ShowAlert"
#define D_KNOWNDEV      @"KnownDevices"

// Daemon's own settings
void SetAlertDisabled(int index);
BOOL AlertDisabled(int index);

// Controller settings
void SetController(NSString *serial, NSDictionary *data);
NSDictionary* GetController(NSString *serial);

// Configuration settings
void SetKnownDevices(NSDictionary *devices);
NSDictionary* GetKnownDevices(void);

// Utility functions
NSString* GetSerialNumber(io_service_t device);
void ConfigController(io_service_t device, NSDictionary *config);
