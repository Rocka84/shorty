#!/bin/bash

pio device monitor -b 9600 --no-reconnect --eol=CRLF --echo --filter send_on_enter

