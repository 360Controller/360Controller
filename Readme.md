# XBox 360 Controller driver for Mac OS X

## About ##

This driver supports the Microsoft Xbox 360 controller, including access to
rumble motors and LEDs, on the Mac OS X platform. It includes a plugin for the
Apple Force Feedback Framework so some games will be able to activate them,
along with a Preference Pane with which allows you to test everything is
installed correctly. Both wired 360 controllers connected via USB, and wireless
360 controllers connected via the Wireless Gaming Receiver for Windows, are
supported.

This project is a fork of the [Xbox360Controller project][1] originally created
by Colin Munro.

[1]: http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller


## Installation ##

See the [releases page][2] for the latest compiled and signed version of the
driver. Most users will want to install and run this.

If you are interested in installing as a developer please see below.

[2]: https://github.com/d235j/360Controller/releases


## Usage ##

The driver exposes a standard game pad with a number of standard controls, so
any game that supports gaming devices should work. In some cases this may need
an update from the manufacturer of the game or a patched version. The
Preference Pane uses the standard Mac OS X Frameworks for accessing HID devices
and accessing Force Feedback capabilities, so should be a good test that the
installation is functional.

[List of working and non-working games.](https://github.com/d235j/360Controller/wiki/Games)


## Developer info ##

Anything below this probably doesn't affect end users, so you can stop reading
now if you just want to use the driver.


### Building ###

You'll need the full xcode installed via the app store. The command line tools
are not enough.

From the command line, run: `./build.sh`

If you'd like to build the .pkg file, there is an installer project for
Packages. Download packages at
[http://s.sudre.free.fr/Software/Packages/about.html][3]
and the resulting dmg file will be copied to the build directory.

[3]: http://s.sudre.free.fr/Software/Packages/about.html

The distribution currently consists of 3 projects - one for the driver
(implemented in C++, as an I/O Kit C++ class), one for the force feedback
support plugin (implemented in C, as an I/O Kit COM plugin) and one for the
Preference Pane (implemented in Objective C as a preference pane plugin).
Ideally these 3 targets should be in the same project, but I've not worked on
this yet.

To build, use the standard Xcode build for Deployment on each of the 3
projects. Build Feedback360 before 360Controller, as the 360Controller project
includes a script to copy the Feedback360 bundle to the correct place in the
.kext to make it work.

To debug the driver, `sudo cp -R 360Controller.kext /tmp/` to assign the
correct properties - note that the Force Feedback plugin only seems to be found
by OSX if the driver is in `/System/Library/Extensions` so I could only debug
it in place.

To test the Preference Pane, just double-click the resulting file.


### Yosemite and signed drivers ###

Since Yosemite (Mac OS X 10.10) all global kexts are required to be signed.
This means if you want to build the drivers and install locally, you need to
have a mac developer account.

If you'd like to avoid paying apple for the developer account and want to
disable the signature checking, execute the following commands inside a
terminal:

``` bash
sudo nvram boot-args="kext-dev-mode=1"
sudo kextcache -m /System/Library/Caches/com.apple.kext.caches/Startup/Extensions.mkext /System/Library/Extensions
```

Note that this is probably a bad idea unless you understand the implications of
running unsigned driver code.


### Debugging ###

Most of the debugging I did was via printing out text. In 360Controller, you
can use IOLog(), and the output will appear in system.log. In Feedback360
normal `fprintf(stderr,...)`, and the output will appear on the console of
whatever application is attempting to use Force Feedback.  In Pref360Control,
`NSLog()` works as it's an Objective C program, and will output to the console
of the Preferences application.


## Licence ##

Copyright (C) 2006-2013 Colin Munro

This driver is licensed under the GNU Public License. A copy of this license is
included in the distribution file, please inspect it before using the binary or
source.
