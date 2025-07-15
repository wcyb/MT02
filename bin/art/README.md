# How to use
1. Select a blank ART partition according to your repeater version.
2. Run the *initialize_art.sh* script, giving as arguments the path to the selected blank partition and the base MAC address of the repeater.

As a result, you will get an initialized ART partition, which you can use instead of making a copy of this partition from the repeater.

# What is the base MAC address?
In the case of these repeaters, it is the MAC address of the **wlan0** interface (or in other words, the WiFi MAC address).
You can get this address by connecting through the serial port to the repeater and then using the `ifconfig` command, or any other way you prefer.
