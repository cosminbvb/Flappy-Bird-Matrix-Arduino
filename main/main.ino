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
int lastMatrixBrightness;
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
byte phase = 1;

bool runningGame = false;
bool gameOver = false;

int phaseThreshold = 10; // score threshold for phase switching

// Flap mechanic
const byte flapButtonPin = A2;
bool buttonState;
bool lastButtonState;
bool reading;
bool registeredFlap = false;
int flaps = 0;
unsigned long lastDebounceTime;
const byte flapHeight = 2;

// Obstacle generation
const int randomPin = A3; // we use this pin to read noise values in order to add to the randomness of the prng

// used for phase 1
byte gapStart;
byte gapLength;

// used for phase 2
byte y1 = 2;  // first height coordinate of the obstacle
byte y2 = 7;  // second -//-
byte lastY1 = 2;
byte lastY2 = 7;

// Bird position
byte birdRow = 4;
const byte birdCol = 1;

const int obstacleInterval = 2200; 
int shiftInterval = 330;     // changes with difficulty
int shiftIntervalPhase2 = 300;
const int newObstacleIntervalPhase2 = 800;
const int scoreUpdateIntervalPhase2 = 1000;
const int birdFlapInterval = 100;
const int birdFallInterval = 200;
const int collisionBlinkInterval = 200;
const int gameOverAnimationDuration = 3000;

unsigned long long lastObstacleTime;
unsigned long long lastShiftTime;
unsigned long long lastBirdMove;
unsigned long long lastBlinkTime;
unsigned long long gameOverTime;
unsigned long long countdownStartTime;
unsigned long long lastScoreUpdatePhase2;

// Collision blinking
bool blinkState;
bool countdownStarted = false;

const int buzzerPin = 13;
bool soundEffects = true;

unsigned int score = 0; // score increases with each obstacle
unsigned int lastScore = 0;
bool scoreChanged = true;

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
int letter = 123;
int letterPosition = 0;
char playerName[10] = {'_', '_', '_', '_', '_', '_', '_', '_', '_', '_'};
bool nameEntered = false;
bool selectedChar = false;
bool registeredScore = false;
bool joyMovedLetterSelection = false;
bool joyMovedPositionSelection = false;


/****** MENU ******/
const int welcomeMessageDuration = 3000;

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
bool joyMovedHighscore = true;

// settings section
byte settingsIndex = 0;
byte settingsMaxIndex = 3;
bool joyMovedSettings = false;
bool joyMovedIntensity = false;
bool joyMovedSound = false;
bool showSettings = true;


void setup() {
  Serial.begin(9600);
  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0, false);                // turn off power saving, enables display
  lc.clearDisplay(0);                   // clear screen

  pinMode(randomPin, INPUT);
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  pinMode(swPin, INPUT_PULLUP);
  pinMode(flapButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(lcdContrastPin, OUTPUT);
  pinMode(lcdBrightnessPin, OUTPUT);

  getSettingsFromEEPROM();
  getLeaderboardFromEEPROM();       // read the top 3 players from memory

  analogWrite(lcdContrastPin, lcdContrast);
  analogWrite(lcdBrightnessPin, lcdBrightness);
  lc.setIntensity(0, matrixBrightness); // sets brightness (0~15 possible values)

  lcd.begin(16, 2); // set up the LCD's number of columns and rows:

  // initializeLeaderboardEEPROM(); // TODO: initialize if empty
  // initializeSettingsEEPROM();    // TODO: initialize if empty
}

