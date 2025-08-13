#! /usr/bin/python3
import cgi, cgitb
import sys
import subprocess
import traceback

def hz_to_inc(hz):
    return int( hz * (2**27)/(125e6) )

def run(command):
    print(f"<br>Running: {command}")
    try:
        res = subprocess.run(command, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except:
        print("<br><pre>")
        traceback.print_exc()
        print("</pre>")
        sys.exit(1)
    print(f"<br><pre>{res.stdout}</pre>")

# Redirect stderr so we can see it in browser
sys.stderr = sys.stdout

# Create instance of FieldStorage
form = cgi.FieldStorage()

# Get data from fields
adc_freq_hz  = int(form.getvalue('adc_freq_hz'))
tune_freq_hz  = int(form.getvalue('tune_freq_hz'))
streaming = form.getvalue('streaming')
streaming_enabled_int = int(streaming == "streaming")

# Send the result to the browser
print ("Content-type:text/html")
print()
print ("<html>")
print ('<head>')
print ("<title>Radio Configurator</title>")
print ('</head>')
print ('<body>')
print ("<h2>Radio Configurator</h2>")
print ("Setting up the radio now...")
print ("ADC Freq = %d, Tune Freq = %d" %(adc_freq_hz,tune_freq_hz))
if (streaming == "streaming"):
    print ("streaming is Enabled<br>")
else :
    print ("streaming is Disabled<br>")

# adc
p=f"devmem 0x43c00000 w {hz_to_inc(adc_freq_hz)}"
run(p)

# tuner
p=f"devmem 0x43c00004 w {hz_to_inc(tune_freq_hz)}"
run(p)

# control (bit 0 is nreset, bit 4 is streaming enable)
p=f"devmem 0x43c00008 w 0x{streaming_enabled_int}0"
run(p)

#Get counter
print("getting counter for sanity check")
p="devmem 0x43c0000c w"
run(p)

print ('</body>')
print ('</html>')

