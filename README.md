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

@TODO more details, photos, links

What I used:

* an arduino uno
* tactile buttons
* neopixel strip cut to single segments
* rotary encoder
* micro-usb board
* [3d printed parts](https://cad.onshape.com/documents/d508c5bf2328906736006f78/w/f44fa97748e9fe6ff14aafbd/e/1209bba8c7ca4193ff877ee7?renderMode=0&uiState=6599d43a8c26f72423a10e08)

I createad 5 buttons from a tactile switch, a neopixel and some 3d printed parts each.
Every button can be lit individually in different colors.

The sixth button is pressing down the rotary encoder. Unfortunately I found no way to light that up yet.

## Sources

* Keyboard firmware for Arduino Uno: https://github.com/coopermaa/USBKeyboard

