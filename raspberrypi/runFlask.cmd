set FLASK_APP=EspCollector
set path=D:\DevelopmentTools\Python39\Scripts;%path%

if "%1"=="" (
    set FLASK_PORT=5000
) ELSE (
    set FLASK_PORT=%1
)
ECHO Flask Port: %FLASK_PORT%

cd D:\Vivek\Google Drive\Projects\iot\iot-learning\raspberrypi
flask run --host=0.0.0.0 --port=%FLASK_PORT%
