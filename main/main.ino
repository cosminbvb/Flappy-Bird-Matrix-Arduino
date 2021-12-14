#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>


/****** LED MATRIX ******/

const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;
const int matrixSize = 8;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1); //DIN, CLK, LOAD, No. DRIVER

int matrixBrightness = 2;
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


/****** JOYSTICK ******/

const int xPin = A0;
const int yPin = A1;
const int swPin = A4;

int xValue = 0;
int yValue = 0;
bool joyMoved = true;
bool swState = LOW;
bool lastSwState = LOW;

const int minThreshold = 350;
const int maxThreshold = 700;
const int debounceDelay = 50;
unsigned long lastSwDebounceTime;


/****** LCD ******/

const int RS = 7;
const int enable = 8;
const int d4 = 5;
const int d5 = 4;
const int d6 = 3;
const int d7 = 2;
const int lcdContrastPin = 6;
const int lcdBrightnessPin = 9;

int lcdContrast = 90;
int lcdBrightness = 128;

LiquidCrystal lcd(RS, enable, d4, d5, d6, d7);


/****** GAME LOGIC ******/

bool runningGame = false;
bool gameOver = false;

// Flap mechanic
const byte flapButtonPin = A5;
bool buttonState;
bool lastButtonState;
bool reading;
bool registeredFlap = false;
int flaps = 0;
unsigned long lastDebounceTime;
const byte flapHeight = 2;

// Obstacle generation
const int randomPin = A3; // we use this pin to read noise values in order to add to the randomness of the prng
byte gapStart;
byte gapLength;

// Bird position
byte birdRow = 4;
const byte birdCol = 3;

// Time constants
const int obstacleInterval = 1200;
const int matrixUpdateInterval = 200;
const int shiftInterval = 180;
const int birdFlapInterval = 100;
const int birdFallInterval = 200;
const int collisionBlinkInterval = 200;
const int gameOverAnimationDuration = 3000;

unsigned long long lastObstacleTime;
unsigned long long lastShiftTime;
unsigned long long lastBirdMove;
unsigned long long lastBlinkTime;
unsigned long long gameOverTime;
unsigned long countdownStartTime;

// Collision blinking
bool blinkState;
bool countdownStarted = false;

const int buzzerPin = 13;

unsigned long score = 0; // score increases with each obstacle


/****** LEADERBOARD ******/

class Player {
  public:
    char name[10];
    unsigned long score;
};

Player player;
Player leaderboard[3]; // top 3 players

int eepromAddress = 0;

// Enter Name Prompt
int letter = 122;
int letterPosition = 0;
char playerName[10] = {'_', '_', '_', '_', '_', '_', '_', '_', '_', '_'};
bool nameEntered = false;
bool selectedChar = false;
bool registeredScore = false;
bool joyMovedLetterSelection = false;
bool joyMovedPositionSelection = false;


/****** MENU ******/

const String menu[4] = {"Play", "Highscore", "Settings", "About"};
int menuIndex = 0;     // index of the displayed option
bool goToOption = false;
bool goToMenu = false;

// about section
const String aboutFirstRow = "Flappy Bird On LED Matrix - Cosmin Petrescu";
byte stringIndex = 0;
const String aboutSecondRow = "GitHub:cosminbvb";
const int aboutShiftInterval = 500;
unsigned long lastAboutShiftTime;

// highscore section
byte highscoreIndex = 0;
byte highscoreMaxIndex = 2;
bool joyMovedHighscore = false;
bool showHighscore = true;

// settings section
byte settingsIndex = 0;
byte settingsMaxIndex = 2;
bool joyMovedSettings = false;
bool joyMovedIntensity = false;
bool showSettings = true;

int lastMatrixBrightness;

void setup() {
  Serial.begin(9600);
  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0, false);                // turn off power saving, enables display
  lc.setIntensity(0, matrixBrightness); // sets brightness (0~15 possible values)
  lc.clearDisplay(0);                   // clear screen

  pinMode(randomPin, INPUT);
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  pinMode(swPin, INPUT_PULLUP);
  pinMode(flapButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(lcdContrastPin, OUTPUT);
  pinMode(lcdBrightnessPin, OUTPUT);

  analogWrite(lcdContrastPin, lcdContrast);
  analogWrite(lcdBrightnessPin, lcdBrightness);

  lcd.begin(16, 2); // set up the LCD's number of columns and rows:

  // initializeLeaderboardEEPROM(); // TODO: initialize if empty
  getLeaderboardFromEEPROM();       // read the top 3 players from memory
}

