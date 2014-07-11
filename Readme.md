**XBox 360 Controller driver for Mac OS X**

Copyright (C) 2006-2013 Colin Munro

[http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller](http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller)


**About**

This driver supports the Microsoft Xbox 360 controller, including access
to rumble motors and LEDs, on the Mac OS X platform. It includes a
plugin for the Apple Force Feedback Framework so some games will be able
to activate them, along with a Preference Pane with which allows you to
test everything is installed correctly. Both wired 360 controllers
connected via USB, and wireless 360 controllers connected via the
Wireless Gaming Receiver for Windows, are supported.


**Licence**

This driver is licensed under the GNU Public License. A copy of this
license is included in the distribution file, please inspect it before
using the binary or source.


**Installation**

Simply complete the installer package to install the driver. The driver
will recommend you restart - if you do not, the driver will only be
usable if the controller is already connected or connected within a
minute after the installer completes. If you are interested in
installing as a developer please see below.


**Usage**

The driver exposes a standard game pad with a number of standard
controls, so any game that supports gaming devices should work. In some
cases this may need an update from the manufacturer of the game or a
patched version. The Preference Pane uses the standard Mac OS X
Frameworks for accessing HID devices and accessing Force Feedback
capabilities, so should be a good test that the installation is
functional.


**Known Problems/Todo**

-   Driver probably needs to be more resilient to errors and odd cases
-   Extra settings? E.g. trigger deadzones, control remapping.
-   Someone has requested old Xbox Controller support too.

**Contact me**

-   Please feel free to contact me with any comments, questions and so
    on using the form at the URL at the top of the file.

**Developer info**

-   Anything below this probably doesn't affect end users, so you can
    stop reading now if you just want to use the driver.


***Building***

(This section does not yet discuss the source of the wireless drivers)

The distribution currently consists of 3 projects - one for the driver
(implemented in C++, as an I/O Kit C++ class), one for the force
feedback support plugin (implemented in C, as an I/O Kit COM plugin) and
one for the Preference Pane (implemented in Objective C as a preference
pane plugin). Ideally these 3 targets should be in the same project, but
I've not worked on this yet.


To build, use the standard Xcode build for Deployment on each of the 3
projects. Build Feedback360 before 360Controller, as the 360Controller
project includes a script to copy the Feedback360 bundle to the correct
place in the .kext to make it work.


To debug the driver, `sudo cp -R 360Controller.kext /tmp/` to assign the
correct properties - note that the Force Feedback plugin only seems to
be found by OSX if the driver is in `/System/Library/Extensions` so I
could only debug it in place.


To test the Preference Pane, just double-click the resulting file.


***Debugging***

Most of the debugging I did was via printing out text. In 360Controller,
you can use IOLog(), and the output will appear in system.log. In
Feedback360 normal `fprintf(stderr,...)`, and the output will appear on
the console of whatever application is attempting to use Force Feedback.
In Pref360Control, `NSLog()` works as it's an Objective C program, and will
output to the console of the Preferences application.


***Installer***

Included is an installer project for Packages. Download Packages at
[http://s.sudre.free.fr/Software/Packages/about.html](http://s.sudre.free.fr/Software/Packages/about.html)


***Other information***

I wrote the driver from scratch, using Apple documentation and drivers
simply as a reference and not basing it upon any existing source. As
such, some things may be done strangely or incorrectly, so excuse any
weirdness. I've also tried to include comments to explain generally
what's going on :)

The calculations for the updated Feedback360 plugin are based on the
unmaintained xi driver for Windows.
