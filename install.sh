#!/bin/bash

cd "$(dirname "$0")"

function installBin() {
    if [ ! -f "shorty-commander/shorty-commander" ]; then
        cd "shorty-commander"
        make || return 1
        cd ..
    fi

    sudo cp shorty-commander/shorty-commander /usr/local/bin/ || return 2
    echo "bin copied to /usr/local/bin/shorty-commander"
    return 0
}

function installUdev(){
    sudo cp "00-shorty.rules" "/etc/udev/rules.d/00-shorty.rules" || return 3
    sudo udevadm control --reload-rules || return 4
    sudo udevadm trigger || return 5
    echo "udev rules installed"
    return 0
}

function remove(){
    sudo rm -f /usr/local/bin/shorty-commander || return 1
    echo "bin removed"

    sudo rm -f "/etc/udev/rules.d/00-shorty.rules" || return 3
    sudo udevadm control --reload-rules || return 4
    sudo udevadm trigger || return 5
    echo "udev rules installed"
    return 0
}

if [ "$1" == "bin" ]; then
    installBin
    exit $?
fi

if [ "$1" == "udev" ]; then
    installUdev
    exit $?
fi

if [ "$1" == "remove" ]; then
    remove
    exit $?
fi

installBin || exit $?
installUdev || exit $?