void loop() {
  // if nameEntered == false => wait for the user to enter a name
  // Serial.println(nameEntered);
  if (!nameEntered) {
    getPlayerName();
  }
  else {
    handleMenu();
  }
  // analogWrite(contrastPin, 90);
}


# pragma region MENU

void getPlayerName() {
  // handles the initial Enter Name prompt
  // joystick vertical move => select letter
  // joystick horizontal move => select letter position
  // joystick click => save entered name
  lcd.setCursor(0, 0);
  lcd.print("Name: ");
  selectLetterPosition();
  selectLetter();
  for (int i = 0; i < 9; i++) {
    lcd.setCursor(i + 6, 0);
    lcd.print(playerName[i]);
  }
  lcd.setCursor(1, 1);
  lcd.print("Click to Save");
  readJoystickSw(nameEntered);
  if (nameEntered) {
    // clear underscores
    for (int i = 0; i <= 9; i++) {
      if (playerName[i] == '_') {
        playerName[i] = ' ';
      }
    }
    strcpy(player.name, playerName);
    lcd.clear();
    lcd.setCursor(0, 0);
  }
}

void selectLetter() {
  xValue = analogRead(xPin);
  if (xValue > maxThreshold && !joyMovedLetterSelection) {
    if (letter < 122) {
      letter++;
    }
    else {
      letter = 97;
    }
    playerName[letterPosition] = letter;
    joyMovedLetterSelection = true;
  }

  if (xValue < minThreshold && !joyMovedLetterSelection) {
    if (letter > 97) {
      letter--;
    }
    else {
      letter = 122;
    }
    playerName[letterPosition] = letter;
    joyMovedLetterSelection = true;
  }

  if (xValue >= minThreshold && xValue <= maxThreshold) {
    joyMovedLetterSelection = false;
  }
}

void selectLetterPosition() {
  yValue = analogRead(yPin);
  if (yValue < minThreshold && !joyMovedPositionSelection) {
    if (letterPosition > 0) {
      letterPosition--;
    }
    else {
      letterPosition = 9;
    }
    letter = 97;
    joyMovedPositionSelection = true;
  }
  if (yValue > maxThreshold && !joyMovedPositionSelection) {
    if (letterPosition < 9) {
      letterPosition++;
    }
    else {
      letterPosition = 0;
    }
    letter = 97;
    joyMovedPositionSelection = true;
  }

  if (yValue >= minThreshold && yValue <= maxThreshold) {
    joyMovedPositionSelection = false;
  }
}

void handleMenu() {
  if (joyMoved) {
    lcd.clear();
    lcd.print(menu[menuIndex]);
  }
  if (!goToOption) {
    // only listen for vertical menu movement when
    // no menu option is currently in process
    handleMenuVerticalMovement();
  }
  handleMenuClick();
}

void handleMenuVerticalMovement() {
  xValue = analogRead(xPin);
  if (xValue > maxThreshold && !joyMoved) {
    if (menuIndex > 0) {
      menuIndex--;
    }
    else {
      menuIndex = 3;
    }
    joyMoved = true;
  }

  if (xValue < minThreshold && !joyMoved) {
    if (menuIndex < 3) {
      menuIndex++;
    }
    else {
      menuIndex = 0;
    }
    joyMoved = true;
  }

  if (xValue >= minThreshold && xValue <= maxThreshold) {
    joyMoved = false;
  }
}

void handleMenuClick() {
  // calls the right function for each selected option
  if (!goToOption) {
    readJoystickSw(goToOption);
  }
  if (goToOption) {
    switch (menuIndex) {
      case 0:
        handlePlay();
        break;
      case 1:
        handleHighscore();
        break;
      case 2:
        handleSettings();
        break;
      case 3:
        handleAbout();
        break;
    }
  }
}

