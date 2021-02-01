# ledmatrix
a small c-project which uses 8x8 matrix led displays connected to max 7219
<br><br>
![Alt text](https://github.com/BernhardR/ledmatrix/blob/main/docu/info.jpg?raw=true "info") 

Installation:

- install linux with gcc support
- install wiringPi
- connect display as follows

If you use AZ-Delivery 4 (8x) Matrix layout you may connect using the colors
- MAX7219_CS0		10 // white (CE0)
- MAX7219_DIN		12 // grey (MOSI)
- MAX7219_CLK		14 // black (SCLK)
<br><br>
![Alt text](https://github.com/BernhardR/ledmatrix/blob/main/docu/wires.jpg?raw=true "wires") 

use 5V external power source on blue wire

Run
- run make
- ./led


