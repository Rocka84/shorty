#!/bin/bash


findDeviceNumber(){
    v=${1%:*}; p=${1#*:}  # split vid:pid into 2 vars
    v=${v#${v%%[!0]*}}; p=${p#${p%%[!0]*}}  # strip leading zeros
    grep -il "^PRODUCT=$v/$p" /sys/bus/usb/devices/*:*/uevent |
    sed s,uevent,, |
    xargs -r grep -r '^DEVNAME=' --include uevent |
    grep input | sed 's/.*input\/input\([0-9]\+\)\/event.*$/\1/'
}

# data -------------\
# latch ----------\ |
#                 |/|\
#               0b0000
#                 ||||
# LED_COMPOSE ----/|||
# LED_SCROLLLOCK --/||
# LED_CAPSLOCK -----/|
# LED_NUMLOCK -------/


device_number="$(findDeviceNumber 03eb:2042)"
if [ -z "$device_number" ]; then
    echo "device not found"
    exit 1
fi

device_path="/sys/class/leds/input$device_number::LED/brightness"

function setLed() {
    echo "$2" | sudo tee ${device_path/LED/$1} > /dev/null
}

function latch() {
    setLed compose "1"
    sleep .15
    setLed compose "0"
    sleep .15
}

function sendBits() {
    setLed numlock "$1"
    setLed capslock "$2"
    setLed scrolllock "$3"
    latch
}

# unlatch
setLed compose "0"

while [ -n "$1" ]; do
    case "$1" in
        "-d")
            device_number="$2"
            device_path="/sys/class/leds/input$device_number::LED/brightness"
            shift
            ;;
        "toggle_light")
            # 0b000
            sendBits 0 0 0
            ;;
        "highlight")
            # 0b001 - 0b110 / 1 - 6
            [ -z "$2" ] && exit 2
            [ $2 -gt 6 ] && exit 2
            sendBits "$(($2 & 1))" "$(($(($2 >> 1)) & 1))" "$(($(($2 >> 2)) & 1))"
            shift
            ;;
        "clear")
            # 0b111 / 7
            sendBits 1 1 1
            ;;
        *)
            echo "unknown command"
            exit 1
    esac
    shift
done