void loop() {
  if (millis() < welcomeMessageDuration) {
    lcd.setCursor(0, 0);
    lcd.print(" Welcome To LED ");
    lcd.setCursor(0, 1);
    lcd.print("  Flappy Bird");
  }
  else {
    // if nameEntered == false => wait for the user to enter a name
    if (!nameEntered) {
      getPlayerName();
    }
    else {
      handleMenu();
    }
  }
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
  for (int i = 0; i <= 9; i++) {
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
    if (playerName[letterPosition] == '_') {
      letter = 123;  // reset letter
    }
    else {
      letter = (int)playerName[letterPosition];
    }
    joyMovedPositionSelection = true;
  }
  if (yValue > maxThreshold && !joyMovedPositionSelection) {
    if (letterPosition < 9) {
      letterPosition++;
    }
    else {
      letterPosition = 0;
    }
    if (playerName[letterPosition] == '_') {
      letter = 123;  // reset letter
    }
    else {
      letter = (int)playerName[letterPosition];
    }
    joyMovedPositionSelection = true;
  }

  if (yValue >= minThreshold && yValue <= maxThreshold) {
    joyMovedPositionSelection = false;
  }
}

void handleMenu() {
  if (joyMoved) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(">");
    lcd.setCursor(1, 0);
    lcd.print(menu[menuIndex]);
    lcd.setCursor(1, 1);
    lcd.print(menu[(menuIndex + 1) % 4]);
    if (menuIndex == 0) {
      displayPlayIcon();
    }
    else if (menuIndex == 1) {
      displayLeaderboardIcon();
    }
    else if (menuIndex == 2) {
      displaySettingsIcon();
    }
    else {
      displayAboutIcon();
    }
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

void handleHighscore() {
  if (joyMovedHighscore) {
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
  }

  if (xValue < minThreshold && !joyMovedHighscore) {
    if (highscoreIndex < highscoreMaxIndex) {
      highscoreIndex++;
    }
    else {
      highscoreIndex = 0;
    }
    joyMovedHighscore = true;
  }

  if (xValue >= minThreshold && xValue <= maxThreshold) {
    joyMovedHighscore = false;
  }
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
      case 3:
        lcd.print("Sound Effects");
        break;
    }
    showSettings = false;
  }
  switch (settingsIndex) {
    case 0:
    {
      for (int row = 0; row < matrixSize; row++) {
        for (int col = 0; col < matrixSize; col++) {
          lc.setLed(0, row, col, 1);
        }
      }
      handleSettingsHorizontalMovement(matrixBrightness, 0, 15, 1);
      lc.setIntensity(0, matrixBrightness);
      int addrMatrixBrightness = sizeof(Player) * 4;
      EEPROM.put(addrMatrixBrightness, matrixBrightness);
      lcd.setCursor(6, 1);
      lcd.print("<");
      lcd.print(matrixBrightness);
      lcd.print(">");
      break;
    }
    case 1:
    {
      handleSettingsHorizontalMovement(lcdContrast, 20, 150, 10);
      analogWrite(lcdContrastPin, lcdContrast);
      int addrLcdContrast = sizeof(Player) * 4 + sizeof(int);
      EEPROM.put(addrLcdContrast, lcdContrast);
      lcd.setCursor(6, 1);
      lcd.print("<");
      lcd.print(lcdContrast);
      lcd.print(">");
      break;
    }
    case 2:
    {
      handleSettingsHorizontalMovement(lcdBrightness, 30, 255, 25);
      analogWrite(lcdBrightnessPin, lcdBrightness);
      int addrLcdBrightnes = sizeof(Player) * 4 + 2 * sizeof(int);
      EEPROM.put(addrLcdBrightnes, lcdBrightness);
      lcd.setCursor(6, 1);
      lcd.print("<");
      lcd.print(lcdBrightness);
      lcd.print(">");
      break;
    }
    case 3:
    {
      handleSoundSettingHorizontalMovement();
      int addrSoundEffects = sizeof(Player) * 4 + 3 * sizeof(int);
      EEPROM.put(addrSoundEffects, soundEffects);
      lcd.setCursor(6, 1);
      lcd.print("<");
      if (soundEffects) {
        lcd.print("On");
      }
      else {
        lcd.print("Off");
      }
      lcd.print(">");
    }
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

void handleSettingsHorizontalMovement(int &intensity, int minIntensity, int maxIntensity, int step) {
  yValue = analogRead(yPin);
  if (yValue < minThreshold && !joyMovedIntensity) {
    if (intensity >= 2 * minIntensity) {
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
      intensity = minIntensity;
    }
    joyMovedIntensity = true;
    showSettings = true;
  }

  if (yValue >= minThreshold && yValue <= maxThreshold) {
    joyMovedIntensity = false;
  } 
}

void handleSoundSettingHorizontalMovement(){
  yValue = analogRead(yPin);
  if ((yValue < minThreshold || yValue > maxThreshold) && !joyMovedSound) {
    soundEffects = !soundEffects;
    joyMovedSound = true;
    showSettings = true;
  }
  if (yValue >= minThreshold && yValue <= maxThreshold) {
    joyMovedSound = false;
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
        // lcd.clear();
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
    lcd.print(">");
    lcd.setCursor(1, 0);
    lcd.print(menu[menuIndex]); // display the current menu option
    lcd.setCursor(1, 1);
    lcd.print(menu[(menuIndex + 1) % 4]);
    gameOver = false;           // mark post game processing as done
    goToMenu = false;           // back to menu command completed
    registeredScore = false;    // reset the flag for the next game
    joyMovedHighscore = true;       // reset the flag for the next highscore access
    showSettings = true;        // reset the flag for the next settings acess
    // display the icons:
    if (menuIndex == 0) {
      displayPlayIcon();
    }
    else if (menuIndex == 1) {
      displayLeaderboardIcon();
    }
    else if (menuIndex == 2) {
      displaySettingsIcon();
    }
    else {
      displayAboutIcon();
    }
  }
}

void displaySettingsIcon() {
  bool settingsAnimation[matrixSize][matrixSize] = {
    {0, 0, 1, 0, 0, 1, 0, 0},
    {0, 1, 1, 0, 0, 1, 1, 0},
    {1, 1, 1, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0}
  };
  writeMatrix(settingsAnimation);
}

void displayPlayIcon() {
  bool playAnimation[matrixSize][matrixSize] = {
    {0, 1, 1, 1, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {0, 1, 1, 1, 0, 0, 0, 0}
  };
  writeMatrix(playAnimation);
}

void displayLeaderboardIcon() {
  bool leaderboardAnimation[matrixSize][matrixSize] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 1, 1, 1, 1},
    {0, 0, 1, 0, 1, 1, 0, 1},
    {1, 1, 1, 0, 1, 1, 0, 1},
    {1, 0, 1, 0, 1, 1, 0, 1},
    {1, 1, 1, 0, 1, 1, 0, 1}
  };
  writeMatrix(leaderboardAnimation);
}

