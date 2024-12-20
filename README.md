# MT02
The MT02 is the cheapest no-name repeater you can currently find on AliExpress.
It costs from $2.69, but is available from various sellers at other prices.
The three main parts are recycled - SoC, RAM and Flash.
From the factory, 8MB of Flash is mounted, but it is easy to replace it with 16MB, and with the assumption that 16MB memory is mounted, U-Boot and OpenWRT ports were created.
There are at least two versions of this repeater, from the outside they look the same, and their packaging also looks identical.
As a bootloader, BREED (MT9533 version) and the old version of U-Boot (MT9341) are running on it by default, and the OS is a camouflaged LEDE.
Of course, the source codes are not available, but the manufacturer is also unknown.
Due to its small size, low price and not too bad performance, it can be a good platform for various projects.

![MT02](/doc/img/mt02.jpg)

# GPIO
## MT9533

![MT9533](/doc/img/mt9533.jpg)

## MT9341

![MT9341](/doc/img/mt9341.jpg)

# Case
If you don't want to use the original case, in the _stl_ folder you will find files with the case I created as an alternative. You can print it on a 3D printer.

![render](/doc/img/render.png)

# U-Boot & OpenWRT
This repository contains modifications that need to be made to U-Boot and OpenWRT to add support for these repeaters.

The modifications currently available were created based on:
| U-Boot      | OpenWRT     |
| ----------- | ----------- |
| 2024.10     | 23.05.5     |

# WiFi
By default, on first boot, a WiFi network named _MT02_ is created, with the password set to _mt02m300_.
This setting was created to allow easier configuration on first boot.
If you don't want to use WiFi, just disable it and change the SSID to _disable-wifi_.
This way, when you update OpenWRT, the initialization script _99-en-wifi-on-first-boot_, will leave the WiFi settings unchanged.

# Usage
For simplicity of use, all you need to do is:
1. Follow the official instructions for preparing the environment provided by the developers of U-Boot and OpenWRT.
2. Then copy the files from this repository to the corresponding U-Boot and OpenWRT branch.
3. Follow the official instructions for compiling U-Boot and OpenWRT.

# Firmware preparation
Since what we will create is incompatible with what is pre-installed (and even if it were not, it is better to have clean software), we need to remember to make a copy of the ART partition.
With a copy of that partition made, just use the *combine_files.sh* script, which will combine U-Boot, its environment settings (optional), ART and OpenWRT at the appropriate offsets to create a file that you just need to load into 16MB Flash memory.
To create this image, use the **initramfs** version of OpenWRT. Once the device is up and running, flash the **sysupgrade** version via either the console or GUI.
