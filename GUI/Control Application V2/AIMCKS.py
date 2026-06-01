"""
sio = socketio.Client()
eeg_data = {}

@sio.event
def connect():
    print("EEG server Connected")

@sio.on("eeg_data")
def on_eeg_data(data):
    global eeg_data
    eeg = json.loads(data)
    eeg_data = eeg
    print("EEG:", eeg)

def start_socket():
    sio.connect("https://signal-filter.onrender.com")
    sio.wait()

threading.Thread(target=start_socket, daemon=True).start()
"""
import socketio
import threading

import AIMCKS_GUI 
import json
import serial
import struct
import time

try:
    port_config = serial.Serial(port='COM3',baudrate=115200,timeout=1)
    time.sleep(2)

except serial.SerialException:
    port_config = None
    print("Serial port could not be opened")

def save_config(parameters):
    target = parameters["target"]
    brainwave = parameters["brainwave_band"]
    panel = parameters["panel"]
    modules = parameters["modules"]

    try:
        with open("config.json", "r") as f:
            file = json.load(f)
    except:
        file = {}

    file.setdefault(target, {})
    file[target].setdefault(brainwave, {})
    file[target][brainwave].setdefault(panel, {})
    file[target][brainwave][panel].setdefault("modules", {})

    existing_modules = file[target][brainwave][panel]["modules"]

    for module_name, module_data in modules.items():
        existing_modules[module_name] = module_data

    with open("config.json", "w") as f:
        json.dump(file, f, indent=4)

    print(parameters)
    print("Saved to config:")

def send_uart(values):
    def sender():
        if not (port_config and port_config.is_open):
            return
        
        if values["type"] == "led":
            device = 1

            color = values["color"].lstrip("#")
            r = int(color[0:2], 16)
            g = int(color[2:4], 16)
            b = int(color[4:6], 16)
            fade = values["fade"]
            print(f" d:{str(device)} , r:{str(r)} , g:{str(g)} , b:{str(b)} , fade:{str(fade)}")
            led_data = struct.pack('<BBBBB',device,r,g,b,fade)
            port_config.write(led_data)

        elif values["type"] == "motor":
            device = 2

            microstep = 2000
            freq = (values["speed"] * microstep) / 60
            print(f" d:{str(device)} , mode:{str(values["mode"])} , dir:{str(values["direction"])} , accel:{str(values["acceleration"])} , freq:{str(freq)}")
            motor_data = struct.pack('<BBBHI',device, values["mode"], values["direction"], values["acceleration"], freq)
            port_config.write(motor_data)

    threading.Thread(target=sender, daemon=True).start()

class PerformanceController:
    def __init__(self, send_uart):
        self.send_uart = send_uart
        self.running = False
        self.thread = None

    def start(self):
        if self.running:
            return
        self.running = True
        self.thread = threading.Thread(target=self.loop, daemon=True)
        self.thread.start()

    def stop(self):
        self.running = False

    def loop(self):
    """Goal: Recieve dominant EEG Brainwave frequency and execute from config"""
        while self.running:
            try:
                with open("config.json", "r") as f:
                    file = json.load(f)

                print("Running performance loop...")

                for target in file:
                    for brainwave in file[target]:
                        for panel in file[target][brainwave]:
                            modules = file[target][brainwave][panel]["modules"]

                            if "led" in modules:
                                data = modules["led"]
                                self.send_uart({
                                    "type": "led",
                                    "color": data["color"],
                                    "fade": data["fade"]
                                })

                            if "motor" in modules:
                                data = modules["motor"]
                                data["type"] = "motor"
                                self.send_uart(data)

                            if "audio" in modules:
                                print("Would play audio:", modules["audio"]["audio"])

                time.sleep(1)

            except Exception as e:
                print("Performance error:", e)
                time.sleep(1)

controller = PerformanceController(send_uart)

app = AIMCKS_GUI.App(on_save=save_config, manual=send_uart, performance=controller)
app.mainloop()
