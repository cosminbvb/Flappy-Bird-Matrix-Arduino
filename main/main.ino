#include "LedControl.h"
#include <LiquidCrystal.h>

/* LED MATRIX */
const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1); //DIN, CLK, LOAD, No. DRIVER

byte matrixBrightness = 2;
const int matrixSize = 8;

unsigned long long lastMoved = 0;

bool matrixChanged = true;

bool matrix[matrixSize][matrixSize] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};


/* JOYSTICK */
const int xPin = A0;
const int yPin = A1;

byte xPos = 0;
byte yPos = 0;

byte xLastPos = 0;
byte yLastPos = 0;

const int minThreshold = 200;
const int maxThreshold = 600;

const int swPin = 8;
bool buttonState;
bool lastButtonState;
bool reading;

const int debounceDelay = 50;
unsigned long lastDebounceTime;


/* LCD */
const int RS = 7;
const int enable = 6;
const int d4 = 5;
const int d5 = 4;
const int d6 = 3;
const int d7 = 2;

LiquidCrystal lcd(RS,enable,d4,d5,d6,d7);

/* GAME LOGIC */
bool runningGame = true;

byte gapStart;
byte gapLength;

byte birdRow = 4;
const byte birdCol = 3;

const int obstacleInterval = 1200;
const int matrixUpdateInterval = 200;
const int shiftInterval = 180;
const int birdFlapInterval = 100;
const int birdFallInterval = 200;

unsigned long long lastObstacleTime;
unsigned long long lastShiftTime;
unsigned long long lastBirdMove;

const int collisionBlinkInterval = 200;
unsigned long long lastBlinkTime;
bool blinkState;

const int gameOverAnimationDuration = 5000;
unsigned long long gameOverTime = 0;

void setup() {
  Serial.begin(9600);
  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0, false); // turn off power saving, enables display
  lc.setIntensity(0, matrixBrightness); // sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen

  pinMode(A3, INPUT);
  pinMode(swPin, INPUT_PULLUP);
  lc.setLed(0, birdRow, birdCol, true);

  // set up the LCD's number of columns and rows:
  lcd.begin(16,2);
  // Print a message to the LCD:
  lcd.print("hello, world!");

}

bool registeredFlap = false;
int flaps = 0;
const byte flapHeight = 2;

void loop() { 
  if (runningGame) {
    readFlap();
    if (registeredFlap) {
      flaps++;
    }
    handleBirdMovement();
    handleMapMovement();
    checkCollision();
  }
  else {
    // perform the collision animation for a few seconds
    if (millis() - gameOverTime < gameOverAnimationDuration) {
      collisionAnimation();      
    }
  }
}

/* GAME LOGIC */
void checkCollision() {
  // if there is an obstacle at matrix[birdRow][birdCol] => collision
  if (matrix[birdRow][birdCol]) {
    runningGame = false;
    gameOverTime = millis();
  }
}


/* BIRD LOGIC */

void handleBirdMovement() {
  if (flaps) {
    // increase height by flapHeight
    if (millis() - lastBirdMove >= birdFlapInterval) {
      lc.setLed(0, birdRow, birdCol, false);
      birdRow -= flapHeight;
      flaps--;
      if (birdRow <= -1) {
        runningGame = false;
        gameOverTime = millis();
      }
      lc.setLed(0, birdRow, birdCol, true);
      lastBirdMove = millis();
    } 
  }
  else {
    // decrease height by 1
    if (millis() - lastBirdMove >= birdFallInterval) {
      lc.setLed(0, birdRow, birdCol, false);
      birdRow += 1;
      if (birdRow >= matrixSize - 1) {
        runningGame = false;
        gameOverTime = millis();
      }
      lc.setLed(0, birdRow, birdCol, true);
      lastBirdMove = millis();
    } 
  }

}

void readFlap() {
  reading = !digitalRead(swPin);

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading == HIGH && buttonState == LOW) {
      registeredFlap = true;
    }
    else {
      registeredFlap = false;
    }
    
    if (reading != buttonState) {
      buttonState = reading;
    }
  }
  
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}


/* MAP LOGIC */

void handleMapMovement() {
  generateObstacle();
  if (matrixChanged) {
    updateMatrix();
    matrixChanged = false;
  }
}


void getRandomObstacle() {
  randomSeed(millis() + analogRead(A3));
  gapStart = random(0, 4);
  gapLength = random(3, 6);
}

void shiftMatrix() {
  for (int i = 0; i < matrixSize; i++) {
    for (int j = 0; j < matrixSize - 1; j++) {
      matrix[i][j] = matrix[i][j+1]; 
    }
    matrix[i][matrixSize - 1] = 0;
  }  
}

void generateObstacle() {
  if (millis() - lastShiftTime > shiftInterval) {
    shiftMatrix();
    lastShiftTime = millis();
  }
  if (millis() - lastObstacleTime > obstacleInterval) {
    // generate new obstacle
    getRandomObstacle();
    for (int i = 0; i < matrixSize; i++) {
      if (i < gapStart) {
        matrix[i][matrixSize - 1] = 1;
      }
      else if (i >= gapStart + gapLength) {
        matrix[i][matrixSize - 1] = 1;
      }
      else {
        matrix[i][matrixSize - 1] = 0;
      }
    }
    lastObstacleTime = millis();
  }
  matrixChanged = true;
}



/* LED MATRIX */

void collisionAnimation() {
  // blink the led to the left of the collision point
  if (millis() - lastBlinkTime > collisionBlinkInterval) {
    blinkState = !blinkState;
    lc.setLed(0, birdRow, birdCol - 1, blinkState);
    lastBlinkTime = millis();
  }
}

void updateMatrix() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, matrix[row][col]);
    }
  }
  lc.setLed(0, birdRow, birdCol, true); // keeps the bird led from blinking
}
