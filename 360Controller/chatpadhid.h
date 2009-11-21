// F:\Documents and Settings\Desktop\hid\ChatPad_Keyboard.h


char ReportDescriptor[58] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x25, 0x00,                    //   LOGICAL_MAXIMUM (0)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x09, 0xe1,                    //   USAGE (Keyboard LeftShift)
    0x09, 0xe0,                    //   USAGE (Keyboard LeftControl)
    0x09, 0xe2,                    //   USAGE (Keyboard LeftAlt)
    0x09, 0xe3,                    //   USAGE (Keyboard Left GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x04,                    //   REPORT_COUNT (4)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x04,                    //   REPORT_COUNT (4)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xe7, 0x00,              //   LOGICAL_MAXIMUM (231)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

