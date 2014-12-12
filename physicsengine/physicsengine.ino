byte pixel[] = {
1, 0, 0, 0, 0, 0, 0, 1,
1, 0, 0, 0, 0, 0, 0, 1,
1, 0, 0, 0, 0, 0, 0, 1,
1, 0, 0, 0, 0, 0, 0, 1,
1, 0, 0, 0, 0, 0, 0, 1,
1, 0, 0, 0, 0, 0, 0, 1,
1, 0, 0, 0, 0, 0, 0, 1,
1, 0, 0, 0, 0, 0, 0, 1
}

byte prevPixel[] = {
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0
}

byte capn[] = {
  0, 7, 8, 15, 16, 23, 24,
  31, 32, 39, 40, 47, 48,
  55, 56, 63
};

byte diff = 0;
byte count = 0;

byte location = 4;
boolean bad = false;

void physics(){
  //drop one pixel
  for (int i = 0; i < 64; i++){
    prevPixel[i] = pixel[i]
  }
  for (int i = 8; i < 64; i++){
    pixel[i] = pixel[i - 8];
  }
  return;
}

boolean collision(){
  if (prevPixel[location + 8] == 1){
    return true;
  }
  else{
    return false;
  }
}

int update(){
  //right = Y+
  //left  = Y-
  int y = yAna
  if (y < 256){
    y = 256;
  }
  else if (y > 768){
    y = 768;
  }
  y = y - 256;

  //512
  int pos = y/64;
  return pos;
}

int dead(){
  int resetButton = 0;
  while(true){
    resetButton = digitalRead(BUTPIN);
    if (resetButton == 1){
      return;
    }
  }
}

void reset(){
  for (int i = 0; i < 64, i++){
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
    insert([1, 0, 0, 0, 0, 0, 0, 1]);
  }
}

void insert(int insArray[8]){
  for (int i = 0; i < 8; i++){
    pixel[i] = insArray[i]
  }
}

void neoGame(){
  for (int i = 0; i < 64; i++){
    if (pixel[i] == 1){
      strip.setPixelColor(i, strip.Color(0, 255, 0));
    }
    else{
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  strip.setPixelColor(location);
  strip.show();
  return;
}

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