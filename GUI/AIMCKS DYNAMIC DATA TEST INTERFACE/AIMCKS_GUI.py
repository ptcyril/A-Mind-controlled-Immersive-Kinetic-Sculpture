import ctk_color_picker_widget as color_picker
import customtkinter 
import tkinter as tk
from tkinter import ttk
from tkinter import filedialog
from pathlib import Path
import webbrowser

base_dir = Path(__file__).parent
audio_dir = base_dir / "Audio"
audio_dir.mkdir(exist_ok=True)

class ConfigMenu(customtkinter.CTkFrame):
    def __init__(self, master, response_selection=None, on_save=None):
        super().__init__(master)

        self.response_selection = response_selection
        self.on_save = on_save

        brainwave_bands=("Alpha 8-12 [Hz]", "Beta 12-30 [Hz]", "Delta <4 [Hz]", "Gamma 30-50 [Hz]", "Theta 4-8 [Hz]")
        panels = ("Panel 1", "Panel 2", "Panel 3", "Panel 4", "Panel 5", "Panel 6", "Panel 7", "Panel 8", "Panel 9")

        # -------------------- Brain Wave Band Dropdown Menu --------------------
        list_variable = tk.Variable(value=panels)
        self.freqband_optionmenu = customtkinter.CTkOptionMenu(self, values=brainwave_bands, width=150)
        self.freqband_optionmenu.grid(row=0, column=0, columnspan=2, padx=10, pady=10, sticky="w")

        # -------------------- Dancer 1 Checkbutton --------------------
        self.dancer1_var = customtkinter.StringVar(value="off")
        self.dancer1 = customtkinter.CTkCheckBox(self, text="Dancer 1",
                                                variable= self.dancer1_var, onvalue="on", offvalue="off")
        self.dancer1.grid(row=1, column=0, padx=10, pady=10)

        # -------------------- Dancer 2 Checkbutton --------------------
        self.dancer2_var = customtkinter.StringVar(value="off")
        self.dancer2 = customtkinter.CTkCheckBox(self, text="Dancer 2",
                                                variable= self.dancer2_var, onvalue="on", offvalue="off")
        self.dancer2.grid(row=1, column=1, padx=10, pady=10)

        # -------------------- Audio Response Button --------------------
        self.audio_btn = customtkinter.CTkButton(self, text="Audio", width=75, command=self.open_audio)
        self.audio_btn.grid(row=2, column=0, padx=10, pady=10)

        # -------------------- Panel Dropdown Menu --------------------
        self.panel_optionmenu = customtkinter.CTkOptionMenu(self, values=panels)
        self.panel_optionmenu.grid(row=3, column=0, padx=10, pady=10)

        # -------------------- LED Response Button --------------------
        self.led_btn = customtkinter.CTkButton(self, text="Panel LED", width=75, command=lambda: self.response_selection("led"))
        self.led_btn.grid(row=3, column=1, padx=10, pady=10)

        # -------------------- Motor Response Button --------------------
        self.motor_btn = customtkinter.CTkButton(self, text="Panel Motor", width=75, command=lambda: self.response_selection("motor"))
        self.motor_btn.grid(row=3, column=2, padx=10, pady=10)

        # -------------------- Save to Config Button --------------------
        self.add_btn = customtkinter.CTkButton(self, text="Add to Config", width=75, command=self.get_config)
        self.add_btn.grid(row=4, column=0, padx=10, pady=10)

        # -------------------- Configurations Treeview --------------------
        self.treeview = ttk.Treeview(self, columns=("brainwave","panel","target","led","audio","motor"), show="headings")

        self.treeview.heading("brainwave", text="Brainwave")
        self.treeview.heading("panel", text="Panel")
        self.treeview.heading("target", text="Target")
        self.treeview.heading("led", text="LED")
        self.treeview.heading("audio", text="Audio")
        self.treeview.heading("motor", text="Motor")

        self.treeview.column("brainwave", width=7)
        self.treeview.column("panel", width=3)
        self.treeview.column("target", width=10)
        self.treeview.column("led", width=10)
        self.treeview.column("audio", width=20)
        self.treeview.column("motor", width=20)

        self.treeview.grid(row=5, column=0, columnspan=3, padx=(0,15), pady=0, sticky="nsew")

        self.treeview.bind("<Control-r>", lambda e: self.remove_row())

        self.led_color = None
        self.motor_frame = None
        self.audio_file = None

    def open_audio(self):
        filepath = filedialog.askopenfilename(initialdir=str(audio_dir), title="Select audio file", filetypes=[("Audio Files", "*.mp3 *.wav *.ogg"), ("All Files", "*.*")])
    
        if filepath:
            self.audio_file = filepath
            print("selected Audio: ", filepath)

    def get_config(self):
        if self.dancer1_var.get() == "off" and self.dancer2_var.get() == "off":
            return
        elif self.dancer1_var.get() == "off":
            target = "dancer2"
        elif self.dancer2_var.get() == "off":
            target = "dancer1"
        else:
            target = "sync"

        config = {
            "brainwave_band" : self.freqband_optionmenu.get(),
            "panel" : self.panel_optionmenu.get(),
            "target" : target,
            "modules": {}
        }

        if self.on_save:
            self.on_save(config)

