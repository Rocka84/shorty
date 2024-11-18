#!/bin/bash

cd "$(dirname "$0")"

if [ ! -d "lufa-arduino-keyboard" ]; then
    git clone --depth 1 --single-branch --branch arduino-keyboard-cdc https://github.com/Rocka84/lufa-arduino-keyboard.git
fi

cd lufa-arduino-keyboard/Projects/arduino-keyboard-cdc
make
cp Arduino-keyboard-cdc.hex ../../../Arduino-keyboard-shorty.hex

