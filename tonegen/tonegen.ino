// Prototype for the tilt-based synth

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

//Debug options. When DEBUG is not commented out
//it prints lots of info about whats going on to
//the serial monitor. Uncommenting TONEON
//will disable the button, and commenting out ENABLENEO
//will turn off the grid.

#define DEBUG true
//#define TONEON true
#define ENABLENEO true
#define ZELDA true
#define GAME true

//libraries
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

//initialize Neopixels
#define NEOPIN 5
#define ONBRIGHT 50
Adafruit_NeoPixel strip = Adafruit_NeoPixel(64, NEOPIN, NEO_GRB + NEO_KHZ800);
const int neoArray[26] = {2,3,4,5,9,24,26,23,24,26,29,31,32,39,40,42,45,47,49,51,52,54,58,59,60,61};

//stuff for the button
#define BUTPIN 2
int buttonState = 0;
int offButtonState = 0;
boolean offPrevState = true;
int mode = 0;
const int modes = 3;

//switch
#define SWPIN 3
int lowPower = 0;

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

//synth vars
#define SPEAKERPIN 11
volatile unsigned int notefreq = 0;

//pentatonic CDEGA C1->A6
const unsigned int pentatonic[] = {
      33,   37,   41,   49,   55, 
      65,  147,  165,  196,  220,
     262,  294,  330,  392,  440,
     523,  587,  659,  784,  880,
    1047, 1175, 1319, 1568, 1760,
};

//all notes C1-> B6
const unsigned int notes[] = {
      33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62,
      65,   69,   73,   78,   82,   87,   93,   98,  104,  110,  117,  123,
     131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247,
     262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
     523,  445,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976
};

//indices of the in pentatonic in the full note list
const int pentind[] = {
     0,  2,  4,  7,  9,
    12, 14, 16, 19, 21,
    24, 26, 28, 31, 33,
    36, 38, 40, 43, 45,
    48, 50, 52, 55, 57
};

#ifdef ZELDA
  const int noteZelda[] = {8, 2, 2, 2, 2,
                           1, 1, 14,
                           8, 2, 2, 2, 2,
                           1, 1, 14,
                           4, 4, 2, 2, 1, 1, 1, 1,
                           8, 2, 2, 1, 1, 2,
                           8, 2, 2, 1, 1, 2,
                           3, 1, 8, 4,
                           3, 1, 4, 4, 2, 2,
                           3, 1, 4, 4, 2, 2,
                           3, 1, 8, 4,
                           2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 2,
                           4, 4, 2, 2, 1, 1, 1, 1,
                           8, 2, 2, 1, 1, 2,
                           12, 4,
                           4, 8, 4,
                           12, 4,
                           4, 8, 4,
                           12, 4,
                           4, 8, 4,
                           12, 4,
                           4, 8, 4,
                           3, 1, 8, 4,
                           2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 2};
  int zeldaLen = 115;
  const int freqZelda[] = {466, 466, 349, 349, 466,
                           415, 370, 415,
                           466, 466, 349, 349, 466,
                           440, 392, 415,
                           466, 349, 349, 466, 466, 523, 597, 622,
                           698, 698, 698, 698, 740, 831,
                           932, 932, 932, 932, 831, 740,
                           831, 740, 698, 698,
                           622, 698, 740, 740, 698, 622,
                           554, 622, 698, 698, 622, 554,
                           523, 597, 659, 784,
                           698, 349, 349, 349, 349, 349, 349, 349, 349, 349, 349,
                           466, 349, 349, 466, 466, 523, 597, 622,
                           698, 698, 698, 698, 740, 831,
                           932, 1109, 1047, 880, 698,
                           740, 932,
                           880, 698, 698,
                           740, 932,
                           880, 698, 597,
                           622, 740,
                           698, 554, 466,
                           523, 597, 659, 740,
                           698, 349, 349, 349, 349, 349, 349, 349, 349, 349, 349};
#endif

#ifdef GAME
  byte pixel[] = {
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 1
  };

  byte prevPixel[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
  };

  byte capn[] = {
    0, 7, 8, 15, 16, 23, 24,
    31, 32, 39, 40, 47, 48,
    55, 56, 63
  };

  byte diff = 0;
  byte count = 0;

  byte location = 4;
  boolean bad = false;
#endif

void setup() {
  //initialize the neopixels
  strip.begin();
  strip.show();

  //begin I2C communications and start the gyro
  Wire.begin();
  //Initialize gyro for +/- 2000 deg/sec
  itgWrite(itgAddress, DLPF_FS, (DLPF_FS_SEL_0|DLPF_FS_SEL_1|DLPF_CFG_0));
  //100hz sample rate
  itgWrite(itgAddress, SMPLRT_DIV, 9);
  #ifdef DEBUG
    Serial.begin(9600);
  #endif
  //set button/audio to input
  pinMode(BUTPIN, INPUT);
  pinMode(SWPIN, INPUT);
}


