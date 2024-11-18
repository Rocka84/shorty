#!/bin/bash
cd "$(dirname "$0")"

model="atmega16u2"

dfu_prog="dfu-programmer"

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
        "-k"|"-K"|"keyboard")
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

dev_name_arduino="Arduino SA Uno"
dev_name_dfu="DFU bootloader"
dev_name_keyboard="Shorty Keyboard"
dev_name_lufa="LUFA Keyboard"

function waitForDFU() {
    echo "Put your Arduino in DFU mode now."
    while ! lsusb | grep -q  "$dev_name_dfu"; do
        sleep 1
    done
}

echo -n "current mode: "
if lsusb | grep -q  "$dev_name_arduino"; then
    echo "Arduino"
    to_arduino=
elif lsusb | grep -q  "$dev_name_dfu"; then
    echo "DFU"
elif lsusb | grep -q  "$dev_name_lufa"; then
    echo "Keyboard (LUFA)"
elif lsusb | grep -q  "$dev_name_keyboard"; then
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
    sudo -v
    waitForDFU

    sudo "$dfu_prog" "$model" erase
    sudo "$dfu_prog" "$model" flash fw/Arduino-usbserial-uno.hex

    if [ $? != 0 ]; then
        echo "Arduino not found"
        exit 2
    fi

    [ -z "$flash_code" ] && exit 0

    echo "Unplug your Arduino and plug it back in."
    while ! lsusb | grep -q  "$dev_name_arduino"; do
        sleep 1
    done
fi


if [ -n "$flash_code" ]; then
    pio run -t upload || exit 1
fi


if [ -n "$to_keyboard" ]; then
    echo "flashing Keyboard firmware"
    sudo -v
    waitForDFU

    sudo "$dfu_prog" "$model" erase
    sudo "$dfu_prog" "$model" flash fw/Arduino-keyboard-shorty.hex

    if [ $? != 0 ]; then
        echo "Arduino not found"
        exit 3
    fi

    echo "Unplug your Arduino and plug it back in. Done."
fi

