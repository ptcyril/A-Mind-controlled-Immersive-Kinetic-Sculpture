import struct
import serial
import time

from tkinter import *
from tkinter import ttk

import HSV_RGB_converter

port_config = serial.Serial(port='COM3',baudrate=115200,timeout=1)
time.sleep(2)

# Application GUI setup
root = Tk()
frm = ttk.Frame(root, padding=10)
frm.grid()

ttk.Label(frm, text="An Immersive Mind-Controlled Kinetic Sculpture Manual Test Interface").grid(column=3, row=0)

# Button functions
def send_led_data():
    if port_config.is_open:
        LED = 1
        h = max(0, min(360, int(hue.get())))
        s = max(0, min(100, int(sat.get())))
        v = max(0, min(100, int(val.get())))

        rgb = HSV_RGB_converter.hsvtorgb(h,s,v)
    
        print(f'LED Data: <device=LED> <r={rgb[0]}> <g={rgb[1]}> <b={rgb[2]}>')

        led_data = struct.pack('<BBBBB',LED,rgb[0],rgb[1],rgb[2],60)
        port_config.write(led_data)
        
 
def send_motor_data():

    if port_config.is_open:

        LED = 1
        h = max(0, min(360, int(hue.get())))
        s = max(0, min(100, int(sat.get())))
        v = max(0, min(100, int(val.get()))) 
        rgb = HSV_RGB_converter.hsvtorgb(h,s,v)

        led_data = struct.pack('<BBBBB',LED,rgb[0],rgb[1],rgb[2],60)
        port_config.write(led_data)
        time.sleep(1)

        stop_status = stop_motor.get()

        if stop_status == False:
            step = int(step_setting.get())
            rpm = max(0, min(1200, speed.get()))
        
            motor_status = 0
            direct = int(direction.get())
            accelerate = accel.get()
            freq = int(rpm * (step * 200) / 60)

            print(f'Motor Data: <device=MOTOR> <status={motor_status}> <direction={direct}> <accel={accelerate}>'
                  f'<frequency={freq}>')

        elif stop_status == True:
            motor_status = 1
            direct = 0
            accelerate = accel.get()
            freq = 400

            print("MOTOR STOPPED")
            
        motor_data = struct.pack('<BBBHI',2,motor_status,direct,accelerate,freq)
        port_config.write(motor_data)

def pairing_mode():

    if port_config.is_open:
        PAIRING = 2
        
    
        print(f'Pairing Data:')

        pairing_data = struct.pack('<B',PAIRING)
        port_config.write(pairing_data)

def wire_test_mode():
    print("WIRE TEST")

def sound_setting():
    print("Sound")

def quitting_program():
    port_config.close()
    root.destroy()
    
    
# LED GUI
hue = IntVar()
ttk.Label(frm, text="LED hue: <0,360>").grid(column=0, row=1)
ttk.Entry(frm, textvariable=hue).grid(column=0, row=2)

sat = IntVar()
ttk.Label(frm, text="LED sat: <0,100>").grid(column=0, row=3)
ttk.Entry(frm, textvariable=sat).grid(column=0, row=4)

val = IntVar()
ttk.Label(frm, text="LED val: <0,100>").grid(column=0, row=5)
ttk.Entry(frm, textvariable=val).grid(column=0, row=6)

ttk.Button(frm, text="SEND LED DATA",command=send_led_data).grid(column=0, row=15)

# Motor GUI
step_setting = IntVar()
ttk.Label(frm, text="Motor step:").grid(column=5, row=1)
ttk.Combobox(frm,state="readonly",textvariable=step_setting,
             values=["1","2","4","8","16","32","64","128","5","10","20","25",
                     "40","50","100","125"]).grid(column=5, row=2)

direction = IntVar()
ttk.Label(frm, text="Motor dir: <CW 0|CCW 1>").grid(column=5, row=3)
ttk.Combobox(frm,state="readonly",textvariable=direction,values=["0","1"]).grid(column=5, row=4)



accel = IntVar()
ttk.Label(frm, text="Motor Accel:").grid(column=5, row=7)
ttk.Entry(frm, textvariable=accel).grid(column=5, row=8)

speed = IntVar()
ttk.Label(frm, text="Motor rpm:").grid(column=5, row=9)
ttk.Combobox(frm,state="readonly",textvariable=speed,values=["100","200","300","400","500","600","700","800","900","1000","1100","1200"]).grid(column=5, row=10)

stop_motor = BooleanVar()
Checkbutton(frm, text="STOP MOTOR DATA", variable=stop_motor, onvalue=True, offvalue=False).grid(column=5, row=14)

motor_button = ttk.Button(frm, text="SEND MOTOR DATA",command=send_motor_data).grid(column=5, row=15)

# Paring Mode
ttk.Button(frm, text="PAIRING MODE",command=pairing_mode).grid(column=3, row=3)

# Wire Limit Test
ttk.Button(frm, text="WIRE LIMIT TEST",command=wire_test_mode).grid(column=3, row=5)

# Sound Setting
ttk.Button(frm, text="Sound Setting",command=sound_setting).grid(column=3, row=7)

# Quit GUI
ttk.Button(frm, text="QUIT",command=quitting_program).grid(column=3, row=12)

root.mainloop()