void loop() {
  lowPower = digitalRead(SWPIN);
  if (lowPower == 1 && mode == 0){
    strip.setBrightness(ONBRIGHT);
    //Get gyro reading
    if (bufferTime != 0){
      //amount of ms since last poll
      currTime = millis();
      totTime = currTime - bufferTime;
    
      //get the new angles
      #ifdef DEBUG
        Serial.print("X: ");
      #endif
      xAng = angle(GYRO_XOUT_H, GYRO_XOUT_L, -59, xAng);
      #ifdef DEBUG
        Serial.print(" Y: ");
      #endif
      yAng = angle(GYRO_YOUT_H, GYRO_YOUT_L, -12, yAng);
      #ifdef DEBUG
        Serial.print(" Z: ");
      #endif
      zAng = angle(GYRO_ZOUT_H, GYRO_ZOUT_L, 6, zAng);
      #ifdef DEBUG
        Serial.print(" |  ");
      #endif

      //set bufferTime
      bufferTime = currTime;
    }
    else {
      bufferTime = millis();
    }

    //fix angles
    #ifdef DEBUG
      Serial.print("X: ");
    #endif
    xAng = fixAngle(xAng);
    #ifdef DEBUG
      Serial.print("Y: ");
    #endif
    yAng = fixAngle(yAng);
    #ifdef DEBUG
      Serial.print("Z: ");
    #endif
    zAng = fixAngle(zAng);
    #ifdef DEBUG
      Serial.print(" | ");
    #endif      
    
    //set angles to more sensible numbers
    xAna = setAna(xAng, false);
    yAna = setAna(yAng, false);
    zAna = setAna(zAng, true);

    //synth update
    updateSynth();

    //update the neopixel grid
    #ifdef ENABLENEO
      updateNeopixels(notefreq);
    #endif
    #ifdef DEBUG
      Serial.println("");
    #endif
  }
  else if (lowPower == 1 && mode == 1) {
    #ifdef ZELDA
      playZelda();
    #endif
  }
  else if (lowPower == 1 && mode == 2) {
    #ifdef GAME
      playGame();
    #endif
  }
  else{
    modeSet();
    strip.setBrightness(0);
    strip.show();
    noTone(SPEAKERPIN);
    xAng = 0;
    yAng = 0;
    zAng = 0;
    xAna = 512;
    yAna = 512;
    zAna = 512;
    bufferTime == 0;
  }
}

void updateNeopixels(int freq){
  int color = 0;

    for(int j = 0; j < 64; j++){
        if (freq < 512){
            color = notefreq/2;
            strip.setPixelColor(j,strip.Color(128,0,color));
        }
        else if (freq < 1023){
            color = (freq - 512)/2;
            strip.setPixelColor(j,strip.Color(color,0,128));
        }
        else{
            color = (freq - 1023)/2;
            if (color > 255){
              color = 255;
            }
            strip.setPixelColor(j,strip.Color(128,color,0));
        }
    }
  strip.show();
}

//Start synth stuff
long fixAngle(long Ang){
  while (Ang > 2400){
    Ang = Ang - 4800;
  }
  while (Ang < -2400){
    Ang = Ang + 4800;
  }
  #ifdef DEBUG
    Serial.print(Ang);
    Serial.print("|");
  #endif

  return Ang;
}

long setAna(long Ang, boolean full){
  long Ana = 0;
  if (full){
    Ana = Ang + 2400;
    Ana = Ana*16L;
    Ana = Ana/75L;
  }
  else {
    Ana = Ang + 1200;
    Ana = Ana*32L;
    Ana = Ana/75L;
  }
  if (Ana < 0){
    Ana = 0;
  }
  if (Ana > 1023){
    Ana = 1023;
  }

  return Ana;
}

void updateSynth(){
  int noteind = pentaDigit(zAna);
  noteind  = pentind[noteind];
  noteind  = noteind - ((xAna/128) - 4);
  //even it out
  if (noteind < 0){
    noteind = 0;
  }
  if (noteind > 60){
    noteind = 60;
  }
  notefreq = notes[noteind];
  notefreq = notefreq + ((yAna - 512)/4);

  buttonState = digitalRead(BUTPIN);
  #ifdef DEBUG
    Serial.print(notefreq);
    Serial.print("|");
    Serial.print(buttonState);
  #endif

  if (notefreq < 36){
      notefreq = 36;
  }
  else if (notefreq > 4000){
    notefreq = 4000;
  }

  if (buttonState == 1){
    tone(SPEAKERPIN, notefreq);
    //play tone
  }
  else {
    noTone(SPEAKERPIN);
  }

  if (notefreq < 31){
      notefreq = 31;
  }

  #ifdef TONEON
    tone(SPEAKERPIN, notefreq);
  #endif

  return;
}

