/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 
 ControlPrefs.h - interface to the preferences functionality
 
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
#import <Cocoa/Cocoa.h>

#define DOM_DAEMON      CFSTR("com.mice.driver.Xbox360Controller.daemon")
#define DOM_CONTROLLERS CFSTR("com.mice.driver.Xbox360Controller.devices")

#define D_SHOWONCE      @"ShowAlert"
#define D_KNOWNDEV      @"KnownDevices"

// Daemon's own settings
void SetAlertDisabled(NSInteger index);
BOOL AlertDisabled(NSInteger index);

// Controller settings
void SetController(NSString *serial, NSDictionary *data);
NSDictionary* GetController(NSString *serial);

// Configuration settings
void SetKnownDevices(NSDictionary *devices);
NSDictionary* GetKnownDevices();

// Utility functions
NSString* GetSerialNumber(io_service_t device);
void ConfigController(io_service_t device, NSDictionary *config);
