#!/bin/bash
model="atmega16u2"

to_arduino=
to_keyboard=
while [ -n "$1" ]; do
    case "$1" in
        "-a")
            to_arduino=1
            ;;
        "-k")
            to_keyboard=1
            ;;
        "-F")
            to_arduino=1
            to_keyboard=1
            ;;
    esac
    shift
done

# modes in lsusb:
# Arduino SA Uno (CDC ACM)
# DFU bootloader
# LUFA Keyboard Demo

function waitForDFU() {
    while ! lsusb | grep -q  " DFU bootloader"; do
        echo "Put your Arduino in DFU mode and press enter."
        read
    done
}

if [ -z "$to_arduino" ] && ! lsusb | grep -q  " Arduino SA Uno (CDC ACM)"; then
    to_arduino=1
fi

if [ -n "$to_arduino" ] && lsusb | grep -q  " Arduino SA Uno (CDC ACM)"; then
    to_arduino=
fi

if [ -n "$to_arduino" ]; then
    waitForDFU

    sudo dfu-programmer "$model" erase
    sudo dfu-programmer "$model" flash fw/Arduino-usbserial-uno.hex

    if [ $? != 0 ]; then
        echo "Arduino not found"
        exit 1
    fi

    echo "Unplug your Arduino and plug it back in and press enter."
    read
fi


pio run -t upload || exit 1


if [ -n "$to_keyboard" ]; then
    waitForDFU

    sudo dfu-programmer "$model" erase
    sudo dfu-programmer "$model" flash fw/Arduino-keyboard-0.3.hex

    if [ $? != 0 ]; then
        echo "Arduino not found"
        exit 1
    fi

    echo "Unplug your Arduino and plug it back in. Done."
fi
