//
//  SocketHandler.m
//  360Daemon
//
//  Created by Drew Mills on 6/4/19.
//  Copyright © 2019 GitHub. All rights reserved.
//

#import "SocketHandler.h"
#import "360DaemonSocketProtocol.h"
#import <libkern/OSReturn.h>
#import <IOKit/kext/KextManager.h>

#define WIRED_DRIVER CFSTR("com.mice.driver.Xbox360Controller")

@implementation SocketHandler

- (void)netService:(NSNetService *)sender didAcceptConnectionWithInputStream:(nonnull NSInputStream *)inputStream outputStream:(nonnull NSOutputStream *)outputStream
{
    if (sender != self.netService)
    {
        // Sender wasn't the expected value. Fail.
        return;
    }

    if (inputStream == nil || outputStream == nil)
    {
        // We need both streams. Fail.
        return;
    }

    [self.netService stop];
    self.readStream = inputStream;
    self.writeStream = outputStream;

    [self.readStream setDelegate:self];
    [self.readStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [self.readStream open];

    [self.writeStream setDelegate:self];
    [self.writeStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [self.writeStream open];
}

- (void)stream:(NSStream *)stream handleEvent:(NSStreamEvent)eventCode
{
    switch(eventCode)
    {
        case NSStreamEventOpenCompleted:
        {
            // Can confirm opens here.
        } break;

        case NSStreamEventHasSpaceAvailable:
        {
            // Queue up bytes here.
        } break;

        case NSStreamEventHasBytesAvailable:
        {
            if (stream == self.readStream)
            {
                DaemonProtocolCommand requestedCommand = None;
                UInt requestedCommandLength = 0;

                requestedCommandLength = (UInt)[self.readStream read:(UInt8*)&requestedCommand maxLength:1];

                if (requestedCommandLength > 0)
                {
                    switch(requestedCommand)
                    {
                        case None:
                        {

                        } break;

                        case LoadWired:
                        {
                            NSLog(@"Loading wired driver...");
                            OSReturn result = KextManagerLoadKextWithIdentifier(WIRED_DRIVER, nil);
                            NSLog(@"Result is: %d", result);
                            [self.writeStream write:(UInt8*)&result maxLength:sizeof(result)];
                        } break;

                        case UnloadWired:
                        {
                            NSLog(@"Unloading wired driver...");
                            OSReturn result = KextManagerUnloadKextWithIdentifier(WIRED_DRIVER);
                            NSLog(@"Result is: %d", result);
                            [self.writeStream write:(UInt8*)&result maxLength:sizeof(result)];
                        } break;
                    }
                }
            }
        }

        case NSStreamEventErrorOccurred:
        {

        } break;

        case NSStreamEventEndEncountered:
        {
            [self stopStream:self.readStream];
            [self stopStream:self.writeStream];
            [self startServer];
        } break;

        default:
        {
            // Rest of the cases go here.
        } break;
    }
}

- (BOOL)isReceiving
{
    return (([self.readStream streamStatus] == NSStreamStatusOpen) && ([self.writeStream streamStatus] == NSStreamStatusOpen));
}

- (void)startServer
{
    if (self.netService == nil)
    {
        self.netService = [[NSNetService alloc] initWithDomain:@"local." type:@"_360Daemon._tcp." name:@"360Daemon" port:0];
        [self.netService setDelegate:self];
    }
    [self.netService publishWithOptions:NSNetServiceListenForConnections];
}

- (void)stopServer
{
    if ([self.readStream streamStatus] == NSStreamStatusOpen)
    {
        [self stopStream:self.readStream];
    }

    if ([self.readStream streamStatus] == NSStreamStatusOpen)
    {
        [self stopStream:self.writeStream];
    }

    [self.netService stop];
}

- (void)stopStream:(NSStream*)stream
{
    [stream setDelegate:nil];
    [stream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [stream close];
}

@end
