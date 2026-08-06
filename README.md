# UBUS module for controlling ESP micro controller

For this task you will create a program which will let you control connected ESP micro controllers to the router over ubus.

Ask your internship manager to give you ESP micro controller. ESP controller will already have a
special firmware uploaded onto it which will let you control ESP pins over the serial. Documentation on how to control the micro controller can be found [here](https://github.com/janenasl/esp_control_over_serial).

Ubus should provide these methods:

- devices
- on
- off
- get

Devices method should return an array of devices with this information about devices:
- port
- vendor id
- product id

More than one device can be connected at a time.

On method should be used to turn on the pin and accept only these arguments:
- port
- pin

Off method should be used to turn off the pin and accept only these arguments:
- port
- pin​​​​​​​

Get method should be used to get data from attached sensors (at the moment only dth sensors are supported). This method should accept these parameters:
- port
- pin
- model
- sensor
 
If something went wrong, ubus should return an error message. Your program should automatically start after router reboot. Your program should work without UCI configuration file. All messages should be returned in JSON format using blob messages library.