# MT02
The MT02 is the cheapest no-name repeater you can currently find on AliExpress.
It costs from $2.69, but is available from various sellers at other prices.
The three main parts are recycled - SoC, RAM and Flash.
An 8MB Flash memory is installed from the factory, but it is easy to replace it with 16MB, and with the assumption that 16MB memory is mounted, U-Boot and OpenWRT ports were created.
There are at least two versions of this repeater, from the outside they look the same, and their packaging also looks identical.
As a bootloader, BREED (MT9533 version) and the old version of U-Boot (MT9341) are running on it by default, and the OS is a camouflaged LEDE.
Of course, the source codes are not available, but the manufacturer is also unknown.
Due to its small size, low price and not too bad performance, it can be a good platform for various projects.

![MT02](/doc/img/mt02.jpg)

# GPIO
In both versions of the repeater, the serial interface works with the same parameters: baud 115200 8N1 and 3.3V signal level.

If you are using a USB to UART converter in which the LEDs indicating activity on the RX and TX lines are directly connected to these lines, it is very possible that this converter will not work properly with your repeater.
To allow such converter to work properly, you need to desolder the mentioned LEDs or the resistors connected to them.

## MT9533
Order of UART pins, starting from the left: TX, RX, GND

![MT9533](/doc/img/mt9533.jpg)

## MT9341
Order of UART pins, starting from the left: GND, TX, RX

![MT9341](/doc/img/mt9341.jpg)

# Case
If you don't want to use the original case, in the _stl_ folder you will find files with the case I created as an alternative. You can print it on a 3D printer.

![render](/doc/img/render.png)

# U-Boot & OpenWRT
This repository contains modifications that need to be made to U-Boot and OpenWRT to add support for these repeaters.
With them, you can compile your version from the sources, or use the already compiled versions available in the _bin_ folder.

The modifications currently available were created based on:
| U-Boot      | OpenWRT     |
| ----------- | ----------- |
| 2025.07     | 24.10.4     |

# WiFi
By default, on first boot, a WiFi network named _MT02_ is created, with the password set to _mt02m300_.
This setting was created to allow easier configuration on first boot.
If you don't want to use WiFi or you have your own WiFi configuration that you don't want to be changed, just disable it and change the SSID to _disable-wifi_ (before OpenWRT 24.10.4) or to _skip-wifi_ (starting from OpenWRT 24.10.4).
This way, when you update OpenWRT, the initialization script _99-en-wifi-on-first-boot_, will leave the WiFi settings unchanged.

# Firmware preparation
If you want to make modifications to OpenWRT or U-Boot, or do the compilation yourself, all you need to do is:
1. Follow the official instructions for preparing the environment provided by the developers of U-Boot and OpenWRT.
2. Copy the files from this repository to the corresponding U-Boot and OpenWRT branch.
3. Follow the official instructions for compiling U-Boot and OpenWRT.

If you don't need to make modifications, just select the appropriate version of U-Boot and OpenWRT from the _bin_ folder.
Always choose the latest release. You will be able to determine the version you need based on the appearance of the repeater PCB.
Compare its appearance with the photos available above.

At this point you should have 2 BIN files.
One with U-Boot and the other with OpenWRT (**initramfs** version).

_There is an alternative to the process described below, but a better solution is to make a copy of the ART partition._
_The mentioned alternative can be found after the description of the partition copy process._

Since what we will create is incompatible with what is pre-installed, we need to make a copy of the ART partition.
To do this, connect to the repeater using a USB-UART converter (baud 115200 8N1, 3.3V signal level) and enter the following commands:

`passwd`

This will allow you to change the factory password. Set a simple one, because we will only need it temporarily.

`cat proc/mtd`

This command will list the partitions. Note the ART partition number.

```
dev:    size   erasesize  name
mtd0: 00010000 00010000 "u-boot"
mtd1: 00010000 00010000 "u-boot-env"
mtd2: 007c0000 00010000 "firmware"
mtd3: 00150000 00010000 "kernel"
mtd4: 00670000 00010000 "rootfs"
mtd5: 00370000 00010000 "rootfs_data"
mtd6: 00010000 00010000 "config"
mtd7: 00010000 00010000 "art"
```

In this case, it is the **mtd7** partition. Now we need to copy it. We can do it this way:

`cat /dev/mtd7 > tmp/art.bin`

All that's left is to download a copy of the ART partition to our PC. Using a PC with Linux installed, just type in terminal:

`scp -O -o HostKeyAlgorithms=ssh-rsa root@192.168.11.1:/tmp/art.bin /home/me/Desktop/`

_192.168.11.1_ is the IP address of the repeater, and _/home/me/Desktop/_ is the path where the art.bin file is to be saved on our PC.

An alternative to obtaining the ART partition from the repeater is to use a script to initialize a blank ART partition.
The script, along with instructions for use, can be found in the _bin/art_ folder.
The ART partition created with its help can be used instead of the one obtained from the repeater.

With a copy of the ART partition made, simply use the *combine_files.sh* script, which will combine U-Boot, its environment settings (optional), ART and OpenWRT at the appropriate offsets to create a file that you just need to load into 16MB Flash memory via a programmer.
To create this image, use the **initramfs** version of OpenWRT. Once the device is up and running, flash the **sysupgrade** version via either the console or GUI.
