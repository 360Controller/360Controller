//
//  BindingTableView.h
//  360 Driver
//
//  Created by Drew Mills on 1/30/15.
//

#import <Cocoa/Cocoa.h>

@interface BindingTableView : NSObject <NSTableViewDelegate, NSTableViewDataSource>

@property NSArray *buttonArr;

+ (NSTableView *)tableView;

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView;
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex;

@end
