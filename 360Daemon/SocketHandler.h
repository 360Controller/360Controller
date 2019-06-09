//
//  SocketHandler.h
//  360Daemon
//
//  Created by Drew Mills on 6/4/19.
//  Copyright © 2019 GitHub. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SocketHandler : NSObject <NSNetServiceDelegate, NSStreamDelegate>

@property (nonatomic, strong, readwrite) NSNetService* netService;
@property (nonatomic, strong, readwrite) NSInputStream* readStream;
@property (nonatomic, strong, readwrite) NSOutputStream* writeStream;

#pragma mark - NSNetServiceDelegate methods
- (void)netService:(NSNetService *)sender didAcceptConnectionWithInputStream:(NSInputStream *)inputStream outputStream:(NSOutputStream *)outputStream;

#pragma mark - NSStreamDelegate methods
- (void)stream:(NSStream *)stream handleEvent:(NSStreamEvent)eventCode;

#pragma mark - SocketHandler methods
- (BOOL)isReceiving;
- (void)startServer;
- (void)stopServer;

@end

NS_ASSUME_NONNULL_END