class PanelMenu(customtkinter.CTkFrame):
    def __init__(self, master):
        super().__init__(master)

        self.panel_labels = ["Panel 1", "Panel 2", "Panel 3", "Panel 4", "Panel 5", "Panel 6", "Panel 7", "Panel 8", "Panel 9"]

        self.buttons = []
        self.selected = None

        for i, label in enumerate(self.panel_labels):
            panel_btn = customtkinter.CTkButton(self, text=label, width=75, command=lambda i=i: self.get_panel_select(i))
            panel_btn.pack(pady=5)

            self.buttons.append(panel_btn)

    def get_panel_select(self, index):
        self.selected = index

        for i, btn in enumerate(self.buttons):
            if i == index:
                btn.configure(fg_color=("gray75", "gray25"),hover_color=("gray65", "gray35"))  # selected style
            else:
                btn.configure(fg_color=("#3B8ED0", "#1F6AA5"))  # default style

        print("Selected:", self.panel_labels[index])

class SoundCtrl(customtkinter.CTkFrame):
    def __init__(self,master):
        super().__init__(master)

        self.sound_label = customtkinter.CTkLabel(self, text="Sound control", fg_color="transparent")
        self.sound_label.grid(row=0, column=0, padx=10, pady=10)

        self.stop_btn = customtkinter.CTkButton(self, text="Stop Audio", command=self.button_click)
        self.stop_btn.grid(row=1, column=0, padx=10, pady=10)

    def button_click(self):
        print("button click")

class MotorCtrl(customtkinter.CTkFrame):
    def __init__(self, master, on_run=None, on_emergency=None):
        super().__init__(master)

        self.changed = False

        self.on_run_cb = on_run
        self.on_emergency_cb = on_emergency

        self.running = False
        self.latest_values = None

        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=2)
        self.grid_rowconfigure(2, weight=1)
        self.grid_rowconfigure(3, weight=2)
        self.grid_rowconfigure(4, weight=1)
        self.grid_rowconfigure(5, weight=1)

        self.grid_columnconfigure(0, weight=2, uniform="equal")
        self.grid_columnconfigure(1, weight=1, uniform="equal")

        # -------------------- Acceleration Slider --------------------
        self.accel_label = customtkinter.CTkLabel(self, text="Acceleration:", fg_color="transparent", font=("Open Sans", 14))
        self.accel_label.grid(row=0, column=0, padx=5, pady=0, sticky="sw")
        self.accel_slider = customtkinter.CTkSlider(self, from_=50, to=5000, number_of_steps=2, command=self.on_change)
        self.accel_slider.set(50)
        self.accel_slider.grid(row=1, column=0, columnspan=3, padx=10, pady=0, sticky="ew")

        # -------------------- Speed Slider --------------------
        self.speed_label = customtkinter.CTkLabel(self, text="Speed:", fg_color="transparent", font=("Open Sans", 14))
        self.speed_label.grid(row=2, column=0, padx=5, pady=0, sticky="sw")
        self.speed_slider = customtkinter.CTkSlider(self, from_=50, to=1000, number_of_steps=20, command=self.on_change)
        self.speed_slider.set(50)
        self.speed_slider.grid(row=3, column=0, columnspan=3, padx=10, pady=0, sticky = "ew")

        # -------------------- Direction Switch --------------------
        self.dir_var = customtkinter.StringVar(value=0)
        self.dir_switch = customtkinter.CTkSwitch(self, text="Move: Up/Down",
                                 variable=self.dir_var, onvalue=0, offvalue=1, progress_color="transparent")
        self.dir_switch.select()
        self.dir_switch.grid(row=4, column=0, columnspan=2, padx=(40,0), pady=10, sticky="nsw")
 
        # -------------------- Stop Motor Button --------------------
        self.poll_btn = customtkinter.CTkButton(self, text="Run Motor", width=75, height=25, command=self.toggle_run)
        self.poll_btn.grid(row=4, column=1, padx=(0,23), pady=10, sticky="nswe")

        # -------------------- Emergency Stop Sculpture Button --------------------
        self.estop_btn = customtkinter.CTkButton(self, text="EMERGENCY", width=75, height=25, command=self.emergency_stop)
        self.estop_btn.grid(row=5, column=1, padx=(0,23), pady=10, sticky="nswe")

    def on_change(self, value=None):
        self.changed = True

    def toggle_run(self):
        self.running = not self.running

        if self.running:
            self.poll_btn.configure(text="Stop Motor")
            self.poll_motor()
        else:
            self.poll_btn.configure(text="Run Motor")

            values = {
                "mode" : 1, # motor set to stop
                "acceleration" : int(self.accel_slider.get()),
                "speed" : 400,
                "direction" : self.dir_var.get()
            }

            if self.on_run_cb:
                self.on_run_cb(values)

    def get_values(self):
        return{
            "acceleration" : int(self.accel_slider.get()),
            "speed" : int(self.speed_slider.get()),
            "direction" : self.dir_var.get()
        }

    def poll_motor(self):
        if not self.running:
            return
        
        values = {
            "mode" : 0, # motor set to run
            "acceleration" : int(self.accel_slider.get()),
            "speed" : int(self.speed_slider.get()),
            "direction" : self.dir_var.get()
        }

        if values != self.latest_values:
            self.latest_values = values

            if self.on_run_cb:
                self.on_run_cb(self.latest_values)

        self.after(850, self.poll_motor)

    def emergency_stop(self):
        values = {
            "mode" : 1,
            "acceleration" : 5000,
            "speed" : 400,
            "direction" : self.dir_var.get()
        }

        if self.on_emergency_cb:
            self.on_emergency_cb(values)