void displayAboutIcon() {
  bool aboutAnimation[matrixSize][matrixSize] = {
    {0, 0, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 0, 0, 1, 1, 0},
    {0, 1, 1, 0, 0, 1, 1, 0},
    {0, 1, 1, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0}
  };
  writeMatrix(aboutAnimation);
}

# pragma endregion

# pragma region CONSISTENCY (LEADERBOARD AND SETTINGS)

void initializeLeaderboardEEPROM() {
  for (int i = 0; i < 3; i++) {
    Player player;
    strcpy(player.name, "Unknown");
    player.score = 0;
    EEPROM.put(eepromAddress, player);
    eepromAddress += sizeof(Player);
  }
}

void initializeSettingsEEPROM() {
  int addr = sizeof(Player) * 4;
  EEPROM.put(addr, 2);
  addr += sizeof(int);
  EEPROM.put(addr, 90);
  addr += sizeof(int);
  EEPROM.put(addr, 128);
  addr += sizeof(int);
  EEPROM.put(addr, true);
}

void getLeaderboardFromEEPROM() {
  for (int i = 0; i < 3; i++) {
    Player player;
    EEPROM.get(eepromAddress, player);
    eepromAddress += sizeof(Player);
    leaderboard[i] = player;
  }
  eepromAddress = 0;
}

