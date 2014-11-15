/* Prototype for the tilt-based MP3 player.
   The functions for this are not filled in
   so it won't do much atm.
*/

/* General library information:
  All of the libraries are in the git repo, just copy them over to your Arduino folder
*/

/* MP3 Shield information
   Open pins:
   D0(UART), D1(UART), D5, D10
   A0-5

   Used pins:
   D2- DREQ (data request)
   D6- CS (chip select)
   D7- DCS (data chip select)
   D8- Mp3 reset
   D9- SDCS (SD chip select)

   Libraries:
   SPI (unused for now, but available)
   SDFat
   SFEMP3Shield (the sparkfun library)

   Resources:
   Library https://github.com/madsci1016/Sparkfun-MP3-Player-Shield-Arduino-Library
   Ref http://mpflaga.github.io/Sparkfun-MP3-Player-Shield-Arduino-Library/class_s_f_e_m_p3_shield.html
   Also remember that you can do some hardcore bit banging if needed, example on Moodle
*/

/* Neopixel Info
  Pin: D5
   
  The Neopixel grid is assigned as follows
 y
 7 [  0  1  2  3  4  5  6  7 ]
 6 [  8  9 10 11 12 13 14 15 ]
 5 [ 16 17 18 19 20 21 22 23 ]
 4 [ 24 25 26 27 28 29 30 31 ]
 3 [ 32 33 34 35 36 37 38 39 ]
 2 [ 40 41 42 43 44 45 46 47 ]
 1 [ 48 49 50 51 52 53 54 55 ]
 0 [ 56 57 58 59 60 61 62 63 ]
      0  1  2  3  4  5  6  7 x

  If you prefer, there is a function to calculate which number led you need
  by calling matrix(xCoord, yCoord). For example, matrix(2,3) = 34. This might
  be useful if you do something with for loops, as you can iterate through a row
  or column with ease.

  Read the docs to figure out how to assign colors to those numbers.
  
  You have 2 values to work with, leftChannel and rightChannel. Both are
  between 0 and 31 (with them spending most of the time between 15-31)
  that are just straight reads off of the left and right channels.

  Resources:
  Library https://github.com/adafruit/Adafruit_NeoPixel

  Guide https://learn.adafruit.com/downloads/pdf/adafruit-neopixel-uberguide.pdf
  Matrix coord system https://learn.adafruit.com/adafruit-gfx-graphics-library/coordinate-system-and-units
*/

/* Gyro Info
  Pins:
  The Gyro uses the 3.3V pin, GND, and the SDA and SCL pins.
  Note: The SDA and SCL pins are NOT labeled on an Uno, but they are there

  You have 3 values to work with here which are, quite simply, xAng, yAng, and zAng
  These are numbers that increment by 1200 for every 90 degrees
  For example, xAng = 1200 means x is tilted by 90 degrees
               xAng = 2400 means x is tilted by 180 degrees
               xAng = 1800 means x is tilted by 135 degrees, etc.

  Z positive is rotating the box (looking at the top) clockwise
  Z negative is rotating the box (looking at the top) counterclockwise
  X positive is rotating the box (looking at the right) clockwise
  X negative is rotating the box (looking at the right) counterclockwise
  Y positive is rotating the box (Looking at the front) clockwise
  Y negative is rotating the box (Looking at the front) counterclockwise

  Play around with different functions in the SFEMP3Shield library and come up with
  something cool!

  Refs
  Guide https://learn.sparkfun.com/tutorials/itg-3200-hookup-guide
  Example https://github.com/sparkfun/ITG-3200_Breakout/tree/master/Firmware/ITG3200_Example
*/

// libraries, using the sparkfun library for the MP3
//SD libs
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>

//Mp3 lib
#include <SFEMP3Shield.h>

//Neopixel lib
#include <Adafruit_NeoPixel.h>

//Gyro lib
#include <Wire.h>

//initialize Neopixels
#define NEOPIN 5
Adafruit_NeoPixel strip = Adafruit_NeoPixel(64, NEOPIN, NEO_GRB + NEO_KHZ800);

//object for sd card and mp3 chip
SdFat sd;
SFEMP3Shield MP3player;
volatile int track = 0;

//stuff for the button
#define BUTPIN 10
int buttonState = 0;

//ints for the audio input, between 0 and 31
byte leftChannel = 0;
byte rightChannel = 0;

//buffer for the millis of the last gyro reading
unsigned long bufferTime = 0;
unsigned long currTime = 0;
unsigned long totTime = 0;

//total angle
long xAng;
long yAng;
long zAng;

