import _thread
from ssdpy import SSDPServer
from sys import platform
import signal
import sys
import asyncio 
import subprocess
import time
import logging
import socket

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')    
    sys.exit(0)

def run_ssdpserver(threadname='ssdpServer',serverlocation=""):
    """
    Thread running the ssdp Server
    """
    logging.basicConfig()
    logger = logging.getLogger("ssdpy.server")
    logger.setLevel(logging.INFO)

    print("Starting thread",threadname)

    server = SSDPServer("rpi-home-automation", device_type="rpi-esp-monitor", location=serverlocation)
    server.serve_forever()

def run_apiserver(threadname='APIServer'):
    """
    Thread running the Flask API server
    """
    print("Starting ",threadname)
    
    if platform == "linux" or platform == "linux2":
        proc = subprocess.Popen(["/home/vivek/Development/iot-learning/raspberrypi/runFlask.sh",str(flash_port)])
    elif platform =="win32":
        proc = subprocess.Popen(["D:/Vivek/Google Drive/Projects/iot/iot-learning/raspberrypi/runFlask.cmd",str(flash_port)])        

    exit_code=proc.wait()
    print(exit_code)

def get_location(flask_host,flask_port):
    """
    Get the String Location of the flash server to send in the M-SEARCH request
    """

    return "http://"+flask_host+":"+str(flask_port)


"""
Main Program
"""

# Globals
flash_port = 5000

# Get My Location
# hostname = socket.gethostname()
# ip_address = socket.gethostbyname(hostname)

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(('8.8.8.8', 1))  # connect() for UDP doesn't send packets
local_ip_address = s.getsockname()[0]

location = get_location(local_ip_address,flash_port)

t2 = _thread.start_new_thread(run_apiserver,("API-Server", ))
time.sleep(10)
# creating thread
t1 = _thread.start_new_thread(run_ssdpserver,("SSDP-Server",location, ))
   
signal.signal(signal.SIGINT, signal_handler)
print('Press Ctrl+C to kill and exit')

loop = asyncio.get_event_loop()

try:
    loop.run_forever()
finally:
    loop.close()
