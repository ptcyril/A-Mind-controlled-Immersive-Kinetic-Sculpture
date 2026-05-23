import socketio
import threading
import json
import serial
import struct
import time
import csv
import os

import AIMCKS_GUI

sio = socketio.Client()
eeg_data = {}

CSV_FILE = "eeg_data_log.csv"


def initialize_csv():
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, mode="w", newline="") as file:
            writer = csv.writer(file)

            writer.writerow([
                "timestamp",
                "alpha_band",
                "beta_band",
                "theta_band",
                "delta_band",
                "gamma_band",
                "dominant_band",
                "alpha_beta_ratio",
                "alpha_delta_ratio",
                "peak_alpha_freq",
                "psd"
            ])

        print(f"Created CSV file: {CSV_FILE}")


def save_eeg_to_csv(eeg):
    try:
        with open(CSV_FILE, mode="a", newline="") as file:
            writer = csv.writer(file)

            writer.writerow([
                eeg.get("timestamp"),
                eeg.get("alpha_band"),
                eeg.get("beta_band"),
                eeg.get("theta_band"),
                eeg.get("delta_band"),
                eeg.get("gamma_band"),
                eeg.get("dominant_band"),
                eeg.get("alpha_beta_ratio"),
                eeg.get("alpha_delta_ratio"),
                eeg.get("peak_alpha_freq"),
                eeg.get("psd")
            ])

        print("EEG data saved to CSV")

    except Exception as e:
        print("CSV Save Error:", e)


@sio.event
def connect():
    print("EEG server Connected")


@sio.event
def disconnect():
    print("EEG server Disconnected")


@sio.on("eeg_data")
def on_eeg_data(data):
    global eeg_data

    try:
        eeg = json.loads(data)

        eeg_data = eeg

        print("EEG:", eeg)

        save_eeg_to_csv(eeg)

    except Exception as e:
        print("EEG Parse Error:", e)


def start_socket():
    try:
        sio.connect("https://signal-filter.onrender.com")
        sio.wait()

    except Exception as e:
        print("Socket Connection Error:", e)

initialize_csv()

threading.Thread(target=start_socket, daemon=True).start()


# =========================
# SERIAL PORT
# =========================

try:
    port_config = serial.Serial(
        port='COM3',
        baudrate=115200,
        timeout=1
    )

    time.sleep(2)

except serial.SerialException:
    port_config = None
    print("Serial port could not be opened")


# =========================
# CONFIG SAVE
# =========================

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
    print("Saved to config")


# =========================
# UART SEND
# =========================

def send_uart(values):

    def sender():

        if not (port_config and port_config.is_open):
            return

        # LED
        if values["type"] == "led":

            device = 1

            color = values["color"].lstrip("#")

            r = int(color[0:2], 16)
            g = int(color[2:4], 16)
            b = int(color[4:6], 16)

            fade = values["fade"]

            print(
                f"d:{device}, r:{r}, g:{g}, b:{b}, fade:{fade}"
            )

            led_data = struct.pack(
                '<BBBBB',
                device,
                r,
                g,
                b,
                fade
            )

            port_config.write(led_data)

        # MOTOR
        elif values["type"] == "motor":

            device = 2

            microstep = 2000

            freq = int((values["speed"] * microstep) / 60)

            print(
                f"d:{device}, "
                f"mode:{values['mode']}, "
                f"dir:{values['direction']}, "
                f"accel:{values['acceleration']}, "
                f"freq:{freq}"
            )

            motor_data = struct.pack(
                '<BBBHI',
                device,
                values["mode"],
                values["direction"],
                values["acceleration"],
                freq
            )

            port_config.write(motor_data)

    threading.Thread(target=sender, daemon=True).start()


# =========================
# PERFORMANCE CONTROLLER
# =========================

class PerformanceController:

    def __init__(self, send_uart):

        self.send_uart = send_uart
        self.running = False
        self.thread = None

    def start(self):

        if self.running:
            return

        self.running = True

        self.thread = threading.Thread(
            target=self.loop,
            daemon=True
        )

        self.thread.start()

    def stop(self):
        self.running = False

    def loop(self):

        while self.running:

            try:

                with open("config.json", "r") as f:
                    file = json.load(f)

                print("Running performance loop...")

                # Optional:
                # print current EEG values
                if eeg_data:
                    print("Current EEG dominant band:",
                          eeg_data.get("dominant_band"))

                for target in file:

                    for brainwave in file[target]:

                        for panel in file[target][brainwave]:

                            modules = file[target][brainwave][panel]["modules"]

                            # LED
                            if "led" in modules:

                                data = modules["led"]

                                self.send_uart({
                                    "type": "led",
                                    "color": data["color"],
                                    "fade": data["fade"]
                                })

                            # MOTOR
                            if "motor" in modules:

                                data = modules["motor"]

                                data["type"] = "motor"

                                self.send_uart(data)

                            # AUDIO
                            if "audio" in modules:

                                print(
                                    "Would play audio:",
                                    modules["audio"]["audio"]
                                )

                time.sleep(1)

            except Exception as e:

                print("Performance error:", e)

                time.sleep(1)


# =========================
# START APPLICATION
# =========================

controller = PerformanceController(send_uart)

app = AIMCKS_GUI.App(
    on_save=save_config,
    manual=send_uart,
    performance=controller
)

app.mainloop()