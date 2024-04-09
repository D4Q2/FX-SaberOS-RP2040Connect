![Schematic](https://github.com/D4Q2/FX-SaberOS-RP2040Connect/blob/master/_FinalLightsaberSchematic.png)

# FX-SaberOS (Modified for RP2040)
System code for Arduino based Lightsaber replicas for Raspberry Pi pico-related boards (specifically the RP2040 Connect).

This repository is currently in development. The currently posted builds is mostly functional, but does not contain the final expected functionality. See the dev notes below.

Installation and configuration instructions are available in the Wiki (from the main FX-SaberOS) https://github.com/Protonerd/FX-SaberOS/wiki

For Discussion, FAQ, and Troubleshooting please visit the Facebook at https://www.facebook.com/groups/FXSaberOS/ or the Saber C++ Discord: https://discord.gg/8YhegR6JRr

## Recent Important Changes:
* There are now two clash methods.  The original "POLL" method, and a new "INT" method which is less intensive but requires attaching INT on the MPU6050 to D2 of the controller board.
https://github.com/Protonerd/FX-SaberOS/wiki/Clash-Method

* Several libraries have been updated (I2Cdev, MPU6050, OneButton, DFPlayer).  If you're updating from a previous build, make sure to replace the old libraries with the new ones. In case of compilation errors when using Arduino IDE 2.x: close the IDE, clear the cache (on Windows by default under "C:\Users\\<your_username>\AppData\Local\Temp") and relaunch.

## Dev Notes
* I'm currently working on adding in the LSMD6SOX gyroscope that's builtin to the RP2040 connect for gyroscope functions. Currently you can use an external MPU6050 or wait for the changes and use the onboard sensor.
* There is a problem with RAM usage that is limiting the number of LEDs to 124, so if you're doing a CROSSGUARD saber, I recommend 12 LEDs for the guards and 112 LEDs for the main blade. You can still use 5 inch crossguard blades, just have the LEDs start a little bit into the blade as the base of the blade is covered by the emitter and won't be seen.
* There is a power problem where if the battery isn't FULLY charged, there won't be enough power to run sounds and lights at the same time. Currently you can reduce the brightness of the LEDs (I have them set at 75/255) to help compensate for this. I don't recommend using full brightness anyway as it drains the battery much faster and isn't really necessary.
