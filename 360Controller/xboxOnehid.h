/*
 Copyright (c) 2014-2015 Drew Mills
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// Report that makes the device appear as an Xbox One controller
unsigned char ReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x20,                    //     REPORT_ID (32)
    
    // Report id and size
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x00,                    //     USAGE (Undefined)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    
    // Sync
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x0c,                    //     USAGE (Button 12)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    // Dummy (always zero)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    
    // Menu button
    0x09, 0x09,                    //     USAGE (Button 9)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    // View button
    0x09, 0x0a,                    //     USAGE (Button 10)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    // A, B, X & Y buttons
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x04,                    //     USAGE_MAXIMUM (Button 4)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    // D-Pad up, down, left & right
    0x19, 0x0c,                    //     USAGE_MINIMUM (Button 12)
    0x29, 0x0f,                    //     USAGE_MAXIMUM (Button 15)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    // Left & right bumpers
    0x19, 0x05,                    //     USAGE_MINIMUM (Button 5)
    0x29, 0x06,                    //     USAGE_MAXIMUM (Button 6)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    // Left & right stick buttons
    0x19, 0x07,                    //     USAGE_MINIMUM (Button 7)
    0x29, 0x08,                    //     USAGE_MAXIMUM (Button 8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    // Left & right triggers
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x32,                    //     USAGE (Z)
    0x09, 0x35,                    //     USAGE (Rz)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    // Left & right sticks (H & -V)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x33,                    //     USAGE (Rx)
    0x09, 0x34,                    //     USAGE (Ry)
    0x16, 0x00, 0x80,              //     LOGICAL_MINIMUM (-32768)
    0x26, 0xff, 0x7f,              //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x07,                    //     REPORT_ID (7)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x00,                    //     USAGE (Undefined)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    
    // Xbox button
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x0b,                    //     USAGE (Button 11)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x00,                    //     USAGE (Undefined)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

// Old trigger minimum and maximum.
// Holding onto it so it is easy to replace if we need it.
// Goes on lines 84 and 85 at time of writing.
//    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
//    0x26, 0xff, 0x03,              //     LOGICAL_MAXIMUM (1023)