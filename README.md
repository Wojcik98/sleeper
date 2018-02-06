# SLEEPER
## Purpose
Sleeper is a device that helps you fall asleep faster by reducing your breathing form 11 to 6 breaths per minute. It lightens and dims the LED so you know when you should inhale and exhale. You can set maximum brightness of LEDs and maximum time the device will be operating. After this time it will shut down every periphery and go to power-down mode to save battery. It can operate on single LiPo cell or two AA batteries or whatever gives the right voltage for ATtiny13.

It is based on the project [FADing](http://www.instructables.com/id/FADing-Fall-Asleep-Device/) but it uses smaller and cheaper ATtiny13.

## Usage
After powering up you can set maximum brightness and time for which the device will be operating. You increase brightness/time with one button and decrease with the second. Third button changes what you will modify at the moment (brightness or time). Maximum brightness is showed on one LED and time is symbolized on the other (brighter means longer). One increase in time is increase of one minute (same for decrease). After about 4 seconds from the last button push the devie will blink three times and go to operating mode. In this mode you can select which LED will be used (during all operating time). Pulsations slowly decrease from 11 to 6 pulses per minute. After the time set earlier device turns off LEDs and go to power-down mode. You can start using it again by pressing any button and then following instructons from the beggining but this time it will remember your previos settings (brightness and time).

## Schematics and PCB
Schematics and PCB are available on [CircuitMaker](https://workspace.circuitmaker.com/Projects/Details/Micha-Wjcik/sleeper).

## Compiling
Program can be compiled with Atmel Studio 7 or other similar tool.

Output file has 1014 bytes which is 99% of ATtiny13 memory.
