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
#include <Adafruit_NeoPixel.h> //Neopixel lib
#include <Wire.h>              //Arduino I2C lib

//initialize Neopixels
#define NEOPIN   5
#define ONBRIGHT 50

//init strip object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(64, NEOPIN, NEO_GRB + NEO_KHZ800);

//stuff for the button
#define BUTPIN 2

//What position is the button in?
int buttonState = 0;

//Mode switch stuff
int offButtonState = 0;
boolean offPrevState = true;
int mode = 0;
const int modes = 3;

//switch
#define SWPIN 3

//lowpower is 0 when it is in low power mode
int lowPower = 0;

//gyro stuff
//buffer for the millis of the last gyro reading
unsigned long bufferTime = 0;
unsigned long currTime = 0;
unsigned long totTime = 0;

//total angle
long xAng = 0;
long yAng = 0;
long zAng = 0;

//angles in 0-1023
long xAna = 500;
long yAna = 500;
long zAna = 500;

//Start gyro defines/addrs
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
volatile unsigned int prevFreq = 0;
volatile boolean change = false;
volatile boolean prevButton = false;

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
  //how long our note lists are
  int zeldaLen = 115;
  //note lengths of the song, 1 is a 16th note
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
  //frequencies of the above notes
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
  //array that shows the current path
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

  //the previous path
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

  //indices of the edges for reset
  byte capn[] = {
    0, 7, 8, 15, 16, 23, 24,
    31, 32, 39, 40, 47, 48,
    55, 56, 63
  };

  //difficulty and up rate clock
  byte diff = 0;
  byte count = 20;

  int updateDelay = 500;
  int playerDelay = 100;

  //location of the player
  byte location = 4;

  //are ya dead?
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

  //start debug monitor if needed
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  //set button/switch to input
  pinMode(BUTPIN, INPUT);
  pinMode(SWPIN, INPUT);

  //set speaker to output
  pinMode(SPEAKERPIN, OUTPUT);

  #ifdef GAME
    //init random seed to an unused analog pin
    randomSeed(analogRead(0));
  #endif
}


void loop() {
  //are we off or on?
  lowPower = digitalRead(SWPIN);
  if (lowPower == 1 && mode == 0){
    //mode 0 is main functionality
    strip.setBrightness(ONBRIGHT);

    //Get gyro reading
    //TODO: pack this in a library
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

    //println to serial to end the current debug line
    #ifdef DEBUG
      Serial.println("");
    #endif
  }
  else if (lowPower == 1 && mode == 1) {
    //if mode is 1 and we compiled ZELDA, play it
    #ifdef ZELDA
      playZelda();
    #endif
  }
  else if (lowPower == 1 && mode == 2) {
    //if mode is 2 and we compiled GAME, play it
    #ifdef GAME
      reset();
      playGame();
    #endif
  }
  else{
    //check for a new mode
    modeSet();

    //make sure strip is off
    strip.setBrightness(0);
    strip.show();

    //make sure tone is off
    noTone(SPEAKERPIN);

    //reset angles
    xAng = 0;
    yAng = 0;
    zAng = 0;

    //more resetting
    xAna = 512;
    yAna = 512;
    zAna = 512;

    //reset freq
    notefreq = 0;

    //reset buffer time to force a refresh on power-on
    bufferTime == 0;
  }
}

