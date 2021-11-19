#!/bin/bash

# Set backlight and button highlights on a shorty keyboard.
#
# Details:
# Sends signals to shorty keyboard via the status LEDs.
# USB Keyboards receive the state their LEDs should be in as one byte.
# The 3 upper most bits are constant, we use on bit as a latching bit
# and are left with 4 bits. This allows us to transmit 15 distinct signals
# to the shorty firmware.
#
# data ------------\
# latch ---------\ |
#                |/|\_
#              0b00000
#                |||||
# LED_KANA ------/||||
# LED_COMPOSE ----/|||
# LED_SCROLLLOCK --/||
# LED_CAPSLOCK -----/|
# LED_NUMLOCK -------/

device_path=

function help() {
    cat <<-END
Usage:
$0 [ clear | light [0|1] | highlight [1-6] [0|1] ] [...]
    clear      turn off backlight and all highlights
    light      turn backlight on (1) or off (0)
    highlight  turn highlight of button [index] on (1) or off (0)

Multiple commands can be chained:
    $0 clear highlight 1 1 highlight 6 1
END
    exit ${1:-0}
}

findDeviceNumber(){ # stackoverflow to the rescue!
    v=${1%:*}; p=${1#*:}  # split vid:pid into 2 vars
    v=${v#${v%%[!0]*}}; p=${p#${p%%[!0]*}}  # strip leading zeros
    grep -il "^PRODUCT=$v/$p" /sys/bus/usb/devices/*:*/uevent |
    sed s,uevent,, |
    xargs -r grep -r '^DEVNAME=' --include uevent |
    grep input | sed 's/.*input\/input\([0-9]\+\)\/event.*$/\1/'
}

function findDevicePath() {
    device_number="$(findDeviceNumber 03eb:2042)"
    if [ -n "$device_number" ]; then
        device_path="/sys/class/leds/input$device_number::LED/brightness"
    fi
}

function setLed() {
    echo "$2" | sudo tee ${device_path/LED/$1} > /dev/null
}

function latch() {
    setLed kana "1"
    sleep .1
    setLed kana "0"
    sleep .1
}

function sendBits() {
    [ -z "$device_path" ] && findDevicePath
    if [ -z "$device_path" ]; then
        echo "device not found"
        exit 1
    fi

    # unlatch
    setLed kana "0"
    setLed numlock "$1"
    setLed capslock "$2"
    setLed scrolllock "$3"
    setLed compose "$4"
    latch
}



if [ "$*" == "" ]; then
    echo "No command given"
    help 1
fi

while [ -n "$1" ]; do
    case "$1" in
        "-h")
            help
            ;;
        "-d")
            device_path="/sys/class/leds/input$2::LED/brightness"
            shift
            ;;
        "light")
            # 0bX000
            state=0
            [ "$2" == "1" ] && state=1
            sendBits 0 0 0 "$state"
            shift
            ;;
        "light_on")
            # 0b1000 / 8
            sendBits 0 0 0 1
            ;;
        "light_off")
            # 0b0000
            sendBits 0 0 0 0
            ;;
        "highlight")
            # 0b001 - 0b110 / 1 - 6
            [ -z "$2" ] && help 2
            [ -z "$3" ] && help 2
            [ $2 -gt 6 ] && help 2
            state=0
            [ "$3" == "1" ] && state=1

            sendBits "$(($2 & 1))" "$(($(($2 >> 1)) & 1))" "$(($(($2 >> 2)) & 1))" "$state"
            shift 2
            ;;
        "clear")
            # 0b0111 / 7
            sendBits 1 1 1 0
            ;;
        "effect")
            # 0b1111 / 15
            sendBits 1 1 1 1
            ;;
        *)
            echo "unknown command '$1'"
            help 1
    esac
    shift
done