void getSettingsFromEEPROM() {
  int addr = sizeof(Player) * 4;
  EEPROM.get(addr, matrixBrightness);
  addr += sizeof(int);
  EEPROM.get(addr, lcdContrast);
  addr += sizeof(int);
  EEPROM.get(addr, lcdBrightness);
  addr += sizeof(int);
  EEPROM.get(addr, soundEffects);
}

# pragma endregion

# pragma region GAMEPLAY

void handlePlay() {
  if (!runningGame && !gameOver) {
    // if the game is not running
    // and we don't have any post game processing to do (gameOver == false)

    if (!countdownStarted) {
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
      if (soundEffects) {
        tone(buzzerPin, 6000, 50);
      }
    }
    handleBirdMovement();  // moves the bird vertically
    if (phase == 1) {
      handleMapMovementPhase1();   // moves the map (aka bird pov) to the left and increases the current score
    }
    else {
      handleMapMovementPhase2();
    }
    increaseDifficulty();
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

void increaseDifficulty() {
  if (phase == 1) {
    if (score != lastScore) {
      if (score % 3) {
        shiftInterval -= 12;  // speed up the gameplay
      }
      lastScore = score;
      if (score % phaseThreshold == 0) {
        // switch phase
        phase = 2;
      }
    }
  }
  else {
    if (score != lastScore) {
      if (score % 3) {
        shiftIntervalPhase2 -= 10;  // speed up the gameplay
      }
      lastScore = score;
      if (score % phaseThreshold == 0) {
        // switch phase
        phase = 1;
      }
    }
  }

}

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
  scoreChanged = true;
  // reset difficulty variables
  shiftInterval = 330;    
  shiftIntervalPhase2 = 300;
  // reset phase
  phase = 1;
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
  if (scoreChanged) {
    lcd.clear();
    scoreChanged = false;
  }
  lcd.setCursor(0, 0);
  lcd.print("Name:");
  for (int i = 0; i < 10; i++) {
    lcd.setCursor(i + 6, 0);
    lcd.print(player.name[i]);
  }
  lcd.setCursor(0, 1);
  lcd.print("Score:");
  lcd.setCursor(7, 1);
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

void handleMapMovementPhase1() {
  generateObstacle();
  if (matrixChanged) {
    updateMatrix();
    matrixChanged = false;
  }
}

void handleMapMovementPhase2() {
  // shift matrix to the left every shiftInterval miliseconds
  if (millis() - lastShiftTime > shiftIntervalPhase2) {
    if (millis() - lastScoreUpdatePhase2 > scoreUpdateIntervalPhase2) {
      score++;
      scoreChanged = true;
      lastScoreUpdatePhase2 = millis();
    }
    shiftMatrix();
    // write the obstacle on the matrix
    for (int i = 0; i < matrixSize; i++) {
      if (i <= y1) {
        matrix[i][matrixSize - 1] = 1;
      }
      else if (i >= y2) {
        matrix[i][matrixSize - 1] = 1;
      }
      else {
        matrix[i][matrixSize - 1] = 0;
      }
    }
    
    if (millis() - lastObstacleTime > newObstacleIntervalPhase2) {
      // get the new obstacle configuration 
      if (lastY2 == 7) {
        // force the decrease of the tunnel height
        y2--; 
        y1--; 
      }
      if (lastY1 == 0) {
        // for the increase of the tunnel height
        y1++;
        y2++; 
      }
      if (y1 == lastY1 && y2 == lastY2) {
        // if no manual increase / decrease has been applied
        // generate new coordinates
        randomSeed(millis() + analogRead(randomPin));
        byte updateOneOrTwo = random(1, 3);
        randomSeed(millis() + analogRead(randomPin));
        int update = random(-1, 2);
        y1 += update;
        y2 += update;
      }
      lastY1 = y1;
      lastY2 = y2;
      lastObstacleTime = millis();
    }

    updateMatrix();
    lastShiftTime = millis();
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
    scoreChanged = true;
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