void handlePlay() {
  if (!runningGame && !gameOver) {
    // if the game is not running
    // and we don't have any post game processing to do (gameOver == false)

    if (!countdownStarted)
    {
      // starting a 3 second countdown
      countdownStartTime = millis();
      countdownStarted = true;
    }
    if (millis() - countdownStartTime < 3000) {
      // display the countdown on the matrix
      displayCountdown();
    }
    else {
      // when the countdown is over, start the game
      countdownStarted = false;
      runningGame = true;
      lc.setLed(0, birdRow, birdCol, true);  // display bird
    }
  }
  if (runningGame && !gameOver) {
    // if the game is running 
    // and we don't have any post game processing to do (gameOver == false)
    readFlap(); // button press listener
    if (registeredFlap) {
      flaps++;  // every time a user presses the button, we increase the number of queued flaps
      tone(buzzerPin, 6000, 50);
    }
    handleBirdMovement();  // moves the bird vertically
    handleMapMovement();   // moves the map (aka bird pov) to the left and increases the current score
    checkCollision();      // ends the game if the bird hit an obstacle
    showLiveScore();       // displays a live score on the lcd
  }
  if (gameOver) {
    // if gameOver == true, the game just ended, meaning that
    // we need to show a crash animation for a few seconds and then
    // analize the score, store it and display the position he finished in
    if (millis() - gameOverTime < gameOverAnimationDuration) {
      collisionAnimation();
    }
    else {
      if (!registeredScore) {
        // registering only once
        registerScore(); // TODO: split into store and show
      }
      resetGame();   // reset map, bird and score
      backToMenu();  // if the user presses down on the joystick, we return him to the menu
    }
  }
}

void handleHighscore() {
  if (showHighscore) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(highscoreIndex + 1);
    lcd.print(".");
    for (int i = 0; i < 10; i++) {
      lcd.setCursor(i + 3, 0);
      lcd.print(leaderboard[highscoreIndex].name[i]);
    }
    lcd.setCursor(3, 1);
    lcd.print("Score: ");
    lcd.setCursor(10, 1);
    lcd.print(leaderboard[highscoreIndex].score);
    showHighscore = false;
  }
  handleHighscoreVerticalMovement();
  backToMenu();
}

void handleHighscoreVerticalMovement() {
  xValue = analogRead(xPin);
  if (xValue > maxThreshold && !joyMovedHighscore) {
    if (highscoreIndex > 0) {
      highscoreIndex--;
    }
    else {
      highscoreIndex = highscoreMaxIndex;
    }
    joyMovedHighscore = true;
    showHighscore = true;
  }

  if (xValue < minThreshold && !joyMovedHighscore) {
    if (highscoreIndex < highscoreMaxIndex) {
      highscoreIndex++;
    }
    else {
      highscoreIndex = 0;
    }
    joyMovedHighscore = true;
    showHighscore = true;
  }

  if (xValue >= minThreshold && xValue <= maxThreshold) {
    joyMovedHighscore = false;
  }
}

void handleSettings() {
  if (showSettings) {
    lcd.clear();
    lcd.setCursor(0, 0);
    switch (settingsIndex) {
      case 0:
        lcd.print("LEDs Brightness");
        break;
      case 1:
        lcd.print("LCD Contrast");
        break;
      case 2:
        lcd.print("LCD Brightness");
        break;
    }
    showSettings = false;
  }
  switch (settingsIndex) {
    case 0:
      for (int row = 0; row < matrixSize; row++) {
        for (int col = 0; col < matrixSize; col++) {
          lc.setLed(0, row, col, 1);
        }
      }
      handleSettingsHorizontalMovement(matrixBrightness, 15, 1);
      lc.setIntensity(0, matrixBrightness);
      lcd.setCursor(6, 1);
      lcd.print("<");
      lcd.print(matrixBrightness);
      lcd.print(">");
      break;
    case 1:
      handleSettingsHorizontalMovement(lcdContrast, 255, 10);
      analogWrite(lcdContrastPin, lcdContrast);
      lcd.setCursor(6, 1);
      lcd.print("<");
      lcd.print(lcdContrast);
      lcd.print(">");
      break;
    case 2:
      handleSettingsHorizontalMovement(lcdBrightness, 255, 30);
      analogWrite(lcdBrightnessPin, lcdBrightness);
      lcd.setCursor(6, 1);
      lcd.print("<");
      lcd.print(lcdBrightness);
      lcd.print(">");
      break;
  }
  handleSettingsVerticalMovement();
  backToMenu();
}

void handleSettingsVerticalMovement() {
  xValue = analogRead(xPin);
  if (xValue > maxThreshold && !joyMovedSettings) {
    if (settingsIndex > 0) {
      settingsIndex--;
    }
    else {
      settingsIndex = settingsMaxIndex;
    }
    joyMovedSettings = true;
    showSettings = true;
  }

  if (xValue < minThreshold && !joyMovedSettings) {
    if (settingsIndex < settingsMaxIndex) {
      settingsIndex++;
    }
    else {
      settingsIndex = 0;
    }
    joyMovedSettings = true;
    showSettings = true;
  }

  if (xValue >= minThreshold && xValue <= maxThreshold) {
    joyMovedSettings = false;
  }
}

