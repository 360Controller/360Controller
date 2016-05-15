# Xbox Controller Driver for Mac OS X

## About
This driver supports the Microsoft Xbox series of controllers including those for the original Xbox, Xbox 360, and Xbox One. Xbox 360 controllers work both wired and wirelessly, while Xbox One controllers only work wired for now. The driver provides developers with access to both force feedback and the LEDs of the controllers. Additionally, a preference pane has been provided so that users can configure their controllers and ensure that the driver has been installed properly.

Controller support includes ALL devices that work with an Xbox series piece of hardware. All wheels, fight sticks, and controllers should work. This includes things like the Xbox One Elite controller. If your hardware does not work with an Xbox console we cannot support it. Sorry.

This project is a fork of the [Xbox360Controller project](http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller) originally created by Colin Munro.

## Installation
See the [releases page](https://github.com/360Controller/360Controller/releases) for the latest compiled and signed version of the driver. Most users will want to run this installer.

## Uninstallation
In order to uninstall the driver: navigate to the preference pane by opening your "System Preferences," navigating to the "Xbox 360 Controllers" pane, clicking on the "Advanced" tab and pressing the "Uninstall" button. This will prompt you to enter your password so that the uninstaller can remove all of the bundled software from your machine.

## Usage
The driver exposes a standard game pad with a number of standard controls, so any game that supports gaming devices should work. In some cases, this may require an update from the developer of the game. The preference pane uses the standard Mac OS X frameworks for accessing HID devices in addition to access of Force Feedback capabilities. This means that the preference pane is a good indicator that the driver is functional for other programs.

It is important to note that this driver does not work, and can never work, with Apple's "Game Controller Framework." This GCController framework corresponds to physical gamepads that have been offically reviewed by Apple and accepted into the mFi program. Due to the fact that we are not Microsoft, we cannot get their gamepad certified to be a GCController. This is an unfortunate oversight on Apple's part. If you would like to discuss this, please do so at [this location.](https://github.com/360Controller/360Controller/issues/55)

Users have been maintaining a [partial list of working and non-working games.](https://github.com/360Controller/360Controller/wiki/Games) Please contribute your findings so that you can help others debug their controller issues.

## My controller doesn't work!

### I'm using a driver from the Tattiebogle website
The Tattiebogle driver is NOT the same driver as this Github project. We do NOT support that driver. Under NO circumstances will we support that driver. If you download the latest version of this driver from the [releases page](https://github.com/360Controller/360Controller/releases) we will do our best to help you out. This driver will install over the Tattiebogle driver. You don't have to worry about uninstalling the Tattiebogle driver first.

### Original Xbox Controllers
Make an issue describing your problem.

### Wired Xbox 360 Controllers
If you have a third party controller, make an issue with the "Product ID" and "Vendor ID" of the controller. These can be found by accessing the Apple menu, selecting "About this Mac", and then selecting "System Report..." on the "Overview" tab. On the left hand side of the new window, select "Hardware". If the controller is plugged in, there should be an entry in this window called "Controller".

### Wireless Xbox 360 Controllers
Remember that wireless controllers must be connected using a wireless adapter. Plugging a "Play and Charge" kit into a wireless controller does not make it a wired controller.

### Wired Xbox One Controllers
If you have a third party controller, make an issue with the "Product ID" and "Vendor ID" of the controller. These can be found by accessing the Apple menu, selecting "About this Mac", and then selecting "System Report..." on the "Overview" tab. On the left hand side of the new window, select "Hardware". If the controller is plugged in, there should be an entry in this window called "Controller".

### Wireless Xbox One Controllers
Wireless Xbox One controllers are currently not supported. Please be patient as we figure out this complicated protocol.

### My Xbox One controller doesn't work with a game!
It is entirely possible that the game doesn't work with controllers, but to get best compatibility from your Xbox One controllers you need to change a setting. First open up the preference pane, navigate to the "Advanced" tab, and check "Pretend to be an Xbox 360 Controller."

## Developer Info
Drivers inherently modify the core operating system kernel. Using the driver as a developer can lead to dangerous kernel panics that can cause data loss or other permanent damage to your computer. Be very careful about how you use this information. We are not responsible for anything this driver does to your computer, or any loss it may incur. Normal users will never have to worry about the developer section of this README.

### Building

##### Apple has recently changed how drivers work in Xcode 7. In order to build the driver, you will need Xcode 6.4 or earlier.
Additionally, to use the included build scripts, you will need to change your preferred Xcode installation using `xcode-select`.

##### You must have a signing certificate to install a locally built driver. Alternatively, you can disable driver signing on your machine, however this is a major security hole and the decision should not be taken lightly.

You will need a full installation of Xcode to build this project. The command line tools are not enough.

The project consists of three main parts: The driver (implemented in C++, as an I/O Kit C++ class), the force feedback plugin (implemented in C, as an I/O Kit COM plugin) and the preference pane (implemented in Objective C as a preference pane plugin). To build, use the standard Xcode build for Deployment on each of the 3 projects. Build Feedback360 before 360Controller, as the 360Controller project includes a script to copy the Feedback360 bundle to the correct place in the .kext to make it work.

To debug the driver, sudo cp -R 360Controller.kext /tmp/ to assign the correct properties - note that the Force Feedback plugin only seems to be found by OSX if the driver is in /System/Library/Extensions so it can only be debugged in place. Due to the fact that drivers are now stored in /Library/Extenions, this means that you must create a symlink between the location of the driver and /System/Library/Extensions so that the force feedback plugin can operate properly.

### Building the .pkg

In order to build the .pkg, you will need to install [Packages.app](http://s.sudre.free.fr/Software/Packages/about.html).

#### If you don't have a signing certificate

* Open `360 Driver.xcodeproj` using Xcode.
* Select the `360 Driver` project in the Navigator.
* Select the `360Daemon` target from the top right corner.
* Select the `Build Settings` tab from the top of the screen.
* In the `Code Signing` section, find `Code Signing Identity` section and expand it.
* In the `Release` section, change the selection to `Don't Code Sign`.
* Run `./build.sh` to build the .pkg. This .pkg can be found in the `Install360Controller` directory.

#### If you have a signing certificate

* Open `360 Driver.xcodeproj` using Xcode.
* Select the `360 Driver` project in the Navigator.
* Select the `360Daemon` target from the top right corner.
* Select the `Build Settings` tab from the top of the screen.
* In the `Code Signing` section, find `Code Signing Identity` section and expand it.
* In the `Release` section, change the selection to your `Developer ID Application` certificate.
* Run `./build.sh` to build the .pkg. This .pkg can be found in the `Install360Controller` directory.

### Disabling signing requirements

Since Yosemite (Mac OS X 10.10) all global kexts are required to be signed. This means if you want to build the drivers and install locally, you need a very specific signing certificate that Apple closely controls. If you want to disable the signing requirement from OS X, you will need to do several things.

First, execute these commands in your terminal:
```
sudo nvram boot-args="kext-dev-mode=1"
sudo kextcache -m /System/Library/Caches/com.apple.kext.caches/Startup/Extensions.mkext /System/Library/Extensions
```

Next, you must disable System Integrity Protection. To do this, boot into recovery mode by holding down `CMD + R` while the computer is starting. Once recovery mode has been loaded, open the terminal from the `Utilites` menu item. Execute the following command:
```
csrutil disable
```

### Re-Enabling signing requirements

From recovery mode, execute the following command:
```
csrutil enable
```

Reboot into OS X like normal. You can reset the boot arguments by executing this command:
```
sudo nvram -d boot-args
```
This will remove ALL boot-args. If you have previously manipulated your boot-args, those changes will be erased as well!

### Debugging the driver

Debugging the driver depends on which part you intend to debug. For the 360Controller driver itself, it uses `IOLog` to output to the `system.log` which can be accessed using Console.app. Feedback360 uses `fprintf(stderr, ...)`, which should appear within the console of the program attempting to use force feedback.

### Debugging the preference pane

These instructions work for Xcode 6.4, the most recent version of Xcode that can still build the driver. Most of these instructions are pulled directly from [this blog post.](http://www.condition-alpha.com/blog/?p=1314) Please visit it for futher information.

First, edit your build scheme for Pref360Control, and select the "Run" scheme, and make sure you are editing "Debug" (A). In the environment variables section, click on "+" to add a new environment variable (B). Name the new variable OBJC_DISABLE_GC, and set its value to YES.

Next, click the little disclosure triangle for the run scheme to reveal its detailed settings. Then select pre-actions. Click the "+" at the bottom to add a run script action. Enter /bin/sh as the shell, make sure that your target is selected to provide build settings, and type a shell command line to install the newly compiled pref pane in your personal Library folder:

```cp -Rf ${CONFIGURATION_BUILD_DIR}/Pref360Control.prefPane ~/Library/PreferencePanes```

Finally, select the run step, choose "other" from the executable drop-down menu, and select System Preferences in the Applications folder. Verify that "Debug executable" and "Automatically" are both checked.

## Licence

Copyright (C) 2006-2013 Colin Munro

This driver is licensed under the GNU Public License. A copy of this license is included in the distribution file, please inspect it before using the binary or source.