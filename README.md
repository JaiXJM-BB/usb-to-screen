# usb-to-screen Version 2

usb-to-screen is a QNX 8.0 OS driver built to monitor USB devices and HID compliant controllers and pump their events to the `screen` subsystem.

This was originaly written for our version of RetroPie, as the QNXE quickstart image does not provide many gamepad related inputs to the `screen` system by default.

## Build Instructions

All dependencies for this driver are included in a baseline SDP installation. Install your QNX SDP of choice (some version of QNX 8.0) or use one you already have installed - If you do not have a QNX License, you can get a free, noncommerical "QNXEverywhere" License here: https://www.qnx.com/products/everywhere/ (This is a version of QNX 8 which is intended for hobbyist or research purposes).

Once you have a baseline installed, simply source and run make as you would any project.
```bash
source ~/path/to/qnx800/qnxsdp-env.sh
cd ~/path/to/usb-to-screen
make
```
This uses the default QNX build system, so all build artifacts will just be put into the relevant nto-ARCHITECTURE folder. The binary is called `usb-to-screen`.

To run this, copy the appropriate binary over to your target (the one in nto-aarch64-le for aarch64 processors like on Raspberry Pi, etc) and call it from terminal.
```bash
#From this machine:
scp nto-aarch64-le/usb-to-screen <username>@<target ip or hostname> /path/to/install

#From the target machine:
cd /path/to/install
./usb-to-screen
```

## How it works

As mentioned above, this USB/HID Driver works more like an event pump, which detects any compatible/connected USB or HID devices and pumps their input to screen. To connect to the USB and HID, it uses the `usbdi` and `hiddi` libraries. Thus, your QNX target will need to have `io-usb` and `io-hid` running (though, io-hid is optional).

`io-usb` is QNX's default USB Stack, which communicates with the hardware. `io-hid` connects to the stack and exposes hid-compliant devices in a more desirable way. Because only one client can be connected to any given USB device, its important to note that `io-hid` should be booted before this driver so it has priority when connecting to devices. `io-usb` should be booted before either of these.

This driver then connects to `screen` as an input provider, sending events whenever the state of one of the USB devices changes.

Here is a diagram of how applications would read input from this driver:

![Diagram of an example system using this driver](readme-assets/usb-to-screen.png)

### Inside this driver

There are two sets of .c and .h files:
- usb-to-screen is the core of the driver. To update usb, hid, or screen connection, edit this!
- parser contains all parsing functions and lookup tables. To add device support, you only need to edit this file!

This driver connects to `io-usb` and `io-hid`, and uses their device insertion callbacks to scan for supported devices. These are stored by vid and pid in a list, which can be adjusted in `parser.c`.  Once a compatible device is found, usb-to-screen attaches to it.

If the device is seen over the USB connection, it finds a suitable endpoint and attaches, recording info such as the expected buffer length and handle into a custom structure. a usbd_io() call is set up to a callback function, which continuously re-queues the same URB with minor changes, thus establishing a continuous connection. This also calls fire_screen_event(), which calls the appropriate parsing function for buttons and analog inputs. These are then reported to screen, using an input provider context we attach.

### Adding Device Support

To add support for a device to this driver, edit `parser.c` to add the device info to the `_device_lookup` array, and add a parsing function. (You may need to add its declaration to `parser.h`)