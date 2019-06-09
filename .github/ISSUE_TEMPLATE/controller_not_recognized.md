---
name: Controller not recognized
about: Have a controller that doesn't work? Get help here.
title: ''
labels: ''
assignees: ''

---

**Connection Method**
A description of how you are connecting the controller to your mac. I.E. Wired, Wireless Adapter, Bluetooth, etc.

**Controller Details**
(If you don't know this information, please refer to the README for how to find it.)

**Output from Driver Enable**
360Controller installs a new pane in your System Preferences. Navigate to the "Advanced" tab and attempt to enable the driver.
If it provides any feedback, follow it. Otherwise let us know that it completes successfully.

***Product Name***
We like to attach an appropriate name to each entry in the Info.plist. Please provide a unique name for your device.

***Product Id***
The product id of your controller

***Vendor Id***
The vendor id of your controller

**`kextutil` output**
Please run the corresponding `kextutil` for your connection type.
For wired controllers: `sudo kextutil /Library/Extensions/360Controller.kext`
For wireless controller: `sudo kextutil /Library/Extensions/WirelessGamingReceiver.kext`