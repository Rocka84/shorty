# Shorty

An Arduino keyboard for shortcuts.

## Features

* 6 Buttons
    * send F13-F24 keys
    * short presses (F13-F18)
    * long presses (F19-F14)
    * recompile to send any key you like
* volume wheel
    * sends volume up/down keys
* individually addressable led per button
    * 8 predefined colors
    * configurable from your PC
* optional backlight
* 7 fancy effects
    * choose effect, color and speed
    * recompile to choose from a list of 50+ effects from WS2812FX
* button combos for most features
* PC script to control every feature
    * linux only for now

### Motivation

My goal was to have dedicated buttons to control microphone, camera and volume for video/audio calls,
that can also show different status with different colors.  

#### Why use an UNO?

My UNO was collecting dust for years and I wanted to breath some new life in it!  
But there are other Arduino (or comparable) boards that are much better suited for building USB devices.
So if you're thinking about your own new project, buy a different model :-D  

## Flashing

### prerequisits

* Arduino Uno
* dfu-programmer
* PlaformIO

### flash.sh

tl;dr: `./flash.sh` flashes in this order:

* the arduino bootloader to the usb controller (if necessary)
* the arduino firmware
* the keyboard firmware to the usb controller

Use `flash.sh -h` for more options.

## Hardware

I used:

* Arduino UNO
* 5 tactile buttons
* neopixel strip cut to 5 single leds
* rotary encoder
* micro-USB breakout board
* [3d printed parts](https://cad.onshape.com/documents/d508c5bf2328906736006f78/w/f44fa97748e9fe6ff14aafbd/e/1209bba8c7ca4193ff877ee7?renderMode=0&uiState=6599d43a8c26f72423a10e08)

### HowTo

I createad 5 buttons from a tactile switch, a neopixel and some 3d printed parts each.
Every button can be lit individually in different colors.

The sixth button is pressing down the rotary encoder. Unfortunately I haven't found a way to light that up (yet).

The micro-USB board is for convenience, I prefer using those cables over USB-B. It is soldered directly to the
solder points of the original connector on the bottom side of the board.

## Sources

* [darran's Arduino USB Keyboard firmware 2024 rebuild](https://github.com/Rocka84/lufa-arduino-keyboard/tree/shorty/Projects/arduino-keyboard)
    * Restored original source and dependencies from 2011 and made buildable in 2024
* [darran's Arduino USB Keyboard firmware 2011 original](https://web.archive.org/web/20120122114237/http://hunt.net.nz/users/darran/weblog/b3029/Arduino_UNO_Keyboard_HID_version_03.html)
    * Source of the infamous `Arduino-keyboard-0.3.hex` (via Wayback Machine)
* [Keyboard class for UNO by coopermaa](https://github.com/coopermaa/USBKeyboard)
    * interface to darran's firmware

