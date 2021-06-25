#!/bin/bash

export FLASK_APP=EspCollector
export PATH=$PATH:/home/vivek/Development/iot-learning/raspberrypi

if [ -z "$1" ]
then
    export FLASK_PORT=5000
else
    export FLASK_PORT=$1

echo $FLASK_PORT

cd /home/vivek/Development/iot-learning/raspberrypi
flask run --host=0.0.0.0 --port=$FLASK_PORT