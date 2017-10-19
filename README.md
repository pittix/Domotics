# Domotics
The project involves the "arduino MCU", the **ATMega 328p**, some low-power wifi chip, the ESP8266-12E and the core controller, the RaspberryPi 1 model B (any computer can do the trick though).
The project has a tree structure: the raspberry receives request periodically from the ESP via WiFi: the requests involves
- Data saving (e.g. temperature measured in a room and battery status of the sensor)
- Data gaining: the arduinos may be interested in the status of the home (e.g. if the heating should be on or off, or if there's someone in the house)
##RoadMap
- [x] Done the arduino part for heating
- [ ] For the light control, there's an interrupt-wake needed: the sensor sleeps most of the time except if there's an incoming I2C transmission or if the light must be turned on.
- [ ] The ESP must be programmed to be updated via OTA and to be able to update the arduinos if necessary
- [ ] The raspberry code must be implemented: 
    -[ ] 
