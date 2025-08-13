#!/bin/bash
for x in udp_radio httpd; do
    echo Stopping $x
    pgrep $x | xargs kill
done
