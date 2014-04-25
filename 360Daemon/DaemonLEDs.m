//
//  DaemonLEDs.m
//  360 Driver
//
//  Created by C.W. Betts on 4/25/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#import "DaemonLEDs.h"

@interface DaemonLEDs ()
@property (copy) NSString *theLED0;
@property (copy) NSString *theLED1;
@property (copy) NSString *theLED2;
@property (copy) NSString *theLED3;
@end

@implementation DaemonLEDs

- (void)setLED:(int)theLED toSerialNumber:(NSString*)serialNum
{
	switch (theLED) {
		case 0:
			self.theLED0 = serialNum;
			break;
			
		case 1:
			self.theLED1 = serialNum;
			break;
			
		case 2:
			self.theLED2 = serialNum;
			break;
			
		case 3:
			self.theLED3 = serialNum;
			break;

		default:
			break;
	}
}

- (NSString *)serialNumberAtLED:(int)theLED
{
	switch (theLED) {
		case 0:
			return self.theLED0;
			break;
			
		case 1:
			return self.theLED1;
			break;
			
		case 2:
			return self.theLED2;
			break;
			
		case 3:
			return self.theLED3;
			break;
			
		default:
			return @"";
			break;
	}
}

- (BOOL)serialNumberAtLEDIsBlank:(int)theLED
{
	switch (theLED) {
		case 0:
			return self.theLED0 == nil;
			break;
			
		case 1:
			return self.theLED1 == nil;
			break;
			
		case 2:
			return self.theLED2 == nil;
			break;
			
		case 3:
			return self.theLED3 == nil;
			break;
			
		default:
			return NO;
			break;
	}
}

- (void)clearSerialNumberAtLED:(int)theLED
{
	switch (theLED) {
		case 0:
			self.theLED0 = nil;
			break;
			
		case 1:
			self.theLED1 = nil;
			break;
			
		case 2:
			self.theLED2 = nil;
			break;
			
		case 3:
			self.theLED3 = nil;
			break;
			
		default:
			break;
	}
}

@end
