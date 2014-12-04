/* Prototype for the tilt-based synth
   The functions for this are not filled in
   so it won't do much atm.
*/

/* General library information:
  All of the libraries are in the git repo, just copy them over to your Arduino folder
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

//libraries
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

//initialize Neopixels
#define NEOPIN 5
Adafruit_NeoPixel strip = Adafruit_NeoPixel(64, NEOPIN, NEO_GRB + NEO_KHZ800);

//stuff for the button
#define STATEPIN 2 //needs interrupt
#define VOICE1PIN -1
#define VOICE2PIN -1//rename for playback functionality/actual pins
#define BEATPIN   -1
int buttonState = 0;
unsigned int v1ButtonState = 0;
unsigned int v2ButtonState = 0;
unsigned int beatButtonState = 0;

//buffer for the millis of the last gyro reading
unsigned long bufferTime = 0;
unsigned long currTime = 0;
unsigned long totTime = 0;

//total angle
long xAng = 0;
long yAng = 0;
long zAng = 0;

//angles in 0-1024
long xAna = 500;
long yAna = 500;
long zAna = 500;

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

//Start global sequencer defines
unsigned int beat;
unsigned int voice1[8] = {0}; // zero is no sound; potentially multiple vars for volume/octave?
unsigned int voice2[8] = {0}; // we could easily write in a prebuilt sequence to start
//unsigned int speed;
volatile boolean PLAYING;
//End state defines

//start synth stuff
#include <avr/io.h>
#include <avr/interrupt.h>

uint16_t syncPhaseAcc;
uint16_t syncPhaseInc;
uint16_t grainPhaseAcc;
uint16_t grainPhaseInc;
uint16_t grainAmp;
uint8_t grainDecay;
uint16_t grain2PhaseAcc;
uint16_t grain2PhaseInc;
uint16_t grain2Amp;
uint8_t grain2Decay;

// Changing these will also requires rewriting audioOn()
#define PWM_PIN       3
#define PWM_VALUE     OCR2B
#define LED_PIN       13
#define LED_PORT      PORTB
#define LED_BIT       5
#define PWM_INTERRUPT TIMER2_OVF_vect

// Smooth logarithmic mapping
//
uint16_t antilogTable[] = {
  64830,64132,63441,62757,62081,61413,60751,60097,59449,58809,58176,57549,56929,56316,55709,55109,
  54515,53928,53347,52773,52204,51642,51085,50535,49991,49452,48920,48393,47871,47356,46846,46341,
  45842,45348,44859,44376,43898,43425,42958,42495,42037,41584,41136,40693,40255,39821,39392,38968,
  38548,38133,37722,37316,36914,36516,36123,35734,35349,34968,34591,34219,33850,33486,33125,32768
};
uint16_t mapPhaseInc(uint16_t input) {
  return (antilogTable[input & 0x3f]) >> (input >> 6);
}

// Stepped chromatic mapping
//
uint16_t midiTable[] = {
  17,18,19,20,22,23,24,26,27,29,31,32,34,36,38,41,43,46,48,51,54,58,61,65,69,73,
  77,82,86,92,97,103,109,115,122,129,137,145,154,163,173,183,194,206,218,231,
  244,259,274,291,308,326,346,366,388,411,435,461,489,518,549,581,616,652,691,
  732,776,822,871,923,978,1036,1097,1163,1232,1305,1383,1465,1552,1644,1742,
  1845,1955,2071,2195,2325,2463,2610,2765,2930,3104,3288,3484,3691,3910,4143,
  4389,4650,4927,5220,5530,5859,6207,6577,6968,7382,7821,8286,8779,9301,9854,
  10440,11060,11718,12415,13153,13935,14764,15642,16572,17557,18601,19708,20879,
  22121,23436,24830,26306
};
uint16_t mapMidi(uint16_t input) {
  return (midiTable[(1023-input) >> 3]);
}

// Stepped Pentatonic mapping
//
uint16_t pentatonicTable[54] = {
  0,19,22,26,29,32,38,43,51,58,65,77,86,103,115,129,154,173,206,231,259,308,346,
  411,461,518,616,691,822,923,1036,1232,1383,1644,1845,2071,2463,2765,3288,
  3691,4143,4927,5530,6577,7382,8286,9854,11060,13153,14764,16572,19708,22121,26306
};

uint16_t mapPentatonic(uint16_t input) {
  uint8_t value = (1023-input) / (1024/53);
  return (pentatonicTable[value]);
}


void audioOn() {
  // Set up PWM to 31.25kHz, phase accurate
  TCCR2A = _BV(COM2B1) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  TIMSK2 = _BV(TOIE2);
}
//end synth stuff

void setup() {

  Serial.begin(9600);
  //initialize the neopixels
  strip.begin();
  strip.show();

  //begin I2C communications and start the gyro
  Wire.begin();
  //Initialize gyro for +/- 2000 deg/sec
  itgWrite(itgAddress, DLPF_FS, (DLPF_FS_SEL_0|DLPF_FS_SEL_1|DLPF_CFG_0));
  //100hz sample rate
  itgWrite(itgAddress, SMPLRT_DIV, 9);

  //state pin and interrupt
  pinMode(STATEPIN, INPUT);
  attachInterrupt(0, updateState, FALLING);
  
   //Extra Button Setup
  pinMode(VOICE1PIN, INPUT);
  pinMode(VOICE2PIN, INPUT);
  pinMode(BEATPIN, INPUT);
  
  //set button/audio to input
  pinMode(PWM_PIN,OUTPUT);
  audioOn();
  pinMode(LED_PIN,OUTPUT);
}


void loop() {
  /* TODO: get button to change stuff */
  
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
  
  //fix angles
  xAng = fixAngle(xAng);
  yAng = fixAngle(yAng);
  zAng = fixAngle(zAng);

  //set angles to more sensible numbers
  xAna = setAna(xAng);
  yAna = setAna(yAng);
  zAna = setAna(zAng);

  //read button states;
  v1ButtonState = digitalRead(VOICE1PIN);
  v2ButtonState = digitalRead(VOICE2PIN);
  beatButtonState = digitalRead(BEATPIN);  
  
  switch(PLAYING) {
    case true  : {
      playbackNeopixels();
      //playback speed/timing logic: every n cycles/millis?
      //updateTempo(); //maybe?
    };
    case false  : { //in editing mode
      if (beatButtonState == HIGH) updateBeat();
      else if (v1ButtonState == HIGH || v2ButtonState == HIGH) 
        updateSequence();
      editorNeopixels(); //is led state persistent?
    };
  }
  
  //synth update
  updateSynth();
  
  //display all visualization changes
  strip.show();
}

