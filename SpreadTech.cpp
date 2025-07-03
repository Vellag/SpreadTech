/*
SPREADTECH.CPP
ENGINEERS: Ryan Schrader, Erik Cotten
ORGANIZATION: National University
DEPENDENCIES: wiringPi, ADCDevice, wiringPiDev
Description: Through Raspberry Pi interface, control temperature of applied butter dish through these components:
        Peltier modules: electrothermal heating & cooling of application
        Thermistor: reads temperature of application
        Buttons: Increase or Decrease temperature range
        LCD1602: Temperature + local time + status indicator
        Toggle Switch: Physical ON/OFF control
*/
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <lcd.h>
#include <pcf8574.h>
#include <ADCDevice.hpp>
#include <math.h>
#include <time.h>
#include <softPwm.h>

// Define output pins of LCD screen
#define BASE    64
#define RS      BASE+0
#define RW      BASE+1
#define EN      BASE+2
#define LED     BASE+3
#define D4      BASE+4
#define D5      BASE+5
#define D6      BASE+6
#define D7      BASE+7

// Define GPIO pins
#define lowerRangeButtonPin 4 // Button to lower temp range
#define increaseRangeButtonPin 5 // Button to increase temp range
#define togglePin 2
#define heatingPin 26 //HW PWM pin
#define coolingPin 1 //HW PWM pin

struct {
    bool heating;
    bool cooling;
} peltierStatus;

ADCDevice *adc;
int lcdhd;

// Function Prototypes
void setup(); // new function
void printTime();
void printTemp();
float readTemp();
void toggleOff();
int lcdDetectI2C(int addr);
void rampDownPeltiers(int pin, float tempFloor, float tempCeiling);

int main(void)
{
    adc = new ADCDevice();
    int pcf8574_address;
    float tempFloor = 67.0;
    float heatTemp = tempFloor - 3.0;
    float tempCeiling = 68.0;
    float coolTemp = tempCeiling + 2.0;
    switch(status())
    {
    case -1:
    {
        printf("LCD I2C init failed\n");
        return 0;
    }
    case -2:
    {
        printf("ADC I2C init failed\n");
        return 0;
    }
    default:
        break;            
    }
    peltierStatus.heating = false;
    peltierStatus.cooling = false;
    printf("Program is starting...\n");
    printf("Welcome to SpreadTech...\n");
    lcdPosition(lcdhd, 3, 0);
    lcdPrintf(lcdhd, "SpreadTech");
    delay(3000);
    lcdClear(lcdhd);

    // Ensure Peltier Modules are initialized to OFF state
    softPwmWrite(heatingPin, 0);
    softPwmWrite(coolingPin, 0);
    
    // Main loop
    while(1)
    {
        if(digitalRead(togglePin) == LOW) // Check if switch is toggled ON
        {
            digitalWrite(LED, HIGH); // Turn on LCD backlight
            printTime();
            printTemp();
            float currentTemp = readTemp();
            printf("%02f\n", currentTemp);

            if(digitalRead(lowerRangeButtonPin) == LOW)
            {
                printf("lowering temp range\n");
                if(heatTemp <= 50.0)
                    break;
                tempCeiling--;
                tempFloor--;
                heatTemp--;
                coolTemp--;
            }
            if(digitalRead(increaseRangeButtonPin) == LOW)
            {
                printf("increasing temp range\n");
                if(coolTemp >= 75.0)
                    break;
                tempCeiling++;
                tempFloor++;
                heatTemp++;
                coolTemp++;
            }
            lcdPosition(lcdhd, 8, 1);
            lcdPrintf(lcdhd, "%dF-%dF", tempFloor, tempCeiling);
            if (currentTemp <= heatTemp) // Heat up system if temp is < 63 Fahrenheight
            {
                if (peltierStatus.heating == false)
                {
                    peltierStatus.heating = true;
                    peltierStatus.cooling = false;
                    softPwmWrite(heatingPin, 15);
                    softPwmWrite(coolingPin, 0);
                    printf("heating up\n");
                    lcdPosition(lcdhd, 9, 0);
                    lcdPrintf(lcdhd, "Heating");
                }
            }
            else if (currentTemp >= coolTemp) // Cool down system if temp is > 70 Fahrenheight
            {
                if(peltierStatus.cooling == false)
                {
                    peltierStatus.heating = false;
                    peltierStatus.cooling = true;
                    softPwmWrite(heatingPin, 0);
                    softPwmWrite(coolingPin, 15);
                    printf("cooling down\n");
                    lcdPosition(lcdhd, 9, 0);
                    lcdPrintf(lcdhd, "Cooling");
                }
            }
            else if (currentTemp >= tempFloor && currentTemp <= tempCeiling) // Disable Peltier elements if temp is between 63 & 68 Fahrenheight
            {
                if(peltierStatus.heating == true || peltierStatus.cooling == true)
                {
                    if (peltierStatus.heating == true)
                    {
                        peltierStatus.heating = false;
                        rampDownPeltiers(heatingPin, tempFloor, tempCeiling);
                    }
                    else
                    {
                        peltierStatus.cooling = false;
                        rampDownPeltiers(coolingPin, tempFloor, tempCeiling);
                    }
                    softPwmWrite(heatingPin, 0);
                    softPwmWrite(coolingPin, 0);
                    peltierStatus.heating = false;
                    peltierStatus.cooling = false;
                    printf("Steady State\n");
                    lcdPosition(lcdhd, 15, 0);
                    lcdPutChar(lcdhd, '');
                    lcdPosition(lcdhd, 9, 0);
                    lcdPrintf(lcdhd, "Steady");
                }
            }
        }
        else // If switch is toggled OFF, call toggleOff function
        {
            toggleOff();
            peltierStatus.heating = false;
            peltierStatus.cooling = false;
        }
        delay(500); // Wait 0.5 seconds in between updates
    }

    return 0;
}