byte pentaDigit(int Ana){
  int PD;
  PD = Ana/41;
  PD = PD + 1;
  if (PD > 24){
      PD = 24;
  }
  if (PD < 0){
    PD = 0;
  }
  byte pentaKill = PD;
  return pentaKill;
}

#ifdef ZELDA
void playZelda(){
  int on = 0;
  for (int i = 0; i < zeldaLen; i++){
    strip.setBrightness(ONBRIGHT);
    tone(SPEAKERPIN, freqZelda[i]);
    updateNeopixels(freqZelda[i]);
    delay(noteZelda[i] * 100);
    noTone(SPEAKERPIN);
    on = digitalRead(SWPIN);
    if (on == 0){
      return;
    }
  }
  return;
}
#endif

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
  #ifdef DEBUG
    Serial.print(rate);
  #endif
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

boolean toggleButton(){
  buttonState = digitalRead(BUTPIN);
  if (!offPrevState && buttonState == 0){
    offPrevState = true;
    return true;
  }
  else if (buttonState == 0){
    offPrevState = true;
    return false;
  }
  else {
    offPrevState = false;
    return false;
  }
}

void modeSet(){
  boolean toggle = toggleButton();
  if (toggle){
    mode = mode + 1;
  }
  if (mode > (modes - 1)){
    mode = 0;
  }
  Serial.println(mode);
  return;
}

//game stuff
#ifdef GAME
  void playGame(){
    location = update();
    physics();
    if (collision()){
      dead();
      reset();
    }
    path();
    neoGame();
    int on = digitalRead(SWPIN);  
    if (on == 0){
      return;
    }
    delay(100);
  }

  void physics(){
    //drop one pixel
    for (int i = 0; i < 64; i++){
      prevPixel[i] = pixel[i];
    }
    for (int i = 8; i < 64; i++){
      pixel[i] = pixel[i - 8];
    }
    return;
  }

  boolean collision(){
    if (prevPixel[location] == 1){
      return true;
    }
    else{
      return false;
    }
  }

  int update(){
    if (bufferTime != 0){
      //amount of ms since last poll
      currTime = millis();
      totTime = currTime - bufferTime;
      yAng = angle(GYRO_YOUT_H, GYRO_YOUT_L, -12, yAng);

      //set bufferTime
      bufferTime = currTime;
    }
    else{
      bufferTime = millis();
    }

    yAng = fixAngle(yAng);
    yAna = setAna(yAng, false);
    //right = Y+
    //left  = Y-
    int y = yAna;
    if (y < 256){
      y = 256;
    }
    else if (y > 768){
      y = 768;
    }
    y = y - 256;

    //512
    int pos = y/64;
    Serial.println(pos);
    return pos;
  }

  void dead(){
    int resetButton = 0;
    neoGame();
    strip.setPixelColor((location + 8), strip.Color(255,0,0));
    strip.show();
    while(true){
      resetButton = digitalRead(BUTPIN);
      if (resetButton == 1){
        return;
      }
      int on = digitalRead(SWPIN);  
      if (on == 0){
        return;
    }
    }
  }

  void reset(){
    location = 4;
    xAng = 0;
    for (int i = 0; i < 64; i++){
      pixel[i] == 0;
    }
    for (int i = 0; i < 16; i++){
      pixel[capn[i]] = 1;
    }
    return;
  }

  void path(){
    int random = analogRead(A0);

    if (diff == 0){
      int passArray[8] = {1, 0, 0, 0, 0, 0, 0, 1};
      insert(passArray);
    }
  }

  void insert(int insArray[8]){
    for (int i = 0; i < 8; i++){
      pixel[i] = insArray[i];
    }
  }

  void neoGame(){
    strip.setBrightness(ONBRIGHT);
    for (int i = 0; i < 64; i++){
      if (pixel[i] == 1){
        strip.setPixelColor(i, strip.Color(0, 255, 0));
      }
      else{
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
    }
    strip.setPixelColor((location + 8), strip.Color(0, 0, 255));
    strip.show();
    return;
  }
#endif
/* License Info:
   This is licensed under Beerware R42. There is some code from
   Sparkfun's ITG3200 example code in here, as well as the APC
   code from Beavis Audio Research
*/
