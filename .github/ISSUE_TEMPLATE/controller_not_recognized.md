---
name: Controller not recognized
about: Have a controller that is in the Info.plist but doesn't work? Get help here.
title: ''
labels: ''
assignees: ''

---

**Connection method**
A description of how you are connecting the controller to your mac. I.E. Wired, Wireless Adapter, Bluetooth, etc.

**Controller Details**
(If you don't know this information, please refer to the README for how to find it.)

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