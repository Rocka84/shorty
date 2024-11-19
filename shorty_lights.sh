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

device_vid="1209"
device_pid="fabd"
device_path=

function help() {
    cat <<-END
Usage:
$0 [ r[eset] | l[ight] [0|1|00|2-n] | b[utton] [1-6] [0|1|00|2-n] | e[ffect] | s[leep] [1-n] ] [...]
    r | reset      turn off backlight and all highlights
    l | light      turn backlight on (1) or off (0)
    b | button     turn highlight of button [index] on (1) or off (0)
    e | effect     toggle effect mode
    s | sleep [n]  sleep [n] seconds

Parameter for light and button:
    1   turn on or set next color if already on
    0   turn off or reset color if already off
    00  turn off and reset (same as 0 twice)
    n   turn on and or set next color n-times

Examples:
    # commands can be chained: reset and turn button 1 and 6 on
    $0 reset button 1 1 button 6 1
    # set button 3 to the fifth color and turn back off
    $0 b 3 00 b 3 5 b 3 0
    # turn button 3 on with the last used color
    $0 b 3 1
    # next color for button 3
    $0 b 3 1
    # turn button 3 off and reset colot
    $0 b 3 00
    # blink button 2 three times with second color
    $0 b 2 00 b 2 2 s 1 b 2 0 s 1 b 2 1 s 1 b 2 0 s 1 b 2 1 s 1 b 2 0

END
    exit ${1:-0}
}

findDeviceNumber(){ # stackoverflow to the rescue!
    v=${1#${1%%[!0]*}}  # strip leading zeros
    p=${2#${2%%[!0]*}}

    grep -il "^PRODUCT=$v/$p" /sys/bus/usb/devices/*:*/uevent |
    sed s,uevent,, |
    xargs -r grep -r '^DEVNAME=' --include uevent |
    grep input | sed 's/.*input\/input\([0-9]\+\)\/event.*$/\1/'
}

function findDevicePath() {
    device_number="$(findDeviceNumber "$device_vid" "$device_pid")"
    if [ -n "$device_number" ]; then
        device_path="/sys/class/leds/input$device_number::LED/brightness"
    fi
}

function ensureAccess() {
    [ -n "$access" ] && return
    if [ "$(id -u)" == "0" ]; then
        sudo=
    else
        sudo="sudo"
        rights="$(stat -c %A ${device_path/LED/kana})"
        if [ "${rights: -2:1}" == "w" ]; then
            access=1
            return
        fi
    fi
    for led in kana numlock capslock scrolllock compose; do
        $sudo chmod a+w ${device_path/LED/$led}
    done
    access=1
}

function setLed() {
    echo "$2" > ${device_path/LED/$1}
}

function latch() {
    setLed kana "1"
    sleep .01
    setLed kana "0"
    sleep .01
}

function sendBits() {
    [ -z "$device_path" ] && findDevicePath
    if [ -z "$device_path" ]; then
        echo "shorty not found"
        exit 1
    fi

    # echo "sendBits $1 $2 $3 $4   = 0b$4$3$2$1"

    ensureAccess

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
        "light"|"l")
            # 0bX000
            if [ "$2" == "0" ]; then
                sendBits 0 0 0 0
            elif [ "$2" == "00" ]; then
                sendBits 0 0 0 0
                sendBits 0 0 0 0
            elif [ "$2" -ge 1 ]; then
                for i in $(seq 1 "$2"); do
                    sendBits 0 0 0 1
                done
            fi
            shift
            ;;
        "highlight"|"h"|"button"|"b")
            # 0b001 - 0b110 / 1 - 6
            if [ -z "$2" ] || [ -z "$3" ] || [ $2 -gt 6 ]; then
                echo "invalid command: '$1 $2 $3'"
                help 2
            fi

            if [ "$3" == "0" ]; then
                sendBits "$(($2 & 1))" "$(($(($2 >> 1)) & 1))" "$(($(($2 >> 2)) & 1))" "0"
            elif [ "$3" == "00" ]; then
                sendBits "$(($2 & 1))" "$(($(($2 >> 1)) & 1))" "$(($(($2 >> 2)) & 1))" "0"
                sendBits "$(($2 & 1))" "$(($(($2 >> 1)) & 1))" "$(($(($2 >> 2)) & 1))" "0"
            elif [ "$3" -ge 1 ]; then
                for i in $(seq 1 "$3"); do
                    sendBits "$(($2 & 1))" "$(($(($2 >> 1)) & 1))" "$(($(($2 >> 2)) & 1))" "1"
                done
            fi
            shift 2
            ;;
        "clear"|"reset"|"r")
            # 0b0111 / 7
            sendBits 1 1 1 0
            ;;
        "effect"|"e")
            if [ "$2" == "next" ] || [ "$2" == "n" ]; then
                sendBits 1 0 0 0
                shift
            elif [ "$2" == "color" ] || [ "$2" == "c" ]; then
                sendBits 0 1 0 0
                shift
            elif [ "$2" == "speed" ] || [ "$2" == "s" ]; then
                sendBits 0 0 1 0
                shift
            else
                # 0b1111 / 15
                sendBits 1 1 1 1
            fi
            ;;
        "sleep"|"s")
            sleep "$2"
            shift
            ;;
        "ensureAccess")
            findDevicePath
            ensureAccess
            ;;
        "install")
            if [ "$(id -u)" != "0" ]; then
                echo "please run as root"
                exit 1
            fi
            if [ "$2" == "-f" ]; then
                force=1
                shift
            fi
            target="/usr/local/bin/shorty_lights"
            if [ -n "$force" ] || [ ! -x "$target" ]; then
                cp "$(realpath "$0")" "$target"
                echo "$(basename "$0") copied to /usr/local/bin/"
            else
                echo "$target already exists"
            fi
            target="/etc/udev/rules.d/00-shorty_lights.rules"
            if [ -n "$force" ] || [ ! -f "$target" ]; then
                rules="$(dirname "$0")/00-shorty_lights.rules"
                if [ -f "$rules" ]; then
                    cp "$(dirname "$0")/00-shorty_lights.rules" "$target"
                    udevadm control --reload-rules
                    udevadm trigger
                    echo "udev rules installed"
                else
                    echo "udev rules file not found"
                fi
            else
                echo "$target already exists"
            fi
            exit 0
            ;;
        "uninstall")
            if [ "$(id -u)" != "0" ]; then
                echo "please run as root"
                exit 1
            fi

            rm -f "/usr/local/bin/shorty_lights"
            echo "/usr/local/bin/shorty_lights removed"

            rm -f "/etc/udev/rules.d/00-shorty_lights.rules"
            udevadm control --reload-rules
            udevadm trigger
            echo "udev rules removed"
            exit 0
            ;;
        "debug")
            echo "device: $device_vid:$device_pid"
            findDevicePath
            if [ -z "$device_path" ]; then
                echo "shorty not found"
                exit 1
            fi
            echo -n "device_path: "
            echo $device_path

            echo -n "write access: "
            rights="$(stat -c %A ${device_path/LED/kana})"
            if [ "${rights: -2:1}" == "w" ]; then
                echo "yes"
            else
                echo "no"
            fi
            ;;
        "sendBits")
            sendBits "$2" "$3" "$4" "$5"
            shift 4
            ;;
        *)
            echo "unknown command '$1'"
            help 1
    esac
    shift
done