void handleSettingsHorizontalMovement(int &intensity, int maxIntensity, int step) {
  yValue = analogRead(yPin);
  if (yValue < minThreshold && !joyMovedIntensity) {
    if (intensity >= step) {
      intensity -= step;
    }
    else {
      intensity = maxIntensity;
    }
    joyMovedIntensity = true;
    showSettings = true;
  }
  if (yValue > maxThreshold && !joyMovedIntensity) {
    if (intensity <= maxIntensity - step) {
      intensity += step;
    }
    else {
      intensity = 0;
    }
    joyMovedIntensity = true;
    showSettings = true;
  }

  if (yValue >= minThreshold && yValue <= maxThreshold) {
    joyMovedIntensity = false;
  } 
}

void handleAbout() {
  if ((millis() - lastAboutShiftTime) > aboutShiftInterval) {
    lcd.setCursor(0, 0);
    lcd.print(aboutFirstRow.substring(stringIndex, stringIndex + 16));
    lcd.setCursor(0, 1);
    lcd.print(aboutSecondRow);
    stringIndex += 1;
    if (stringIndex > (aboutFirstRow.length() - 16)) {
      stringIndex = 0;
    }
    lastAboutShiftTime = millis();
  }
  backToMenu();
}

void readJoystickSw(bool &pressed) {
  // sets pressed to high if the user pressed down on the joystick
  reading = !digitalRead(swPin);
  // If the switch changed, due to noise or pressing:
  if (reading != lastSwState) {
    // reset the debouncing timer
    lastSwDebounceTime = millis();
  }
  if ((millis() - lastSwDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    if (reading != swState) {
      swState = reading;
      if (swState == HIGH) {
        pressed = true;
      }
    }
  }
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastSwState = reading;
}

void backToMenu() {
  // if the user presses down on the joystick
  // we return the him to the menu state
  // and we display the first menu option
  readJoystickSw(goToMenu);
  if (goToMenu) {
    goToOption = false;         // menu option satisfied
    lcd.clear();                
    lcd.setCursor(0, 0);
    lcd.print(menu[menuIndex]); // display the current menu option
    gameOver = false;           // mark post game processing as done
    goToMenu = false;           // back to menu command completed
    registeredScore = false;    // reset the flag for the next game
    showHighscore = true;       // reset the flag for the next highscore access
    showSettings = true;        // reset the flag for the next settings acess
  }
}

# pragma endregion

# pragma region LEADERBOARD

/*
  void initializeLeaderboardEEPROM() {
  for (int i = 0; i < 3; i++) {
    Player player;
    strcpy(player.name, "Unknown");
    player.score = 0;
    EEPROM.put(eepromAddress, player);
    eepromAddress += sizeof(Player);
  }
  }
*/

void getLeaderboardFromEEPROM() {
  for (int i = 0; i < 3; i++) {
    Player player;
    EEPROM.get(eepromAddress, player);
    eepromAddress += sizeof(Player);
    leaderboard[i] = player;
  }
  eepromAddress = 0;
}

int getLeaderboardPosition(int score) {
  // compares the given score with the top 3 scores
  if (score <= leaderboard[2].score) {
    // not in top 3
    return -1;
  }
  else if (score > leaderboard[0].score) {
    // first place
    return 0;
  }
  else if (score > leaderboard[1].score) {
    // second place
    return 1;
  }
  else {
    // third place
    return 2;
  }
}

# pragma endregion

# pragma region GAME LOGIC

void resetGame() {
  // reset map
  for (int i = 0; i < matrixSize; i++) {
    for (int j = 0; j < matrixSize; j++) {
      matrix[i][j] = 0;
    }
  }
  // reset bird height
  birdRow = 4;
  // reset score
  score = 0;
}

void registerScore() {
  // stores the name and score in the EEPROM if it made the top 3
  // and shows a status regarding the obtained position on the lcd
  int leaderboardPosition = getLeaderboardPosition(score); // returns the position in the leaderboard (indexed from 0)
  if (leaderboardPosition == 2) {
    // if he finished 3rd
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You came 3rd! :)");
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.setCursor(7, 1);
    lcd.print(score);
    player.score = score;
    leaderboard[2] = player;  // update 3rd place
    EEPROM.put(eepromAddress + 2 * sizeof(Player), player);  // store the leadeboard
  }
  else if (leaderboardPosition == 1) {
    // if he finished 2nd
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You came 2nd! :)");
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.setCursor(7, 1);
    lcd.print(score);
    player.score = score;
    leaderboard[2] = leaderboard[1];  // relegate the lower score
    leaderboard[1] = player;          // update 2nd place
    EEPROM.put(eepromAddress + sizeof(Player), player);  // store the leadeboard
  }
  else if (leaderboardPosition == 0) {
    // if he finished 1st
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You came 1st! :)");
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.setCursor(7, 1);
    lcd.print(score);
    player.score = score;
    leaderboard[2] = leaderboard[1];    // relegate the lower score
    leaderboard[1] = leaderboard[0];    // relegate the lower score
    leaderboard[0] = player;            // update 1st place
    EEPROM.put(eepromAddress, player);  // store the leadeboard
  }
  else {
    // if he didn't make the top 3
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Not in top 3 :(");
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.setCursor(7, 1);
    lcd.print(score);
  }
  registeredScore = true;  // mark the score as registered
}

