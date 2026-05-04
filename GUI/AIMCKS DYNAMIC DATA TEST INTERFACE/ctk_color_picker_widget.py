# CTk Color Picker widget for customtkinter
# Author: Akash Bora (Akascape)

import tkinter
import customtkinter
from PIL import Image, ImageTk
import sys
import os
import math

PATH = os.path.dirname(os.path.realpath(__file__))

class CTkColorPicker(customtkinter.CTkFrame):
    
    def __init__(self,
                 master: any = None,
                 width: int = 300,
                 initial_color: str = None,
                 fg_color: str = None,
                 corner_radius: int = 8,
                 command = None,
                 orientation = "vertical",
                 **slider_kwargs):
    
        super().__init__(master=master, corner_radius=corner_radius)
        
        WIDTH = width if width>=200 else 200
        HEIGHT = WIDTH + 150
        self.image_dimension = int(self._apply_widget_scaling(WIDTH - 100))
        self.target_dimension = int(self._apply_widget_scaling(20))
        self.lift()

        self.after(10)       
        self.default_hex_color = "#ffffff"  
        self.default_rgb = [255, 255, 255]
        self.base_rgb_color = self.default_rgb[:]
        self.rgb_color = self.default_rgb[:]
        
        self.fg_color = self._apply_appearance_mode(self._fg_color) if fg_color is None else fg_color
        self.corner_radius = corner_radius
        
        self.command = command
        
        self.configure(fg_color=self.fg_color)
          
        self.canvas = tkinter.Canvas(self, height=self.image_dimension, width=self.image_dimension, highlightthickness=0, bg=self.fg_color)
        self.canvas.bind("<B1-Motion>", self.on_mouse_drag)

        self.img1 = Image.open(os.path.join(PATH, 'color_wheel.png')).resize((self.image_dimension, self.image_dimension), Image.Resampling.LANCZOS)
        self.img2 = Image.open(os.path.join(PATH, 'target.png')).resize((self.target_dimension, self.target_dimension), Image.Resampling.LANCZOS)

        self.wheel = ImageTk.PhotoImage(self.img1)
        self.target = ImageTk.PhotoImage(self.img2)
        self.target_id = None
        
        self.canvas.create_image(self.image_dimension/2, self.image_dimension/2, image=self.wheel)
        self.set_initial_color(initial_color)
        
        self.brightness_slider_value = customtkinter.IntVar()
        self.brightness_slider_value.set(255)
        
        self.brightness_label = customtkinter.CTkLabel(self, text="Brightness:", fg_color="transparent", font=("Open Sans", 14))
        self.brightness_slider = customtkinter.CTkSlider(master=self, from_=0, to=255, variable=self.brightness_slider_value, number_of_steps=256,
                                              command=lambda x:self.update_colors(), orientation=orientation, **slider_kwargs)
        
        self.fade_slider_value = customtkinter.IntVar()
        self.fade_slider_value.set(60)

        self.fade_label = customtkinter.CTkLabel(self, text="Transition \n time:", fg_color="transparent", font=("Open Sans", 14))
        self.fade_slider = customtkinter.CTkSlider(master=self, from_=1, to=255, variable=self.fade_slider_value, number_of_steps=5, orientation=orientation)

        self.label = customtkinter.CTkLabel(master=self, text="", width=100, height=100, fg_color=self.default_hex_color, wraplength=1, corner_radius=8)
        
        self.text_var = tkinter.StringVar()
        self.text_var.trace_add("write", self.on_entry_change)
        self.entry = customtkinter.CTkEntry(self, textvariable=self.text_var ,placeholder_text="#000000", font=("Open Sans", 18))
        self.entry.bind("<Return>", lambda e: self.on_entry_change())
        
        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=1)
        self.grid_rowconfigure(2, weight=1)

        self.grid_columnconfigure(0, weight=2, uniform="equal")
        self.grid_columnconfigure(1, weight=1, uniform="equal")
        self.grid_columnconfigure(2, weight=1, uniform="equal")

        self.canvas.grid(row=1, column=0, pady=20, padx=(20,0))
        self.brightness_label.grid(row=0, column=1, padx=0, pady=0, sticky="s")
        self.brightness_slider.grid(row=1, column=1, pady=0, padx=(0,10))
        self.fade_label.grid(row=0, column=2, padx=0, pady=0, sticky="s")
        self.fade_slider.grid(row=1, column=2, pady=0, padx=(0,10))
        self.label.grid(row=2, column=0, padx=(15,0), pady=15, sticky="")
        self.entry.grid(row=2, column=1, columnspan=2, pady=10)
            
    def get(self):
        return self.default_hex_color
    
    def destroy(self):
        super().destroy()
        del self.img1
        del self.img2
        del self.wheel
        del self.target
        
    def on_mouse_drag(self, event):
        x = event.x
        y = event.y
        self.canvas.delete("all")
        self.canvas.create_image(self.image_dimension/2, self.image_dimension/2, image=self.wheel)
        
        d_from_center = math.sqrt(((self.image_dimension/2)-x)**2 + ((self.image_dimension/2)-y)**2)
        
        if d_from_center < self.image_dimension/2:
            self.target_x, self.target_y = x, y
        else:
            self.target_x, self.target_y = self.projection_on_circle(x, y, self.image_dimension/2, self.image_dimension/2, self.image_dimension/2 -1)

        self.canvas.create_image(self.target_x, self.target_y, image=self.target)
        
        self.get_target_color()
        self.update_colors()
  
    def get_target_color(self):
        try:
            self.base_rgb_color = list(self.img1.getpixel((self.target_x, self.target_y)))
            
            r = self.rgb_color[0]
            g = self.rgb_color[1]
            b = self.rgb_color[2]    
            self.rgb_color = [r, g, b]
            
        except AttributeError:
            self.rgb_color = self.default_rgb
    
    def update_colors(self):
        brightness = self.brightness_slider_value.get()

        self.get_target_color()

        r = int(self.base_rgb_color[0] * (brightness/255))
        g = int(self.base_rgb_color[1] * (brightness/255))
        b = int(self.base_rgb_color[2] * (brightness/255))
        
        self.rgb_color = [r, g, b]

        self.default_hex_color = "#{:02x}{:02x}{:02x}".format(*self.rgb_color)

        self.label.configure(fg_color=self.default_hex_color)
        self.text_var.set(self.default_hex_color)

        if self.command:
            self.command(self.get_values())
            
    def projection_on_circle(self, point_x, point_y, circle_x, circle_y, radius):
        angle = math.atan2(point_y - circle_y, point_x - circle_x)
        projection_x = circle_x + radius * math.cos(angle)
        projection_y = circle_y + radius * math.sin(angle)

        return projection_x, projection_y
    
    def set_initial_color(self, initial_color):
        # set_initial_color is in beta stage, cannot seek all colors accurately
        
        if initial_color and initial_color.startswith("#"):
            try:
                r,g,b = tuple(int(initial_color.lstrip('#')[i:i+2], 16) for i in (0, 2, 4))
            except ValueError:
                return
            
            self.default_hex_color = initial_color
            for i in range(0, self.image_dimension):
                for j in range(0, self.image_dimension):
                    self.rgb_color = self.img1.getpixel((i, j))
                    if (self.rgb_color[0], self.rgb_color[1], self.rgb_color[2])==(r,g,b):
                        self.canvas.create_image(i, j, image=self.target)
                        self.target_x = i
                        self.target_y = j
                        return
                    
        self.canvas.create_image(self.image_dimension/2, self.image_dimension/2, image=self.target)

    def on_entry_change(self, *args):
        value = self.text_var.get()

        # basic validation for hex color
        if not isinstance(value, str) or not value.startswith("#") or len(value) != 7:
            return

        try:
            r, g, b = tuple(int(value.lstrip('#')[i:i+2], 16) for i in (0, 2, 4))
        except ValueError:
            return

        # update internal color
        self.rgb_color = [r, g, b]
        self.default_hex_color = value

        # update label color
        self.label.configure(fg_color=value)

        # optionally move the picker target (approximate)
        self.set_initial_color(value)

        if self.command:
            self.command(self.get())

    def get_values(self):
        color = self.default_hex_color
        fade = self.fade_slider_value.get()
        return color, fade