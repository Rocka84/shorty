# Shorty

An Arduino keyboard for shortcuts.

## Flashing

### prerequisits

* Arduino Uno
* dfu-programmer
* PlaformIO

### flash.sh

tl;dr: `./flash.sh -f` flashes in this order:

* the arduino bootloader to the usb controller (if necessary)
* the arduino firmware
* the keyboard firmware to the usb controller

Use `flash.sh -h` for more options.

## Hardware

@TODO

## Sources

* Keyboard firmware for Arduino Uno: https://github.com/coopermaa/USBKeyboard
