#!/bin/bash
model="atmega16u2"

dfu_prog="dfu-programmer"
# dfu_prog="/home/dilli/arduino/dfu-programmer_0-6-1"

to_arduino=1
to_keyboard=1
flash_code=1

function showHelp() {
    cat <<-EOD
  -a | arduino    stay at arduino firmware
  -A | Arduino    only flash arduino firmware but no code
  -k | keyboard   only flash keyboard firmware

  default: flash arduino firmware if needed, flash code, flash keyboard firmware
EOD
}

while [ -n "$1" ]; do
    case "$1" in
        "-a"|"arduino")
            to_keyboard=
            ;;
        "-A"|"Arduino")
            to_keyboard=
            flash_code=
            ;;
        "-k"|"keyboard")
            to_arduino=
            flash_code=
            ;;
        "-h"|"--help")
            showHelp
            exit 0
            ;;
    esac
    shift
done

# output of lsusb:
# default: Arduino SA Uno (CDC ACM)
# DFU: Atmel Corp. atmega16u2 DFU bootloader
# Keyboard: Atmel Corp. LUFA Keyboard Demo Application

function waitForDFU() {
    while ! lsusb | grep -q  "DFU bootloader"; do
        echo "Put your Arduino in DFU mode and press enter."
        read
    done
}

echo -n "current mode: "
if lsusb | grep -q  " Arduino SA Uno (CDC ACM)"; then
    echo "Arduino"
    to_arduino=
elif lsusb | grep -q  "DFU bootloader"; then
    echo "Bootloader"
elif lsusb | grep -q  "LUFA Keyboard"; then
    echo "Keyboard (LUFA)"
elif lsusb | grep -q  "Shorty"; then
    echo "Keyboard (Shorty)"
else
    echo "unknown!"
    exit 1
fi

# echo "flash_code $flash_code"
# echo "to_arduino $to_arduino"
# echo "to_keyboard $to_keyboard"



if [ -n "$to_arduino" ]; then
    echo "flashing Arduino firmware"
    waitForDFU

    echo sudo "$dfu_prog" "$model" erase
    sudo "$dfu_prog" "$model" erase
    sudo "$dfu_prog" "$model" flash fw/Arduino-usbserial-uno.hex

    if [ $? != 0 ]; then
        echo "Arduino not found"
        exit 2
    fi

    echo "Unplug your Arduino, plug it back in and press enter."
    read
fi


if [ -n "$flash_code" ]; then
    pio run -t upload || exit 1
fi


if [ -n "$to_keyboard" ]; then
    echo "flashing Keyboard firmware"
    waitForDFU

    sudo "$dfu_prog" "$model" erase
    sudo "$dfu_prog" "$model" flash fw/Arduino-keyboard-shorty.hex

    if [ $? != 0 ]; then
        echo "Arduino not found"
        exit 3
    fi

    echo "Unplug your Arduino and plug it back in. Done."
fi