int setup()
{
    wiringPiSetup();
    pinMode(ledPin, OUTPUT);
    pinMode(togglePin, INPUT);
    pinMode(lowerRangeButtonPin, INPUT);
    pinMode(increaseRangeButtonPin, INPUT);
    pullUpDnConrol(lowerRangeButtonPin, PUD_UP);
    pullUpDnControl(increaseRangeButtonPin, PUD_UP);
    softPwmCreate(heatingPin, 0, 100);
    softPwmCreate(coolingPin, 0, 100);
     // Initialize LCD I2C communication
    if(lcdDetectI2C(0x27))
        pcf8574_address = 0x27;
    else
    {
        printf("Could not find LCD I2C address, check wiring...\n");
        return -1;
    }
    // Initialize ADC I2C communication
    if(adc->detectI2C(0x4b))
    {
        delete adc;
        adc = new ADS7830();
    }
    else
    {
        printf("Could not find ADC I2C address, check wiring...\n");
        return -2;
    }
    // Initialize LCD display
    pcf8574Setup(BASE, pcf8574_address);
    for(int i = 0; i < 8; i++)
        pinMode(BASE+i, OUTPUT);
    digitalWrite(LED, HIGH);
    digitalWrite(RW, LOW);
    lcdhd = lcdInit(2, 16, 4, RS, EN, D4, D5, D6, D7, 0, 0, 0, 0);
    if(lcdhd == -1)
    {
        printf("lcdInit failed, Exiting...\n");
        return -1;
    }
    return 0
}

// NAME: readTemp
// Parameters: N/A
// Return type: float
// Description: Reads ADC value of Thermistor, converts value into Fahrenheight, then returns calculated value
float readTemp()
{
    int adcValue = adc->analogRead(0);
    float voltage = (float)adcValue / 255.0 * 3.3;
    float Rt = 10 * voltage / (3.3 - voltage);
    float tempK = 1/(1/(273.15 + 25) + log(Rt/10)/3950.0);
    float tempC = tempK - 273.15;
    float tempF = (tempC * 9 / 5) + 32.0; 
    return tempF;
}

// NAME: printTime
// Parameters: N/A
// Return Type: N/A
// Description: Prints system time of Raspberry Pi to LCD screen in 12-hour HH:MM format
void printTime()
{
    time_t rawTime;
    struct tm *timeinfo;
    time(&rawTime);
    timeinfo = localtime(&rawTime);
    char timeBuf[80];
    lcdPosition(lcdhd, 0, 1);
    strftime(timeBuf, sizeof(timeBuf), "%I:%M%p", timeinfo);
    lcdPrintf(lcdhd, "%s", timeBuf);
}

// NAME: printTemp
// Parameters: N/A
// Return type: N/A
// Description: calls readTemp function and prints returned value to LCD screen in format: "Temp:XXF"
void printTemp()
{
    lcdPosition(lcdhd, 0, 0);
    lcdPrintf(lcdhd, "Temp:%02dF", (int)readTemp());
}

// NAME: lcdDetectI2C
// Parameters: addr - type int
// Return type: int
// Description: Initializes I2C communication with device at given address (addr) from wiringPi library
int lcdDetectI2C(int addr)
{
    int _fd = wiringPiI2CSetup(addr);
    if (_fd < 0)
    {
        printf("Error address: 0x%x\n", addr);
        return 0;
    }
    else
    {
        if (wiringPiI2CWrite(_fd, 0) < 0)
        {
            printf("Not found device in address 0x%x\n", addr);
            return 0;
        }
        else
        {
            printf("Found device in address 0x%x\n", addr);
            return 1;
        }
    }
}

// NAME: toggleOff
// Parameters: N/A
// Return type: N/A
// Description: Turns off Peltier modules, status LED, LCD backlight, and clears LCD text
void toggleOff()
{
    softPwmWrite(heatingPin, 0);
    softPwmWrite(coolingPin, 0);
    digitalWrite(LED, LOW);
    lcdClear(lcdhd);
}

// NAME: rampDownPeltiers
// Parameters: pin of type int
// Return type: N/A
// Description: gradually ramps down power to Peltier modules by 2% PWM duty cycle every 20 seconds until fully disabled
//              This will ramp down over a period of 2 minutes (120 seconds)
void rampDownPeltiers(int pin, float tempFloor, float tempCeiling)
{
    for (int i = 12; i > 0; i -= 2)
    {
        int currentTemp = (int)readTemp();
        if (currentTemp <= tempFloor || currentTemp >= tempCeiling)
            return;
        softPwmWrite(pin, i);
        printTemp();
        printf("Current temp: %02f\n", currentTemp);
        delay(20000);
    }
}
