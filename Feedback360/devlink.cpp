/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    devlink.c - code to speak to the driver itself
    
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

#include <IOKit/IOCFPlugIn.h>
#include "devlink.h"

// Initialise the link
bool Device_Initialise(DeviceLink *link,io_object_t device)
{
    IOCFPlugInInterface **plugInInterface = NULL;
    SInt32 score = 0;
    IOReturn ret = IOCreatePlugInInterfaceForService(device, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
    
    if (ret!=kIOReturnSuccess) return false;
    ret=(*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID121), (LPVOID*)(&link->interface));
    (*plugInInterface)->Release(plugInInterface);
    if (ret!=kIOReturnSuccess) return false;
    (*link->interface)->open(link->interface, 0);
    return true;
}

// Finish the link
void Device_Finalise(DeviceLink *link)
{
    (*link->interface)->close(link->interface);
    (*link->interface)->Release(link->interface);
    link->interface = NULL;
}

// Send a report via the link
bool Device_Send(DeviceLink *link,void *data,int length)
{
    if(link->interface==NULL) {
        fprintf(stderr, "Attempting to send to a closed link!\n");
        return false;
    }
    else {
        //fprintf(stderr, "Attempting to send: %d %d %d %d\n",((unsigned char*)data)[0], ((unsigned char*)data)[1], ((unsigned char*)data)[2], ((unsigned char*)data)[3]);
        IOReturn res=(*link->interface)->setReport(link->interface,kIOHIDReportTypeOutput,0,data,length,10000,NULL,NULL,NULL);
        if (res != kIOReturnSuccess)
            fprintf(stderr, "Device_Send failed: 0x%.8x\n", res);
        return res == kIOReturnSuccess;
    }
}
