//
//  DaemonLEDs.h
//  360 Driver
//
//  Created by C.W. Betts on 4/25/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface DaemonLEDs : NSObject
- (void)setLED:(int)theLED toSerialNumber:(NSString*)serialNum;
- (NSString *)serialNumberAtLED:(int)theLED;
- (BOOL)serialNumberAtLEDIsBlank:(int)theLED;
- (void)clearSerialNumberAtLED:(int)theLED;
#if 0
- (void)setObject:(id)obj atIndexedSubscript:(NSUInteger)idx;
- (id)objectAtIndexedSubscript:(NSUInteger)idx;
#endif
@end
