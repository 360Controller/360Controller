//
//  MyWhole360ControllerMapper.m
//  360 Driver
//
//  Created by Drew Mills on 1/30/15.
//

#import "MyWhole360Controller.h"
#import "MyWhole360ControllerMapper.h"
#import "Pref360StyleKit.h"
#import "BindingTableView.h"

@implementation MyWhole360ControllerMapper
{
    int currentMappingIndex;
    NSButton *remappingButton;
    Pref360ControlPref *pref;
}

static UInt8 mapping[15];

+ (UInt8 *)mapping
{
    return mapping;
}

- (void)runMapperWithButton:(NSButton *)button andOwner:(Pref360ControlPref *)prefPref {
    pref = prefPref;
    remappingButton = button;
//    [self resetMapping];
//    [pref changeSetting:nil];
    currentMappingIndex = 0;
    _isMapping = YES;
    [self setUpPressed:YES];
}

- (int)realignButtonToByte:(int)index {
    if (index == 0) return 12;
    if (index == 1) return 13;
    if (index == 2) return 14;
    if (index == 3) return 15;
    if (index == 4) return 8;
    if (index == 5) return 9;
    if (index == 6) return 6;
    if (index == 7) return 7;
    if (index == 8) return 4;
    if (index == 9) return 5;
    if (index == 10) return 10;
    if (index == 11) return 0;
    if (index == 12) return 1;
    if (index == 13) return 2;
    if (index == 14) return 3;
    return -1; // Should Never Happen
}

- (void)buttonPressedAtIndex:(int)index {
    mapping[currentMappingIndex] = [self realignButtonToByte:index];
    currentMappingIndex++;
    [self setButtonAtIndex:currentMappingIndex];
    if (currentMappingIndex == 15) {
        _isMapping = NO;
        currentMappingIndex = 0;
        [remappingButton setState:NSOffState];
        [[BindingTableView tableView] reloadData];
        [pref changeSetting:nil];
    }
}

- (void)setButtonAtIndex:(int)index {
    switch (index) {
        case 1:
            [self setUpPressed:NO];
            [self setDownPressed:YES];
            break;
            
        case 2:
            [self setDownPressed:NO];
            [self setLeftPressed:YES];
            break;
            
        case 3:
            [self setLeftPressed:NO];
            [self setRightPressed:YES];
            break;
            
        case 4:
            [self setRightPressed:NO];
            [self setStartPressed:YES];
            break;
            
        case 5:
            [self setStartPressed:NO];
            [self setBackPressed:YES];
            break;
            
        case 6:
            [self setBackPressed:NO];
            [self setLeftStickPressed:YES];
            break;
            
        case 7:
            [self setLeftStickPressed:NO];
            [self setRightStickPressed:YES];
            break;
            
        case 8:
            [self setRightStickPressed:NO];
            [self setLbPressed:YES];
            break;
            
        case 9:
            [self setLbPressed:NO];
            [self setRbPressed:YES];
            break;
            
        case 10:
            [self setRbPressed:NO];
            [self setHomePressed:YES];
            break;
            
        case 11:
            [self setHomePressed:NO];
            [self setAPressed:YES];
            break;
            
        case 12:
            [self setAPressed:NO];
            [self setBPressed:YES];
            break;
            
        case 13:
            [self setBPressed:NO];
            [self setXPressed:YES];
            break;
            
        case 14:
            [self setXPressed:NO];
            [self setYPressed:YES];
            break;
            
        case 15:
            [self setYPressed:NO];
            break;
            
        default:
            break;
    }
}

- (void)resetMapping {
    for (int i = 0; i < 15; i++) {
        mapping[i] = i;
    }
    for (int i = 12; i < 16; i++) {
        mapping[i-1] = i;
    }
}

- (void)reset {
    [super reset];
    _isMapping = NO;
    [self resetMapping];
    [remappingButton setState:NSOffState];
    [pref changeSetting:nil];
    [[BindingTableView tableView] reloadData];
//    [pref changeSetting:nil];
}

- (void)awakeFromNib {
    [super awakeFromNib];
    _isMapping = NO;
    [self resetMapping];
    [[BindingTableView tableView] reloadData];
}

@end