class ConfigMode(customtkinter.CTkFrame):
    def __init__(self, master, on_save=None):
        super().__init__(master)

        self.on_save = on_save
        self.led_data = None

        self.grid_columnconfigure(0, weight=1, uniform="equal")
        self.grid_columnconfigure(1, weight=1, uniform="equal")

        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=4)
        
        # -------------------- Left Panel (Config selection) --------------------
        self.freq_label = customtkinter.CTkLabel(self, text="Configuration", fg_color="transparent", font=("Open Sans", 20))
        self.freq_label.grid(row=0, column=0, padx=5, pady=0, sticky="sw")
        self.freq_frame = ConfigMenu(self, response_selection=self.change_frame, on_save=self.add_to_config)
        self.freq_frame.grid(row=1, column=0, padx=5, pady=0, sticky="nsew")

        # -------------------- Right Panel (Response selection) --------------------
        self.led_label = customtkinter.CTkLabel(self, text="LED response", fg_color="transparent", font=("Open Sans", 20))
        self.led_label.grid(row=0, column=1, padx=5, pady=0, sticky="sw")
        self.led_frame = color_picker.CTkColorPicker(self, width=250, command=self.set_led_color)
        self.led_frame.grid(row=1, column=1, padx=5, pady=0, sticky="nsew")

        self.motor_label = customtkinter.CTkLabel(self, text="Motor response", fg_color="transparent", font=("Open Sans", 20))
        self.motor_label.grid(row=0, column=1, padx=5, pady=0, sticky="sw")
        self.motor_frame = MotorCtrl(self)
        self.motor_frame.grid(row=1, column=1, padx=5, pady=0, sticky="nsew")
        self.motor_frame.estop_btn.grid_remove()
        self.motor_frame.poll_btn.grid_remove()

        self.selected_label = self.led_label
        self.selected_frame = self.led_frame
        self.motor_label.grid_remove()
        self.motor_frame.grid_remove()
        
        self.frames = {
            "led": self.led_frame,
            "motor": self.motor_frame
        }

    def set_led_color(self, data):
        color, fade = data
        self.led_data = {
            "color" : color,
            "fade" : fade
        }

    def change_frame(self, mode):
        if self.selected_frame:
            self.selected_label.grid_remove()
            self.selected_frame.grid_remove()

        self.selected_frame = self.frames[mode]

        if mode == "led":
            self.selected_label = self.led_label
        elif mode == "motor":
            self.selected_label = self.motor_label
        
        self.selected_label.grid()
        self.selected_frame.grid()

    def add_to_config(self, base_config):

        if "modules" not in base_config:
            base_config["modules"] = {}

        if self.led_data is not None:
            base_config["modules"]["led"] = {
                "color": self.led_data["color"],
                "fade" : self.led_data["fade"]
            }

        motor_values = self.motor_frame.get_values()
        if self.motor_frame.changed:
            base_config["modules"]["motor"] = motor_values

        audio_file = self.freq_frame.audio_file
        if audio_file:
            base_config["modules"]["audio"] = {
                "audio" : audio_file
            }

        if self.on_save:
         self.on_save(base_config)