//Start gyro defines
const char SMPLRT_DIV= 0x15;
const char DLPF_FS = 0x16;
const char GYRO_XOUT_H = 0x1D;
const char GYRO_XOUT_L = 0x1E;
const char GYRO_YOUT_H = 0x1F;
const char GYRO_YOUT_L = 0x20;
const char GYRO_ZOUT_H = 0x21;
const char GYRO_ZOUT_L = 0x22;
const char DLPF_CFG_0 = (1<<0);
const char DLPF_CFG_1 = (1<<1);
const char DLPF_CFG_2 = (1<<2);
const char DLPF_FS_SEL_0 = (1<<3);
const char DLPF_FS_SEL_1 = (1<<4);
const char itgAddress = 0x69;
//end gyro defines

void setup() {
  //initiate the sd card
  if(!sd.begin(9, SPI_HALF_SPEED)) sd.initErrorHalt();
  if(!sd.chdir("/")) sd.errorHalt("sd.chdir");

  //initiate the mp3player
  MP3player.begin();
  MP3player.setVUmeter(1);

  //normal volume
  MP3player.setVolume(40,40);

  //begin I2C communications and start the gyro
  Wire.begin();
  //Initialize gyro for +/- 2000 deg/sec
  itgWrite(itgAddress, DLPF_FS, (DLPF_FS_SEL_0|DLPF_FS_SEL_1|DLPF_CFG_0));
  //100hz sample rate
  itgWrite(itgAddress, SMPLRT_DIV, 9);

  //set button to input
  pinMode(BUTPIN, INPUT);
}


void loop() {
  /* TODO: get button to advance track*/

  //play the track. If the track is already playing
  //this will do nothing
  MP3player.playTrack(2);

  //Get the db reading from both channels
  union twobyte vu;
  vu.word = MP3player.getVUlevel();
  leftChannel  = vu.byte[1];
  rightChannel = vu.byte[0];
  
  //Get gyro reading
  if (bufferTime != 0){
    //amount of ms since last poll
    currTime = millis();
    totTime = currTime - bufferTime;
    
    //get the new angles
    xAng = angle(GYRO_XOUT_H, GYRO_XOUT_L, -54, xAng);
    yAng = angle(GYRO_YOUT_H, GYRO_YOUT_L, -16, yAng);
    zAng = angle(GYRO_ZOUT_H, GYRO_ZOUT_L, 4, zAng);

    //set bufferTime
    bufferTime = currTime;
  }
  else {
  bufferTime = millis();
  }
 
  //adjust the pitch/volume/etc with the
  //gyro sensor
  gyroAdjust();

  //update the neopixel grid
  updateNeopixels();
}

void gyroAdjust() {
  //adjust pitch/speed/volume based on gyro information.
  //This will be run every time
  //Use the refs I gave you in the header to help

  return;
}

void updateNeopixels() {
  //You have 2 global vars to use here, the left channel
  //and the right channel which will be values from 0 to 31
  //based on the output that is at that time. I did some
  //testing and it seems like 15-31 is used the most
  //Use the Neopixel docs that I gave above to help.

  return;
}

//Start gyro functions
//This function will write a value to a register on the itg-3200.
//Parameters:
// char address: The I2C address of the sensor. For the ITG-3200 breakout the address is 0x69.
// char registerAddress: The address of the register on the sensor that should be written to.
// char data: The value to be written to the specified register.
void itgWrite(char address, char registerAddress, char data)
{
//Initiate a communication sequence with the desired i2c device
Wire.beginTransmission(address);
//Tell the I2C address which register we are writing to
Wire.write(registerAddress);
//Send the value to write to the specified register
Wire.write(data);
//End the communication sequence
Wire.endTransmission();
}

//This function will read the data from a specified register on the ITG-3200 and return the value.
//Parameters:
// char address: The I2C address of the sensor. For the ITG-3200 breakout the address is 0x69.
// char registerAddress: The address of the register on the sensor that should be read
//Return:
// unsigned char: The value currently residing in the specified register
unsigned char itgRead(char address, char registerAddress)
{
//This variable will hold the contents read from the i2c device.
unsigned char data=0;
//Send the register address to be read.
Wire.beginTransmission(address);
//Send the Register Address
Wire.write(registerAddress);
//End the communication sequence.
Wire.endTransmission();
//Ask the I2C device for data
Wire.beginTransmission(address);
Wire.requestFrom(address, 1);
//Wait for a response from the I2C device
if(Wire.available()){
//Save the data sent from the I2C device
data = Wire.read();
}
//End the communication sequence.
Wire.endTransmission();
//Return the data read during the operation
return data;
}

long angle(char addrA, char addrB, char offset, long oldAng){
  int rate = read(addrA, addrB, offset);
  oldAng = oldAng * 1000L;
  long ang = oldAng + (rate*totTime);
  ang = ang/1000L;
  return ang;
}

int read(char addrA, char addrB, char offset)
{
int data=0;
data = itgRead(itgAddress, addrA)<<8;
data |= itgRead(itgAddress, addrB);
data += offset;
return data;
}

int matrix(int x, int y){
  int result = (56 - y*8) + x;
  return result;
}

/* License Info:
   This is licensed under Beerware R42. There is some code from
   Sparkfun's ITG3200 example code in here, so thanks to them for that
*/