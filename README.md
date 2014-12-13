tiltmusic
=========

Tilt based music machine based off Arduino Uno, ITG 3200, Neopixel 8x8 grid, and VS1053

# Libraries

The only library this uses at the time of writing is the neopixel library. This library is included in the repo (and will always be the version we are using). Just copy the `Adafruit_Neopixel` dir to your Arduino libraries and you will be good to go

# Hardware
11 speaker 
05 Neopixel
02 Button
03 Switch
SDA I2C data
SCL I2C clock
Neopixel +5V
Gyro +5V
the 3 GND pins

# Documentation
Due to ease of use most of the information will be in the tonegen file itself

# Download
`> git clone https://github.com/Merglyn/tiltmusic.git`

or use Github's "download zip" functionality (on the right under clone url)

# Install
Just throw everything in your Arduino folder and call it a day.

# Debug info
If the term "DEBUG" is defined a bunch of debug info will be printed to serial
