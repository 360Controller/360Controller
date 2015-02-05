//
//  BindingTableView.m
//  360 Driver
//
//  Created by Drew Mills on 1/30/15.
//

#import "BindingTableView.h"
#import "MyWhole360ControllerMapper.h"

@implementation BindingTableView

static NSTableView *tblView;

+ (NSTableView *)tableView {
    return tblView;
}

- (void)awakeFromNib {
    _buttonArr = @[@"Up", @"Down", @"Left", @"Right", @"Start", @"Back", @"LS Click", @"RS Click", @"LB", @"RB", @"Guide", @"A", @"B", @"X", @"Y"];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    tblView = tableView;
    return [_buttonArr count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    if ([[aTableColumn identifier] isEqualToString:@"input"]) {
        return [NSNumber numberWithInt:[MyWhole360ControllerMapper mapping][rowIndex]];
    }
    else
        return _buttonArr[rowIndex];
}

@end