void updateNeopixels(int freq){
  //make color
  //TODO: Color schemes?
  int color = 0;

  for(int j = 0; j < 64; j++){
    if (freq < 512){
      //if small, assign and go to town
      color = notefreq/2;
      strip.setPixelColor(j,strip.Color(128,0,color));
    }
    else if (freq < 1023){
      //if larger, assign slightly differently
      color = (freq - 512)/2;
      strip.setPixelColor(j,strip.Color(color,0,128));
    }
    else{
      //throw a green in there if it's really big...
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
  //simple conversion that says
  //"if the angle > 180 deg, set it to -180 deg
  //else if angle <  -180 deg, set it to 180 deg"

  //These are while loops just in case the angle
  //is really bad
  while (Ang > 2400){
    Ang = Ang - 4800;
  }
  while (Ang < -2400){
    Ang = Ang + 4800;
  }

  //print ang to debug console and return
  #ifdef DEBUG
    Serial.print(Ang);
    Serial.print("|");
  #endif

  return Ang;
}

long setAna(long Ang, boolean full){
  long Ana = 0;

  //simple math to convert and double check
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
  //find the initial note
  int noteind = pentaDigit(zAna);
  prevFreq = notefreq;
  
  //find where that note is on the index
  noteind  = pentind[noteind];

  //modify it accordingly
  noteind  = noteind - ((xAna/128) - 4);

  //even it out
  if (noteind < 0){
    noteind = 0;
  }
  if (noteind > 60){
    noteind = 60;
  }
  //set initial frequency
  notefreq = notes[noteind];

  //fine tune frequency
  notefreq = notefreq + ((yAna - 512)/4);

  //read the button
  buttonState = digitalRead(BUTPIN);

  //modify the freq to reduce helicoptering
  if (notefreq < 36){
      notefreq = 36;
  }
  //unsigned int leads to problems when the fine-tune
  //causes it to go below zero.
  else if (notefreq > 4000){
    notefreq = 36;
  }

  #ifdef DEBUG
    Serial.print(notefreq);
    Serial.print("|");
    Serial.print(buttonState);
  #endif

  //if the button is on, play it
  if (buttonState == 1){
    if (prevFreq != notefreq || !prevButton){
      tone(SPEAKERPIN, notefreq);
      }
    prevButton = true;
  }
  //if the button is off, make sure the tone is too
  else {
    noTone(SPEAKERPIN);
    prevButton = false;
  }

  //if the TONEON flag is set, play it either way
  #ifdef TONEON
    tone(SPEAKERPIN, notefreq);
  #endif

  return;
}

byte pentaDigit(int Ana){
  //takes an Ana value and changes it
  //to an index in our pentatonic array
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
  strip.setBrightness(ONBRIGHT);
  for (int i = 0; i < zeldaLen; i++){
    //play the correct tone
    tone(SPEAKERPIN, freqZelda[i]);

    //update the grid
    updateNeopixels(freqZelda[i]);

    //delay the correct amount
    delay(noteZelda[i] * 100);

    //done
    noTone(SPEAKERPIN);

    //if the switch is off, kill it
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

//sparkfun provided I2C communication function
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

//another sparkfun communication function
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

//the big function to modify our angle
long angle(char addrA, char addrB, char offset, long oldAng){

  //turn to a helper function to read the rate
  int rate = read(addrA, addrB, offset);
  #ifdef DEBUG
    Serial.print(rate);
  #endif

  //up our old ang to make math more accurate
  oldAng = oldAng * 1000L;

  //multiply rate * time and add
  long ang = oldAng + (rate*totTime);

  //scale back down
  ang = ang/1000L;
  return ang;
}

int read(char addrA, char addrB, char offset)
{
  //use itgRead to get our rate from the two addrs
  int data=0;
  data = itgRead(itgAddress, addrA)<<8;
  data |= itgRead(itgAddress, addrB);

  //compensate for drift
  data += offset;
  return data;
}

int matrix(int x, int y){
  //helper 
  int result = (56 - y*8) + x;
  return result;
}

boolean toggleButton(){
  //turn a momentary button into a toggle button
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
  //set the mode using the toggle function
  boolean toggle = toggleButton();
  if (toggle){
    mode = mode + 1;
  }
  if (mode > (modes - 1)){
    mode = 0;
  }

  #ifdef DEBUG
    Serial.println(mode);
  #endif
  return;
}

//game stuff
#ifdef GAME
  void playGame(){
    static int updateCount = 0;
    location = update();
    updateCount +=1;
    physics();
    if (collision()){
      reset();
    }
    if (updateCount == 5){
        updateCount = 0;
        diffup();
        path();}

    neoGame();
    int on = digitalRead(SWPIN);  
    if (on == 0){
      return;
    }
    delay(100);
  }
  
  void diffup(){
      count -= 1;
      if (count == 0){
          diff += 1;
          if (diff < 5){
            for (int i = 54; i < 64; i++){
                if (prevPixel[i] == 0){
                    prevPixel[i] == 1;
                    count = random(1,40);
                    return;
                }
            }
          }
          else {
              updateDelay = 500 - diff;
          }
      return;
    }
  }

  void physics(){
    //drop one pixel
    for (int i = 0; i < 64; i++){
      prevPixel[i] = pixel[i];
    }
    for (int i = 0; i < 54; i++){
      pixel[i] = pixel[i + 8];
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
    reset();
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
    xAna = 0;
    for (int i = 0; i < 64; i++){
      pixel[i] == 0;
    }
    for (int i = 0; i < 16; i++){
      pixel[capn[i]] = 1;
    }
    for (int i = 0; i < 64; i++){
        prevPixel[i] = pixel[i];
    }
    diff = 0;
    count = 20;
    return;
  }

  void path(){
    int rand = random(1,10);
    int prevPath[8] = {prevPixel[56], prevPixel[57], prevPixel[58], prevPixel[59], prevPixel[60], prevPixel[61], prevPixel[62], prevPixel[63]};
    int path[8];
    int buffer[8];
    for (int i = 0; i < 8; i++){
        buffer[i] = prevPath[i];
    }
    if (rand < 4){
        if (prevPath[1] == 0){
            for (int i = 0; i < 8; i++){
              path[i] = prevPath[i];
            }
        }
        else{
          for(int i = 1; i < 7; i++){
              path[i] = buffer[i - 1];
          }
            path[0] = 1;
        }
    }
    else if (rand > 6){
        if (prevPath[6] == 0){
            for (int i = 0; i < 8; i++){
              path[i] = prevPath[i];
            }
        }
        else{
            for(int i = 0; i < 6; i++){
                path[i] = buffer[i + 1];
            }
            path[7] = 1;
        }
    }
    insert(path);
    for (int i = 0; i < 8; i++){
      Serial.print(path[i]);
    }
    Serial.println("");
  }


  void insert(int insArray[8]){
    for (int i = 56; i < 64; i++){
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
        strip.setPixelColor(i, strip.Color(255, 255, 255));
      }
    }
    strip.setPixelColor((location + 8), strip.Color(0, 0, 255));
    strip.show();
    return;
  }
#endif
/* License Info:
   This is licensed under Beerware R42.
   Stuff from Sparkfun's Gyro example code is here
   it is labeled accordingly
*/