void updateState() {
  //neeed to debounce?
  PLAYING = !PLAYING;
  return; 
}

void updateBeat() {
  //update sequencer position
  beat = xAna / 128;//add scaling
  return; 
}

void updateSequence() {
  if (v1ButtonState == HIGH) {
    voice1[beat] = xAna / 128; //needs scaling
    //voice1vol[beat] = yAna
  }
  
  if (v2ButtonState == HIGH) {
    voice2[beat] = xAna / 128; //needs scaling
  }
  
  return; 
}

void editorNeopixels() {
  //updates all neopixel grid for edit view
  //if slow will optimize for pins that are already set correctly
  strip.clear(); //clear screen
  for (int i = 0; i < 8; i++) strip.setPixelColor(matrix(beat, i), (50, 50, 50)); //highlight working row
  if (voice1[beat] > 0 && voice1[beat] < 9)
    strip.setPixelColor(matrix(beat, voice1[beat] - 1), {0, 255, 255});
  if (voice2[beat] > 0 && voice2[beat] < 9)
    strip.setPixelColor(matrix(beat, voice2[beat] - 1), {255, 192, 255});
  return;
}

void playbackNeopixels() {
  //You have 2 global vars to use here, the left channel
  //and the right channel which will be values from 0 to 31
  //based on the output that is at that time. I did some
  //testing and it seems like 15-31 is used the most
  //Use the Neopixel docs that I gave above to help.

  //playback visualizations go here
  return;
}

//Start synth stuff
long fixAngle(long Ang){
  while (Ang > 2800){
    Ang = Ang - 5600;
  }
  while (Ang < -2800){
    Ang = Ang + 5600;
  }

  return Ang;
}

long setAna(long Ang){
  long Ana = Ang + 2800;
  Ana = Ana/175L;
  Ana = Ana*32L;

  Serial.println(Ana);

  return Ana;
}

void updateSynth(){
  // The loop is pretty simple - it just updates the parameters for the oscillators.
  //
  // Avoid using any functions that make extensive use of interrupts, or turn interrupts off.
  // They will cause clicks and poops in the audio.
  
  // Smooth frequency mapping
  //syncPhaseInc = mapPhaseInc(analogRead(SYNC_CONTROL)) / 4;
  
  // Stepped mapping to MIDI notes: C, Db, D, Eb, E, F...
  //syncPhaseInc = mapMidi(analogRead(SYNC_CONTROL));
  
  // Stepped pentatonic mapping: D, E, G, A, B
  
  
  //you should reference global sequencer vars beat, voice1 and voice2 to play notes -typo
  //dont want the digital read below but I dont want to break existing functionality
  
  
  syncPhaseInc = mapPentatonic(zAna);

  buttonState = digitalRead(STATEPIN);
  if (buttonState == HIGH){
    grainPhaseInc  = mapPhaseInc(yAna) / 2;
    grainDecay     = analogRead(xAna) / 8;  
  }
  else{
    grain2PhaseInc = mapPhaseInc(yAna) / 2; //second voice or synth effect?
    grain2Decay    = analogRead(xAna) / 4;
  }

}

SIGNAL(PWM_INTERRUPT)
{
  uint8_t value;
  uint16_t output;

  syncPhaseAcc += syncPhaseInc;
  if (syncPhaseAcc < syncPhaseInc) {
    // Time to start the next grain
    grainPhaseAcc = 0;
    grainAmp = 0x7fff;
    grain2PhaseAcc = 0;
    grain2Amp = 0x7fff;
    LED_PORT ^= 1 << LED_BIT; // Faster than using digitalWrite
  }
  
  // Increment the phase of the grain oscillators
  grainPhaseAcc += grainPhaseInc;
  grain2PhaseAcc += grain2PhaseInc;

  // Convert phase into a triangle wave
  value = (grainPhaseAcc >> 7) & 0xff;
  if (grainPhaseAcc & 0x8000) value = ~value;
  // Multiply by current grain amplitude to get sample
  output = value * (grainAmp >> 8);

  // Repeat for second grain
  value = (grain2PhaseAcc >> 7) & 0xff;
  if (grain2PhaseAcc & 0x8000) value = ~value;
  output += value * (grain2Amp >> 8);

  // Make the grain amplitudes decay by a factor every sample (exponential decay)
  grainAmp -= (grainAmp >> 8) * grainDecay;
  grain2Amp -= (grain2Amp >> 8) * grain2Decay;

  // Scale output to the available range, clipping if necessary
  output >>= 9;
  if (output > 255) output = 255;

  // Output to PWM (this is faster than using analogWrite)  
  PWM_VALUE = output;
}
//end synth stuff

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
   Sparkfun's ITG3200 example code in here, as well as the APC
   code from Beavis Audio Research
*/