class ManualMode(customtkinter.CTkFrame):
    def __init__(self, master, manual=None):
        super().__init__(master)

        self.manual = manual
        self.led_latest_values = None
        self.led_poll_job = None

        self.grid_columnconfigure(0, weight=1, uniform="equal")
        self.grid_columnconfigure(1, weight=2, uniform="equal")
        self.grid_columnconfigure(2, weight=2, uniform="equal")

        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=4)

        # -------------------- Left panel (Panel selection) --------------------
        self.panel_label = customtkinter.CTkLabel(self, text="Panel selection", fg_color="transparent", font=("Open Sans", 20))
        self.panel_label.grid(row=0, column=0, padx=10, pady=0, sticky="sw")
        self.panel_frame = PanelMenu(self)
        self.panel_frame.grid(row=1, column=0, padx=5, pady=0, sticky="nsew")
        
        # -------------------- Middle panel (LED controls) --------------------
        self.led_label = customtkinter.CTkLabel(self, text="LED control", fg_color="transparent", font=("Open Sans", 20))
        self.led_label.grid(row=0, column=1, padx=10, pady=0, sticky="sw")
        self.led_frame = color_picker.CTkColorPicker(self, width=250, command=self.on_led_change)
        self.led_frame.grid(row=1, column=1, padx=5, pady=0, sticky="nsew")

        # -------------------- Right panel (Motor controls) --------------------
        self.motor_label = customtkinter.CTkLabel(self, text="Motor control", fg_color="transparent", font=("Open Sans", 20))
        self.motor_label.grid(row=0, column=2, padx=5, pady=0, sticky="sw")
        self.motor_frame = MotorCtrl(self, on_run=self.send_to_motor, on_emergency=self.handle_emergency)
        self.motor_frame.grid(row=1, column=2, padx=5, pady=0, sticky="nsew")

    def on_led_change(self, data):
        if data == self.led_latest_values:
            return
        
        self.led_latest_values = data

        if self.led_poll_job is not None:
            self.after_cancel(self.led_poll_job)

        self.led_poll_job = self.after(850, lambda: self.send_to_led(data))

    def send_to_led(self, values):
        if self.manual:
            color, fade = values
            payload = {
                "type" : "led",
                "color" : color,
                "fade" : fade
            }
            self.send_uart(payload)

    def send_to_motor(self, values):
        if self.manual:
            values["type"] = "motor"
            self.send_uart(values)
            print("Send", values)

    def handle_emergency(self, values):
        if self.manual:
            self.send_uart(values)
            print("E Send", values)

class TabView(customtkinter.CTkTabview):
    def __init__(self, master, on_save=None, manual=None, **kwargs):
        super().__init__(master, **kwargs)

        self.add("Config Mode")
        self.add("Manual Mode") 
        
        self.set("Config Mode") # opens as default

        self.config_mode_ctrl = ConfigMode(self.tab("Config Mode"), on_save=on_save)
        self.config_mode_ctrl.pack(padx=10, pady=10, fill="both", expand=True)

        self.manual_mode_ctrl = ManualMode(self.tab("Manual Mode"), manual=manual)
        self.manual_mode_ctrl.pack(padx=10, pady=10, fill="both", expand=True)

class App(customtkinter.CTk):
    def __init__(self, on_save=None, manual=None, performance=None):
        super().__init__()

        self.on_save = on_save
        self.manual = manual

        self.performance_controller = performance
        self.performance_running = False

        customtkinter.set_appearance_mode("system")
        self.geometry("900x500")
        self.title("An Immersive Mind-Controlled Kinetic Sculpture Control Interface")

    # -------------------- Menu Bar --------------------
        menubar = tk.Menu(self)

        file_menu = tk.Menu(menubar, tearoff=0)
        file_menu.add_command(label="Open...")
        file_menu.add_command(label="Save")
        file_menu.add_command(label="Save as...")
        menubar.add_cascade(label="File", menu=file_menu)

        options_menu = tk.Menu(menubar, tearoff=0)
        options_menu.add_command(label="Configure Sculpture")
        options_menu.add_command(label="Start Performance", command=self.start_performance)
        menubar.add_cascade(label="Options", menu=options_menu)

        help_menu = tk.Menu(menubar, tearoff=0)
        help_menu.add_command(label="About IUCRC BRAIN Center", command=lambda: webbrowser.open("https://iucrc.nsf.gov/centers/building-reliable-advances-and-innovations-in-neurotechnology/"))
        help_menu.add_command(label="Documentation")
        menubar.add_cascade(label="Help", menu=help_menu)

        self.config(menu=menubar)

    # -------------------- Tab View --------------------
        self.tab_view = TabView(master=self, on_save=self.on_save)
        self.tab_view.pack(expand=True, fill="both")

    def start_performance(self):
        self.performance_running = not self.performance_running

        if self.performance_running:
            print("Performance STARTED")
            self.performance_controller.start()
        else:
            print("Performance STOPPED")
            self.performance_controller.stop()
