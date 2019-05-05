//
//  MyWhole360Controller.h
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import <Cocoa/Cocoa.h>

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
#define OnState NSControlStateValueOn
#define OffState NSControlStateValueOff
#else
#define OnState NSOnState
#define OffState NSOffState
#endif

@interface MyWhole360Controller : NSView

@property (nonatomic) BOOL aPressed;
@property (nonatomic) BOOL bPressed;
@property (nonatomic) BOOL xPressed;
@property (nonatomic) BOOL yPressed;

@property (nonatomic) BOOL leftPressed;
@property (nonatomic) BOOL upPressed;
@property (nonatomic) BOOL rightPressed;
@property (nonatomic) BOOL downPressed;

@property (nonatomic) BOOL backPressed;
@property (nonatomic) BOOL startPressed;
@property (nonatomic) BOOL homePressed;

@property (nonatomic) BOOL lbPressed;
@property (nonatomic) BOOL rbPressed;
@property (nonatomic) BOOL leftStickPressed;
@property (nonatomic) BOOL rightStickPressed;

@property (nonatomic) CGPoint leftStickPosition;
@property (nonatomic) CGPoint rightStickPosition;
@property (nonatomic) CGFloat leftStickDeadzone;
@property (nonatomic) CGFloat rightStickDeadzone;
@property (nonatomic) BOOL leftNormalized;
@property (nonatomic) BOOL rightNormalized;

@property (nonatomic) int leftStickXPos;
@property (nonatomic) int leftStickYPos;
@property (nonatomic) int rightStickXPos;
@property (nonatomic) int rightStickYPos;

- (void)reset;

@end
