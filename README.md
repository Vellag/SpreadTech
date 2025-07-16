# SpreadTech
Temperature-Controlled Butter Dish, written in C/C++ primarily using the wiringPi library

**FUNCTIONALITY**
With this being a relatively passive application designed for kitchen counter use, this system will largely perform with no user input required, aside from 2 buttons to control the temperature range, and a toggle switch to turn the system on/off.
Under normal conditions, the system will attempt to heat until reaching between 65F - 68F if below that threshold, and attempt to cool to that same range if above that threshold.
The heating elements are given a PWM duty cycle of 15% during heating or cooling operations, once a temperature within 2 degrees of the range is read, the duty cycle is then gradually reduced until 0% to prevent heat-soaking.
The LCD screen is used for status indicators, showing current temperature, temperature range, heating/cooling/steady, and current local time.

To build the system yourself, here's what will be needed in terms of hardware, external libraries, and the circuit diagram.

**HARDWARE REQUIRED**
- Raspberry Pi 4b
- GPIO extension board + ribbon cable
- ADS7830 ADC
- LCD1602 I2C LCD Screen
- Breadboard
- Push Button x2
- Toggle Switch
- BTS7960 H-Bridge motor driver
- Tec1-12706 Peltier Module x3
- 40mm 12V DC fan x3
- NTC Thermistor
- 12V 6A Power Supply
- 10k $\Omega$ Resistor x6

**EXTERNAL LIBRARIES**
- WiringPi
- ADCDevice
- WiringPiDev (part of WiringPi)

**To Build Program:** Run ```build.sh``` to install external libraries, then run newly-made executable ```SpreadTech.exe```

**CIRCUIT DIAGRAM**
<img width="872" height="870" alt="image" src="https://github.com/user-attachments/assets/31e8828a-e595-471d-83bc-0c171b971b9c" />
