import serial
import time

#setup the serial line
ser =serial.Serial('COM7', 9600)
time.sleep(3)

date=[]
for i in range(50):
    b = ser.readline()
    string_n = b.decode()
    string = string_n.rstrip()
    print(string)
    time.sleep(1)

ser.close()