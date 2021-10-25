# Sensible
Sensible aims to be the **ultimate ceiling-mounted multisensor**: a Human Presence Detector with additional sensor and actuator functions. The prototype is surface mount but the aim is to consider options for recessed mount for an architectural finish. There are some challenges with audio and temp sensing when recessed into the ceiling.

<img src="https://user-images.githubusercontent.com/7063284/124400552-410a9480-dd1b-11eb-9708-7633617143a6.png" height="160"> <img src="https://user-images.githubusercontent.com/7063284/124400554-4536b200-dd1b-11eb-9c35-615c648f231c.png" height="160"> <img src="https://user-images.githubusercontent.com/7063284/124400537-23d5c600-dd1b-11eb-94f4-8139c80127df.png" height="160"> <img src="https://user-images.githubusercontent.com/7063284/124400960-43bab900-dd1e-11eb-9f59-2d8a473cccd0.png" height="160">
<img src="https://user-images.githubusercontent.com/7063284/124400774-ed00af80-dd1c-11eb-868f-a861e379b630.png" height="130">
<img src="https://user-images.githubusercontent.com/7063284/124400753-c93d6980-dd1c-11eb-80c2-6408d4570f2e.png" height="130">
<img src="https://user-images.githubusercontent.com/7063284/124400791-20433e80-dd1d-11eb-9c62-123b372b53a2.png" height="130">



Description
---
The project comprises
- designs for 3D printed hardware which can be mounted to the ceiling
- designs for a PCB with various sensors / actuators attached
- a custom firmware for ESP32 with basic audio functions such as threshold, calibration, decay, retrigger
- Node-RED flows

Within:
- PIR sensor - bare component (AS312)
- Microwave sensor board (RCWL-0516)
- Temperature / humidity (currently DHT22 as it offers least self-heating and works well inside the housing) 
- Digital lux sensor (TS2561 as it provides the required resolution at very low light levels)
- LED ring for effects (SK6812 strip as I like the option of warm white as well as RGB) 
- Buzzer (for getting someone's attention in the room, or general purpose notification of emergencies etc.)

Project Progress
---
As of 04/07/2021 this is a work in progress. So far we have:
- 3D-printed sensor housing featuring a twist-lock mechanism, concealed LED pixel ring, and adaptable mounting option for baffles to limit the direction and angle of the PIR sensor
- Custom firmware supporting OTA updates, audio calibration initiated over MQTT (period of calibration set over MQTT), remote control of audio threshold, audio decay, and timing for repeat / interim audio trigger messages (configured over MQTT), audio levels reported back every x seconds that audio is above the threshold, and of course sensor data reporting back over MQTT
- Node-RED flows that hook in with the Node-RED lighting system, to allow us to set up instances of sensors, toggle individual sensing types, etc.

Design decisions
---
Wired vs. wireless. My preference has always been for wired ethernet-connected devices in the home. This project has been through a few iterations but have settled on wireless for reasons of available space in the housing. Toyed with the idea of building a board that directly supported W5100 and ATMega328 but keeping it simple to start with and will have a detachable development board. (Also means we can use space under the board as dev board will be mounted on female headers.) Power is provided via 12-24V passive PoE through network cable. The board will have an ethernet jack for ease of connecting power, but IP connection over wireless. Strange combo, but justifiable at least to get started. Ideally this board would be wired with 802.3af PoE, perhaps that's for later as this is basically designing a development board from scratch.

Photos
---
Some photos and screenshots should give a better idea of this:

- 3D-printed mount comes in two sections. This photo shows a blank bottom plate, later versions have cutouts for sensors and ventilation holes for temp / humidity
![image](https://user-images.githubusercontent.com/7063284/124400548-3a7c1d00-dd1b-11eb-83af-9982363ea7dc.png)

- The ring around the PIR is for attaching baffles. The circlular pattern is for airflow for the temp sensor. The small hole is for the lux sensor.
![image](https://user-images.githubusercontent.com/7063284/124400552-410a9480-dd1b-11eb-9708-7633617143a6.png)

- This is version 1 of the board. We have scrapped the ESP01 and ATTiny85 and combined into one MCU, the ESP32. We are still using this ESP32 dev board which is the MH-ET LIVE MiniKit
![image](https://user-images.githubusercontent.com/7063284/124400554-4536b200-dd1b-11eb-9c35-615c648f231c.png)

- An early prototype showing LED ring, looks far more diffused in real life (also this was a test blank bottom plate)
![image](https://user-images.githubusercontent.com/7063284/124400537-23d5c600-dd1b-11eb-94f4-8139c80127df.png)

- Node-RED dashboard showing PIR currently being triggered (countdown starts when trigger ends)
![image](https://user-images.githubusercontent.com/7063284/124400774-ed00af80-dd1c-11eb-868f-a861e379b630.png)

- Node-RED dashboard showing the lights having been triggered by microwave sensor (countdown started - timer is set elsewhere in Node-RED)
![image](https://user-images.githubusercontent.com/7063284/124400753-c93d6980-dd1c-11eb-80c2-6408d4570f2e.png)

- Node-RED dashboard showing the lights not triggered (i.e. lights are off)
![image](https://user-images.githubusercontent.com/7063284/124400791-20433e80-dd1d-11eb-9c62-123b372b53a2.png)

- PCB layout for v1. We'll be keeping the dimensions and placement of some devices, but re-designing a lot of it for version 2.
![image](https://user-images.githubusercontent.com/7063284/124400960-43bab900-dd1e-11eb-9f59-2d8a473cccd0.png)

