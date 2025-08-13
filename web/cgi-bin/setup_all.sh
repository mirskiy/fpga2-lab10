#!/bin/bash
echo "Content-type: text/html" # Tells the browser what kind of content to expect
echo "" # An empty line. Mandatory, if it is missed the page content will not load
echo "<p><em>"

# can't get configure_codec.sh to stay executable on my dev system for some reason
echo "setting configure_codec.sh executable<br>"
chmod a+x configure_codec.sh

echo "compiling udp_radio<br>"
echo "<pre>" && gcc udp_radio.c -o udp_radio 2>&1 && echo "</pre><br>"
echo "compiling test_radio<br>"
echo "<pre>" && gcc test_radio.c -o test_radio 2>&1 && echo "</pre><br>"
echo "compiling udpsender<br>"
echo "<pre>" && gcc udpsender.c -o udpsender 2>&1 && echo "</pre><br>"
echo "<br>"

echo "loading PL...<br>"
fpgautil -b design_1_wrapper.bit.bin
echo "</p></em><p>"
echo "configuring Codec...<br>"
./configure_codec.sh
echo "</p>"
echo "Starting my UDP Streamer Program Here...<br>"
./udp_radio 192.168.1.3 > ../udp_radio_log &
echo "log <a href='/udp_radio_log'>here</a><br>"
echo "Started UDP Streamer<br>"
echo "<p><em>All Done!</em></p>" 
