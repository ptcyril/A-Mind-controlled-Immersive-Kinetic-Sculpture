import struct
import serial
import time

from tkinter import *
from tkinter import ttk

import HSVtoRGB

port_config = serial.Serial(port='COM3',baudrate=115200,timeout=1)
time.sleep(2)

# Application GUI setup
root = Tk()
frm = ttk.Frame(root, padding=10)
frm.grid()

ttk.Label(frm, text="AIMCKS DYNAMIC DATA TEST INTERFACE").grid(column=3, row=0)

# Button functions
def send_led_data():
    if port_config.is_open:
        LED = 0
        h = max(0, min(360, int(hue.get())))
        s = max(0, min(100, int(sat.get())))
        v = max(0, min(100, int(val.get())))

        rgb = HSVtoRGB.hsvtorgb(h,s,v)
    
        print(f'LED Data: <device=LED> <r={rgb[0]}> <g={rgb[1]}> <b={rgb[2]}>')

        led_data = struct.pack('<BBBB',LED,rgb[0],rgb[1],rgb[2])
        port_config.write(led_data)
        
 
def send_motor_data():

    if port_config.is_open:
        stop_status = stop_motor.get()

        if stop_status == False:
            step = int(step_setting.get())
            rpm = max(0, min(1200, speed.get()))
        
            MOTOR = 1       
            motor_status = 1
            dir = int(direction.get())
            delta_t = delta_time.get()
            delta_f = delta_freq.get()
            freq = int(rpm * (step * 200) / 60)

            print(f'Motor Data: <device=MOTOR> <status={motor_status}> <direction={dir}> <dt={delta_t}>'
                  f'<df={delta_f}> <frequency={freq}>')

        elif stop_status == True:
            MOTOR = 1
            motor_status = 0
            dir = 0
            delta_t = delta_time.get()
            delta_f = delta_freq.get()
            freq = 0

            print("MOTOR STOPPED")
            
        motor_data = struct.pack('<BBBHHI',MOTOR,motor_status,dir,delta_t,delta_f,freq)
        port_config.write(motor_data)

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

delta_time = IntVar()
ttk.Label(frm, text="Motor time interval:").grid(column=5, row=5)
ttk.Entry(frm, textvariable=delta_time).grid(column=5, row=6)

delta_freq = IntVar()
ttk.Label(frm, text="Motor freq interval:").grid(column=5, row=7)
ttk.Entry(frm, textvariable=delta_freq).grid(column=5, row=8)

speed = IntVar()
ttk.Label(frm, text="Motor rpm:").grid(column=5, row=9)
ttk.Combobox(frm,state="readonly",textvariable=speed,values=["0","100","200","300","400","500","600","700","800","900","1000","1100","1200"]).grid(column=5, row=10)

stop_motor = BooleanVar()
Checkbutton(frm, text="STOP MOTOR DATA", variable=stop_motor, onvalue=True, offvalue=False).grid(column=5, row=14)
motor_button = ttk.Button(frm, text="SEND MOTOR DATA",command=send_motor_data).grid(column=5, row=15)

# Quit GUI
ttk.Button(frm, text="QUIT",command=quitting_program).grid(column=3, row=12)

root.mainloop()
