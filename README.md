# ESP-WROOM-32 Documentation for An Immersive Mind-Controlled Kinetic Sculpture

	Acknowledgment:

		Previous Team (https://github.com/Neural-Kinetic-Sculpture)

	Table of Contents:

		ESP-IDF Visual Studio code installation and setup guide
		Project licenses
		Program architecture
		References
		Notes
	
# ESP-IDF Visual Studio code installation and setup guide
Last update: 2/12/2026

	To install the ESP32 extension for Visual Studio Code follow the guide provided below:

		(https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html)
		# Use along side this documentation		

		Select download server: GitHub
		Select ESP-IDF version: Latest release
		Enter ESP-IDF container directory: Leave default
		Enter ESP-IDF Tools directory (IDF_TOOLS_PATH): Leave default
		Select Python version: Leave default

	To start a new project:

		Go to view > command palette in VS Code and type "ESP-IDF: New Project"

		Choose ESP-IDF Target (IDF_TARGET): ESP32
		Choose ESP-IDF Board: Custom Board
		Choose serial port: no port

		OpenOCD Configuration files (Relative paths to OPENOCD_SCRIPTS): Leave as is
		# USB-to-UART is used instead of JTAG

		Add your ESP-IDF Component directory: Should be left blank 
		# Unless you have additional external libraries to include

		The example template:
			
			Go to ESP-IDF > peripherals > rmt > led_strip
		
		Create Project
	
	To build project:
		
		In VS code go to View > Command Palette and type "ESP-IDF: Build your Project"

	To flash device:
		
		In VS code go to View > Command Palette and type "ESP-IDF: Select Port to Use": Select COM Number

		Go to View > Command Palette and type "ESP-IDF: Flash your Project": Select UART
	
# Project licenses
Last update: 2/12/2026

	Control application uses CustomTkinter by Tom Schimansky (MIT License)
				 CTkColorPicker by Akascape (CC0 1.0 Universal).

	ESP32 Transmitter and Receiver bases code from examples by 
	Espressif Systems (Shanghai) CO LTD, 2021-2022, licensed under Unlicense or CC0-1.0.

	Signal Processing Module by previous team (https://github.com/Neural-Kinetic-Sculpture)

# Program architecture
Last update: 5/1/2026

	Control Application:

		User sets manual input -> Transmit panel data to transmitter ESP32

		User sets preset config -> Read EEG data -> Determine input from config -> Transmit audio to theater audio system
											-> Transmit LED/motor to transmitter ESP32

	Transmitter ESP32:

		Receive data from UART -> Determine panel -> Transmit data to panel receiver ESP32

	Receiver ESP32:

		Receive data from ESPNOW -> send data to queue -> pull data from queue -> control panel LED/Motor
 
# References
Last update: 2/12/2026

	Control Application:

		https://github.com/TomSchimansky/CustomTkinter
		https://github.com/Akascape/CTkColorPicker/blob/main/CTkColorPicker/ctk_color_picker_widget.py

	References:

		Espressif Systems API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html)

	Logging:
	
		Logging API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/log.html)

	Wired communication:

		UART API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html)

	Wireless communication:

		WIFI API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_wifi.html)

		ESPNOW API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html)
		
		MAC addresses:
			ESP 1 - 3c:8a:1f:76:a3:e8
			ESP 2 - 3c:8a:1f:7f:35:54
			ESP 3 - 3c:8a:1f:76:de:44
			ESP 4 - 5c:01:3b:73:6c:0c
			ESP 5 - 3c:8a:1f:77:8f:b0
			ESP 6 - 3c:8a:1f:a0:e5:74
			ESP 7 - 3c:8a:1f:77:8c:ac
			ESP 8 - 3c:8a:1f:77:2c:84
			ESP 9 - 3c:8a:1f:7e:34:bc
			ESP 10 - 3c:8a:1f:77:8c:00

	LED:

		Smooth step function (https://en.wikipedia.org/wiki/Smoothstep)

		RMT API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/rmt.html#api-reference)

	Motor: 

		Motion Profile explanation (https://www.pmdcorp.com/resources/type/articles/get/mathematics-of-motion-control-profiles-article)

		GPIO API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html)

		LEDC API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/ledc.html)

		ESP TIMER API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/esp_timer.html)

		FreeRTOS TASK API reference (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html#tasks)

		FreeRTOS QUEUE API reference (https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-reference/system/freertos.html#queue-api)

		formula: freq = (steps/rev) * (rpm) / 60

				Motor setup limits
		+------------+-----------+--------------+-------------+
		| Micro step | steps/rev |   freq [Hz]  | speed [rpm] |
		|   factor   |           | @ 1200 [rpm] | @ 200 [kHz] |
		+------------+-----------+--------------+-------------+
		|      1     |    200    |     4000     |    60000    |
		+------------+-----------+--------------+-------------+
		|      2     |    400    |     8000     |    30000    |
		+------------+-----------+--------------+-------------+
		|      4     |    800    |     16000    |    15000    |
		+------------+-----------+--------------+-------------+
		|      8     |    1600   |     32000    |     7500    |
		+------------+-----------+--------------+-------------+
		|     16     |    3200   |     64000    |     3750    |
		+------------+-----------+--------------+-------------+
		|     32     |    6400   |    128000    |     1875    |
		+------------+-----------+--------------+-------------+
		|     64     |   12800   |    256000    |    937.5    |
		+------------+-----------+--------------+-------------+
		|     128    |   25600   |    512000    |    468.75   |
		+------------+-----------+--------------+-------------+
		|      5     |    1000   |     20000    |    12000    |
		+------------+-----------+--------------+-------------+
		|     10     |    2000   |     40000    |     6000    |
		+------------+-----------+--------------+-------------+
		|     20     |    4000   |     80000    |     3000    |
		+------------+-----------+--------------+-------------+
		|     25     |    5000   |    100000    |     2400    |
		+------------+-----------+--------------+-------------+
		|     40     |    8000   |    160000    |     1500    |
		+------------+-----------+--------------+-------------+
		|     50     |   10000   |    200000    |     1200    |
		+------------+-----------+--------------+-------------+
		|     100    |   20000   |    400000    |     600     |
		+------------+-----------+--------------+-------------+
		|     125    |   25000   |    500000    |     480     |
		+------------+-----------+--------------+-------------+

# Notes
Last update: 5/1/2026

	Tested stepper motor up to 1200 [rpm] at a micro step setting of 2000. The Stepper motor achieved this speed without the panel attached.
	
	Stepper motor was left running for approximately 12 minutes, with movements simulating finalized usage, and the motor reached temperatures of approximately 140 Fahrenheit.  

	The LED strip was able to be changed while stepper motor is actively moving.

	There was an error on the ESP32 receiver side where the received motor data had an extra byte of data. This error was somehow fixed but unknown how, some alterations were made to the ESP32 transmitter code but eventually were reverted and the issue resolved itself.

	Previous attempts to control the stepper motor used the Espressif RMT, GPTimer, and MCPWM API. The issue with RMT is that the RMT looping feature did not work and not using that feature increased load on CPU. Most likely cause was either provided RMT encoder from Espressif not enabling the looping feature or the ESP-WROOM-32 modules not supporting looping because they are outdated. GPTimer could not be correctly implemented to toggle the GPIO for the required frequencies. It is possible that RMT could be used to control the stepper motor. MCPWM required a lot more setup to get correctly working. It is possible that MCPWM could be used to control the stepper motor. 

	The current method to control the stepper motor used the Espressif LEDC API. This method is easy to setup, use, and fulfills current requirements. It is recommended to continue usage of LEDC until a issue appears to continue exploring GPTimer and MCPWM along with other possible methods. 

	This design for the sculpture should allow for greater modularity as the use of an Arduino to control the stepper motors is no longer needed, the ESP32 controlling both the LED strip and the stepper motor just leaves the power wires to run between panels. It should also allow for more real time responses as the control application has been moved to the same computer as the signal processing module, removing the latency of wireless communication between the signal processing module on the computer to the control application on a mobile device. It should also be a more stable low latency wireless connection using ESPNOW between a transmitter and receiver ESP32 than between the mobile device and the receiver ESP32.

	Overall this should be a good platform for future teams to further develop the project but work is still required. For example a pairing mode to connect the ESP32 transmitter to the ESP32 Receiver without requiring hardcoding of the mac address and reflashing should be developed. The length of wire, if that is still a known parameter in the motor code, and the micro step factor should also be changeable without reflashing. The control application requires further development and testing of the LED, motor, and audio systems along with refinement of the GUI to make it intuitive. It has also been recommended by IAB board judges to include a fail safe to detect if the stepper motor is stalling or rewinding into the panel by measuring the current or back EMF. 
