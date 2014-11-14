/* Prototype for the tilt-based MP3 player.
   The functions for this are not filled in
   so it won't do much atm.
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
   
   You guys might have to do a little research on this.
   We are using the 8x8 Neopixel grid for this, and all
   of the resources I found are below

   Resources:
   Library https://github.com/adafruit/Adafruit_NeoPixel

   Matrix Libraries
   https://github.com/adafruit/Adafruit_NeoMatrix
   https://github.com/adafruit/Adafruit-GFX-Library

   Guide https://learn.adafruit.com/downloads/pdf/adafruit-neopixel-uberguide.pdf
   Matrix coord system https://learn.adafruit.com/adafruit-gfx-graphics-library/coordinate-system-and-units
*/

/* Gyro Info
  Pins:
  The Gyro uses the 3.3V pin, GND, and the SDA and SCL pins.
  Note: The SDA and SCL pins are NOT labeled on an Uno, but they are there
  
  
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

//ints for the audio input, between 0 and 31
byte leftChannel = 0;
byte rightChannel = 0;

//buffer for the millis of the last gyro reading
unsigned long bufferTime = 0;
unsigned long currTime = 0;
unsigned long totTime = 0;

//rates for gyro readings
long xRate;
long yRate;
long zRate;

//Start gyro defines
char SMPLRT_DIV= 0x15;
char DLPF_FS = 0x16;
char GYRO_XOUT_H = 0x1D;
char GYRO_XOUT_L = 0x1E;
char GYRO_YOUT_H = 0x1F;
char GYRO_YOUT_L = 0x20;
char GYRO_ZOUT_H = 0x21;
char GYRO_ZOUT_L = 0x22;
char DLPF_CFG_0 = (1<<0);
char DLPF_CFG_1 = (1<<1);
char DLPF_CFG_2 = (1<<2);
char DLPF_FS_SEL_0 = (1<<3);
char DLPF_FS_SEL_1 = (1<<4);
char itgAddress = 0x69;
//end gyro defines

//int 
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

}


void loop() {
  //play the track. If the track is already playing
  //this will do nothing
  MP3player.playTrack("track001.mp3");
  
  //Get the db reading from both channels
  union twobyte vu;
  vu.word = MP3player.getVUlevel();
  leftChannel  = vu.byte[1];
  rightChannel = vu.byte[0];
  
  //Get gyro reading
  if (bufferTime != 0){
    //amount of ms since last poll
    currTime = millis();
    totTime = currtime - bufferTime;

    //rates are in deg/ms
    xRate = readX();
    yRate = readY();
    zRate = readZ();

    //total angle change *1000
    
    /*
    TODO
    */
    
    
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

}

void Neopixel() {
  //You have 2 global vars to use here, the left channel
  //and the right channel which will be values from 0 to 31
  //based on the output that is at that time. I did some
  //testing and it seems like 15-31 is used the most
  //Use the Neopixel docs that I gave above to help.

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

long readX(void)
{
long data=0;
data = itgRead(itgAddress, GYRO_XOUT_H)<<8;
data |= itgRead(itgAddress, GYRO_XOUT_L);
data = (data/512)*15625;
return data;
}

long readY(void)
{
long data=0;
data = itgRead(itgAddress, GYRO_YOUT_H)<<8;
data |= itgRead(itgAddress, GYRO_YOUT_L);
data = (data/512)*15625;
return data;
}

long readZ(void)
{
long data=0;
data = itgRead(itgAddress, GYRO_ZOUT_H)<<8;
data |= itgRead(itgAddress, GYRO_ZOUT_L);
data = (data/512)*15625;
return data;
}
//end gyro functions
