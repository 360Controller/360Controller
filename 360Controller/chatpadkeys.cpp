/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 
 chatpadkeys.cpp - Converts a chatpad scancode to a USB key value
 
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

#include "chatpadkeys.h"

#define ROW_SIZE 8

typedef struct MAP_DATA {
	unsigned char row[ROW_SIZE];
} MAP_DATA;

static const MAP_DATA columns[] = {
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{0x00, 0x24, 0x23, 0x22, 0x21, 0x20, 0x1F, 0x1E},
{0x00, 0x18, 0x1C, 0x17, 0x15, 0x08, 0x1A, 0x14},
{0x00, 0x0D, 0x0B, 0x0A, 0x09, 0x07, 0x16, 0x04},
{0x00, 0x11, 0x05, 0x19, 0x06, 0x1B, 0x1D, 0x00},
{0x00, 0x4F, 0x10, 0x37, 0x2C, 0x50, 0x00, 0x00},
{0x00, 0x00, 0x36, 0x28, 0x13, 0x27, 0x26, 0x25},
{0x00, 0x2A, 0x0F, 0x00, 0x00, 0x12, 0x0C, 0x0E},
};

unsigned char ChatPad2USB(unsigned char input)
{
	unsigned char row = input & 0x0F;
	unsigned char column = (input & 0xF0) >> 4;
	
	if (row >= ROW_SIZE)
		return 0x00;
	if (column >= (sizeof(columns) / sizeof(columns[0])))
		return 0x00;
	return columns[column].row[row];
}