void showLiveScore() {
  lcd.setCursor(0, 0);
  lcd.print("Score:");
  lcd.setCursor(7, 0);
  lcd.print(score);
}

void checkCollision() {
  // if there is an obstacle at matrix[birdRow][birdCol] => collision
  if (matrix[birdRow][birdCol]) {
    runningGame = false;
    gameOver = true;
    gameOverTime = millis();
  }
}

void collisionAnimation() {
  // blink the led to the left of the collision point
  if (millis() - lastBlinkTime > collisionBlinkInterval) {
    blinkState = !blinkState;
    lc.setLed(0, birdRow, birdCol - 1, blinkState);
    lastBlinkTime = millis();
  }
}

void handleBirdMovement() {
  if (flaps) {
    // increase height by flapHeight
    if (millis() - lastBirdMove >= birdFlapInterval) {
      lc.setLed(0, birdRow, birdCol, false);
      birdRow -= flapHeight;
      flaps--;
      if (birdRow <= -1) {
        runningGame = false;
        gameOver = true;
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
        gameOver = true;
        gameOverTime = millis();
      }
      lc.setLed(0, birdRow, birdCol, true);
      lastBirdMove = millis();
    }
  }

}

void readFlap() {
  reading = !digitalRead(flapButtonPin);
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

void handleMapMovement() {
  generateObstacle();
  if (matrixChanged) {
    updateMatrix();
    matrixChanged = false;
  }
}

void getRandomObstacle() {
  // initialize the seed of the rng with the value of miliseconds
  // plus the value read by an unconnected pin
  randomSeed(millis() + analogRead(randomPin));
  gapStart = random(0, 4);  // generate random a gap starting point
  gapLength = random(3, 6); // and random a gap size
}

void shiftMatrix() {
  // shift matrix to the left
  for (int i = 0; i < matrixSize; i++) {
    for (int j = 0; j < matrixSize - 1; j++) {
      matrix[i][j] = matrix[i][j + 1];
    }
    matrix[i][matrixSize - 1] = 0;
  }
}

void generateObstacle() {
  // shift matrix to the left every shiftInterval miliseconds
  if (millis() - lastShiftTime > shiftInterval) {
    shiftMatrix();
    lastShiftTime = millis();
  }
  // generate and add a new obstacle on the map every obstacleInterval miliseconds
  if (millis() - lastObstacleTime > obstacleInterval) {
    score++;
    getRandomObstacle();
    // set the column values of the obstacle (1 for obstacle, 0 for gap)
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
  matrixChanged = true;  // give the led matrix the signal that it needs to update
}

void updateMatrix() {
  writeMatrix(matrix);
  lc.setLed(0, birdRow, birdCol, true); // keeps the bird led from blinking
}

void writeMatrix(bool matrix[][matrixSize]) {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, matrix[row][col]);
    }
  }
}

void displayCountdown() {
  bool three[matrixSize][matrixSize] = {
    {0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  };
  bool two[matrixSize][matrixSize] = {
    {0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  };
  bool one[matrixSize][matrixSize] = {
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  };
  if (millis() - countdownStartTime < 1000) {
    writeMatrix(three);
  }
  else if (millis() - countdownStartTime < 2000) {
    writeMatrix(two);
  }
  else {
    writeMatrix(one);
  }
}

# pragma endregion
