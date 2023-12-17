#!/bin/bash
model="atmega16u2"

dfu_prog="dfu-programmer"
# dfu_prog="/home/dilli/arduino/dfu-programmer_0-6-1"

to_arduino=1
to_keyboard=

while [ -n "$1" ]; do
    case "$1" in
        "-a"|"arduino")
            to_arduino=1
            to_keyboard=
            ;;
        "-k"|"keyboard")
            to_arduino=
            to_keyboard=1
            ;;
        "-F"|"full")
            to_arduino=1
            to_keyboard=1
            ;;
    esac
    shift
done

# output of lsusb:
# default: Arduino SA Uno (CDC ACM)
# DFU: Atmel Corp. atmega16u2 DFU bootloader
# Keyboard: Atmel Corp. LUFA Keyboard Demo Application

function waitForDFU() {
    while ! lsusb | grep -q  " DFU bootloader"; do
        echo "Put your Arduino in DFU mode and press enter."
        read
    done
}

if lsusb | grep -q  " Arduino SA Uno (CDC ACM)"; then
    echo "Arduino mode"
elif lsusb | grep -q  "DFU bootloader"; then
    echo "Bootloader"
elif lsusb | grep -q  "LUFA Keyboard"; then
    echo "Keyboard mode"
fi
echo

if [ -n "$to_arduino" ]; then
    waitForDFU

    sudo "$dfu_prog" "$model" erase
    sudo "$dfu_prog" "$model" flash fw/Arduino-usbserial-uno.hex

    if [ $? != 0 ]; then
        echo "Arduino not found"
        exit 1
    fi

    echo "Unplug your Arduino and plug it back in and press enter."
    read
fi

if [ -n "$to_keyboard" ]; then
    waitForDFU

    sudo "$dfu_prog" "$model" erase
    sudo "$dfu_prog" "$model" flash fw/Arduino-keyboard-0.3.hex

    if [ $? != 0 ]; then
        echo "Arduino not found"
        exit 1
    fi

    echo "Unplug your Arduino and plug it back in. Done."
fi

