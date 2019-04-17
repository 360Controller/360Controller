# Xbox Controller Driver for macOS

## Table of Contents
1. [About](#about)
2. [Installation](#installation)
3. [Uninstallation](#uninstallation)
4. [Usage](#usage)
5. [My controller doesn't work!](#my-controller-doesnt-work)
   1. [I'm using a driver from the Tattiebogle website](#im-using-a-driver-from-the-tattiebogle-website)
   2. [My controller doesn't work with a game!](#my-controller-doesnt-work-with-a-game)
   3. [How do I find my Vendor ID and Product ID?](#how-do-i-find-my-vendor-id-and-product-id)
   4. [Original Xbox Controllers](#original-xbox-controllers)
   5. [Wired Xbox 360 Controllers](#wired-xbox-360-controllers)
   6. [Wireless Xbox 360 Controllers](#wireless-xbox-360-controllers)
   7. [Xbox One Controllers connected with USB](#xbox-one-controllers-connected-with-usb)
   8. [Xbox One Controllers connected with Wireless Adapter](#xbox-one-controllers-connected-with-wireless-adapter)
   9. [Xbox One Controllers connected with Bluetooth](#xbox-one-controllers-connected-with-bluetooth)
   10. [Xbox One Adaptive Controller](#xbox-one-adaptive-controller)
6. [Adding Third Party Controllers](#adding-third-party-controllers)
7. [Developer Info](#developer-info)
   1. [Building](#building)
   2. [Building the .pkg](#building-the-pkg)
   3. [Disabling signing requirements](#disabling-signing-requirements)
   4. [Re-Enabling signing requirements](#re-enabling-signing-requirements)
   5. [Debugging the driver](#debugging-the-driver)
   6. [Debugging the preference pane](#debugging-the-preference-pane)
   7. [A note on Unity mappings](#a-note-on-unity-mappings)
8. [Licence](#licence)

## About
This driver supports the Microsoft Xbox series of controllers including:

1. Original Xbox
    - Original Xbox controllers are supported by using a USB adapter.

2. Xbox 360
    - Wired Xbox 360 controllers are supported directly.
    - **As of macOS 10.11, Wireless Xbox 360 controller support causes kernel panics. This issue cannot be resolved with minor changes to the driver, and requires that the driver be re-written from scratch to resolve the issue. Due to an excess of caution, we have disabled Wireless Xbox 360 controller support as of 0.16.6. If you want to use a wireless controller, download 0.16.5 or earlier and disable the driver before the computer enters a "sleep" state in order to prevent kernel panics. Alternatively, you can revert to a macOS version before 10.11 to avoid this issue.**

3. Xbox One
    - Xbox One controllers are supported when connected with a micro USB cable. Using the controller with the Wireless Adapter is not currently supported.
    - Bluetooth capable Xbox One controllers (released after August 2016) are natively supported by macOS without the use of this driver. However, installing this driver will allow you to use the controller via USB.

The driver provides developers with access to both force feedback and the LEDs of the controllers. Additionally, a preference pane has been provided so that users can configure their controllers and ensure that the driver has been installed properly.

Controller support includes ALL devices that work with an Xbox series piece of hardware. All wheels, fight sticks, and controllers should work. This includes things like the Xbox One Elite controller. If your hardware does not work with an Xbox console we cannot support it. Sorry.

This project is a fork of the [Xbox360Controller project](http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller) originally created by Colin Munro.

## Installation
See the [releases page](https://github.com/360Controller/360Controller/releases) for the latest compiled and signed version of the driver. Most users will want to run this installer. If you are using macOS 10.13.4 or later, then you will have to allow the signing certificate of "Drew Mills" in order for the software to run. Usually, the installer will prompt you to complete this process:
![System prompt: System Extension Blocked](https://imgur.com/zXM5JlU.png)
You can either click "Open Security Preferences" to quickly fix this. If you didn't see this prompt, you can navigate to the same window using the Apple menu in the top left hand corner of your screen, navigating the "System Preferences" and then clicking on "Security & Privacy." This will open up the following page. All you need to do is click the "Allow" button near the bottom right.
![Security & Privacy Preference Pane displaying prompt to user: System software from user "Drew Mills" was blocked from loading](https://imgur.com/HrL77Ii.png)
This prompt has been known to have issues with software or hardware that interferes with mouse movement. If you are using software that impacts the movement of your mouse, such as MagicKeys, or are using a special interface device, such as a Wacom tablet, please using a standard input device, such as a mouse, to click this button. This is a security feature of macOS and is out of our control.

## Uninstallation
In order to uninstall the driver: navigate to the preference pane by opening your "System Preferences," navigating to the "Xbox 360 Controllers" pane, clicking on the "Advanced" tab and pressing the "Uninstall" button. This will prompt you to enter your password so that the uninstaller can remove all of the bundled software from your machine.

## Usage
The driver exposes a standard game pad with a number of standard controls, so any game that supports gaming devices should work. In some cases, this may require an update from the developer of the game. The preference pane uses the standard macOS frameworks for accessing HID devices in addition to access of Force Feedback capabilities. This means that the preference pane is a good indicator that the driver is functional for other programs.

It is important to note that this driver does not work, and can never work, with Apple's "Game Controller Framework." This GCController framework corresponds to physical gamepads that have been offically reviewed by Apple and accepted into the mFi program. Due to the fact that we are not Microsoft, we cannot get their gamepad certified to be a GCController. This is an unfortunate oversight on Apple's part. If you would like to discuss this, please do so at [this location.](https://github.com/360Controller/360Controller/issues/55)

Users have been maintaining a [partial list of working and non-working games.](https://github.com/360Controller/360Controller/wiki/Games) Please contribute your findings so that you can help others debug their controller issues.

## My controller doesn't work!

### I'm using a driver from the Tattiebogle website
The Tattiebogle driver is NOT the same driver as this Github project. We do NOT support that driver. Under NO circumstances will we support that driver. If you download the latest version of this driver from the [releases page](https://github.com/360Controller/360Controller/releases) we will do our best to help you out. This driver will install over the Tattiebogle driver. You don't have to worry about uninstalling the Tattiebogle driver first.

### My controller doesn't work with a game!
We cannot fix game specific issues. This driver does its absolute best to put out a standardized format for games to use. If they don't take advantage of that, there is **ABSOLUTELY NOTHING** we can do. The best we can do for you is give you the "Pretend to be an Xbox 360 Controller" option in the "Advanced" tab. This will make any wired Xbox 360 or wired Xbox One controller appear to games as if it were an official Microsoft Xbox 360 Controller. That way if the game is only looking for Xbox 360 controllers and isn't looking for other devices like third party Xbox 360 controllers or Xbox One controllers, you should be able to trick the game. If you experience an issue with a game that this toggle does not fix, we cannot help you, sorry. That is just the nature of drivers.

### How do I find my Vendor ID and Product ID?
Navigate to the Apple menu at the top left corner of your screen. Select the `About This Mac` option. This will open a new window, where you need to select `System Report...` in the `Overview` tab. This will open another new window. On the left hand side of this window, there will be a number of options. Select `USB`. It will be somewhere near the bottom of the `Hardware` section. This will show you the USB device tree. Find and click on the entry that corresponds to your controller. This will provide you with the information needed at the bottom of the window. If you cannot find your device, make sure that all devices are properly connected to the computer. Try different cables if the controller still is not found.

### Original Xbox Controllers
Make an issue describing your problem.

### Wired Xbox 360 Controllers
Always check your controller with the preference pane found at: `Apple Menu -> System Preferences -> Xbox 360 Controllers` before creating an issue. If the controller works in this menu, then the driver is operating as intended. If your controller works with this menu, but not with a specific game, then read the [My controller doesn't work with a game!](#my-controller-doesnt-work-with-a-game) section.
If you have a third party controller, make an issue following the template with the "Product ID" and "Vendor ID" of the controller. Follow [How do I find my Vendor ID and Product ID?](#how-do-i-find-my-vendor-id-and-product-id) for instructions on how to find this information.

### Wireless Xbox 360 Controllers
**As of macOS 10.11, Wireless Xbox 360 controller support causes kernel panics. This issue cannot be resolved with minor changes to the driver, and requires that the driver be re-written from scratch to resolve the issue. Due to an excess of caution, we have disabled Wireless Xbox 360 controller support as of 0.16.6. If you want to use a wireless controller, download 0.16.5 or earlier and disable the driver before the computer enters a "sleep" state in order to prevent kernel panics. Alternatively, you can revert to a macOS version before 10.11 to avoid this issue.**

### Xbox One Controllers connected with USB
Always check your controller with the preference pane found at: `Apple Menu -> System Preferences -> Xbox 360 Controllers` before creating an issue. If the controller works in this menu, then the driver is operating as intended. If your controller works with this menu, but not with a specific game, then read the [My controller doesn't work with a game!](#my-controller-doesnt-work-with-a-game) section.
If your controller is recognized by the preference pane, but you aren't getting any response from button presses, this is likely due to an issue with macOS 10.11 and later. Apple changed some of the underlying USB code with this release and broke compatibility with some controllers. This is specifically found in controllers from PDP and PowerA. If you revert to macOS 10.10 or earlier, these controllers will work.
If the preference pane can't find your controller, make sure that it is listed in `Apple Menu -> About this Mac -> System Report -> Overview -> Hardware -> USB`. This menu should list a device called "Controller." If it isn't listed there, then you likely have a "charge" Micro USB cable instead of a "data" cable. If the cable isn't sending data, then you can't use the driver. Try a different cable.
If you have a third party controller, make an issue following the template with the "Product ID" and "Vendor ID" of the controller. Follow [How do I find my Vendor ID and Product ID?](#how-do-i-find-my-vendor-id-and-product-id) for instructions on how to find this information.
**At this time, PDP and PowerA controllers are unsupported by this driver as of macOS 10.11+ thanks to a rewrite of the macOS USB kernel. We cannot resolve this issue. It is a bug in Apple's core OS code.**

### Xbox One Controllers connected with Wireless Adapter
Xbox One controllers connected with the Wireless Adapter are currently not supported. Please be patient as we figure out this complicated protocol.

### Xbox One Controllers connected with Bluetooth
The Xbox One controller works with macOS automatically when connected over Bluetooth via System Preferences. Only specific Xbox One controllers released after August 2016 have Bluetooth capability. See [Microsoft's support page](https://support.xbox.com/en-US/xbox-on-windows/accessories/connect-and-troubleshoot-xbox-one-bluetooth-issues-windows-10) for determining if your controller supports Bluetooth. Due to the fact that this controller works by default, it will not be supported by this driver. If you choose to plug this controller in via USB, you will need this driver. If you do not wish to connect the controller via USB, then you do not need this driver. Any problems with game compatibility in Bluetooth mode are completely out of our control and are up to you to solve in conjunction with the game developer.

### Xbox One Adaptive Controller
The Xbox One adaptive controller can connect to your macOS machine through either a Bluetooth or wired connection. In Bluetooth mode, it is not controlled by the driver in any way, and will not show up in the "Xbox 360 Controllers" preference pane. If you are having issues with a wired connection, where the preference pane is recognizing your controller, but isn't recieving inputs, please connect it to a PC or VM running Windows in order to recieve a crucial firmware update. This update may also be possible through an Xbox One console.

## Adding Third Party Controllers
First, [disable signing requirements](#disabling-signing-requirements) so that you can run your custom build with your third party controller added. Then edit `360Controller/360Controller/Info.plist`. Add your controller following the pattern of pre-existing controllers by adding your vendor and product IDs to a new entry. After this, follow the information in the [building](#building) section, following the "If you don't have a signing certificate" path to build your new .kext. Then, place your shiny new `360Controller.kext` in to `/Library/Extensions` over the old one. You may need to take ownership of the driver in order for it to operate properly. You can do this with `sudo chown -R root:wheel /Library/Extensions/360Controller.kext`. Then, to make sure everything went according to plan, run `sudo kextutil /Library/Extensions/360Controller.kext`. This will load your kext into the OS and you should be able to use your controller. Once you reboot, your custom driver should be loaded automatically.

## Developer Info
Drivers inherently modify the core operating system kernel. Using the driver as a developer can lead to dangerous kernel panics that can cause data loss or other permanent damage to your computer. Be very careful about how you use this information. We are not responsible for anything this driver does to your computer, or any loss it may incur. Normal users will never have to worry about the developer section of this README.

### Building

##### Apple has recently changed how drivers work in Xcode 7. In order to build the driver, you will need Xcode 6.4 or earlier.
Additionally, to use the included build scripts, you will need to change your preferred Xcode installation using `xcode-select`.

##### You must have a signing certificate to install a locally built driver. Alternatively, you can disable driver signing on your machine, however this is a major security hole and the decision should not be taken lightly.

You will need a full installation of Xcode to build this project. The command line tools are not enough.

The project consists of three main parts: The driver (implemented in C++, as an I/O Kit C++ class), the force feedback plugin (implemented in C, as an I/O Kit COM plugin) and the preference pane (implemented in Objective C as a preference pane plugin). To build, use the standard Xcode build for Deployment on each of the 3 projects. Build Feedback360 before 360Controller, as the 360Controller project includes a script to copy the Feedback360 bundle to the correct place in the .kext to make it work.

To debug the driver, `sudo cp -R 360Controller.kext /tmp/` to assign the correct properties - note that the Force Feedback plugin only seems to be found by OSX if the driver is in /System/Library/Extensions so it can only be debugged in place. Due to the fact that drivers are now stored in /Library/Extenions, this means that you must create a symlink between the location of the driver and /System/Library/Extensions so that the force feedback plugin can operate properly.

### Building the .pkg

In order to build the .pkg, you will need to install [Packages.app](http://s.sudre.free.fr/Software/Packages/about.html).

#### If you don't have a signing certificate

* Open `360 Driver.xcodeproj` using Xcode.
* Select the `360 Driver` project in the Navigator.
* Select the `360Daemon` target from the top right corner.
* Select the `Build Settings` tab from the top of the screen.
* In the `Code Signing` section, find `Code Signing Identity` section and expand it.
* In the `Release` section, change the selection to `Don't Code Sign`.
* Set the code signing identity for `360Daemon`, `Feedback360`, `360Controller`, `DriverTool`, `Pref360Control`, `Wireless360Controller`, `WirelessGamingReceiver` and `Whole Driver`.
* Run `./build.sh` to build the .pkg. This .pkg can be found in the `Install360Controller` directory.

#### If you have a signing certificate

* Create a file named `DeveloperSettings.xcconfig`
* Select the `360 Driver` project in the Navigator.
* In this file, add the following lines:
   * `DEVELOPMENT_TEAM = XXXXXXXXXX` where `XXXXXXXXXX` is the development team on your Developer ID Application and Installer certificates.
   * `DEVELOPER_NAME = First Last` where `First Last` is the name on the Developer ID Installer certificate.
   * `DEVELOPER_EMAIL = my.address@email.com` where `my.address@email.com` is the email address of your Apple account that has your Developer ID Application and Installer certificates.
   * `NOTARIZATION_PASSWORD = abcd-efgh-ijkl-mnop` where `abcd-efgh-ijkl-mnop` is a temporary password that you have generated for your Apple account for the purposes of notarization.

### Disabling signing requirements

Since Yosemite (macOS 10.10) all global kexts are required to be signed. This means if you want to build the drivers and install locally, you need a very specific signing certificate that Apple closely controls. If you want to disable the signing requirement from macOS, you will need to do several things.

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

Reboot into macOS like normal. You can reset the boot arguments by executing this command:
```
sudo nvram -d boot-args
```
This will remove ALL boot-args. If you have previously manipulated your boot-args, those changes will be erased as well!

### Notarization of the driver

This is only possible if you have a signing certificate, but it is a relatively straightforward process.

* Build the driver as previously instructed and make sure to include the necessary information in your `DeveloperSettings.xcconfig` file, as they will be used during this process.
* Make sure to `cd` into the `Install360Controller` directory and run `./makedmg.sh`
* Run `./notarize.sh`
* This should finish with the message: `The validate action worked!`

Then you can distribute the notarized and stapled version of the driver.

### Debugging the driver

Debugging the driver depends on which part you intend to debug. For the 360Controller driver itself, it uses `IOLog` to output to the `system.log` which can be accessed using Console.app. Feedback360 uses `fprintf(stderr, ...)`, which should appear within the console of the program attempting to use force feedback.

### Debugging the preference pane

Most of these instructions are pulled directly from [this blog post.](http://www.condition-alpha.com/blog/?p=1314) Please visit it for futher information.

First, create a copy of `System Preferences.app` called `System Preferences (signed).app`. Then sign this new System Preferences with the command:

```codesign -s "Developer ID Application: First Last (XXXXXXXXXX)" -f /Applications/System\ Preferences\ \(signed\).app/```

where `Developer ID Application: First Last (XXXXXXXXXX)` is the name of your Developer ID Application signing certificate.

Edit your build scheme for Pref360Control, and select the "Run" scheme, and make sure you are editing "Debug" (A). In the environment variables section, click on "+" to add a new environment variable (B). Name the new variable OBJC_DISABLE_GC, and set its value to YES.

Next, click the little disclosure triangle for the run scheme to reveal its detailed settings. Then select pre-actions. Click the "+" at the bottom to add a run script action. Enter /bin/sh as the shell, make sure that your target is selected to provide build settings, and type a shell command line to install the newly compiled pref pane in your personal Library folder:

```cp -Rf ${CONFIGURATION_BUILD_DIR}/Pref360Control.prefPane ~/Library/PreferencePanes```

Finally, select the run step, choose "other" from the executable drop-down menu, and select `System Preferences (signed)` in the Applications folder. Verify that "Debug executable" and "Automatically" are both checked.

### A note on Unity mappings

The issues with the button and axis mappings in the Unity game engine are outside of our control. Unity mangles the button and axis values provided by the controller and remaps them to different values. There is absolutely no way that we can introduce a shim to fix it. Complaints about this should be directed at Unity, not at us.

## Licence

Copyright (C) 2006-2013 Colin Munro

This driver is licensed under the GNU Public License. A copy of this license is included in the distribution file, please inspect it before using the binary or source.
