# Flappy Bird on a Matrix of LEDs using Arduino

When the game starts, you are asked to enter your name by using the joystick (can be blank if you don't care). After pressing down on the joystick to confirm the username, the main menu options are being shown. These are:
* Play
* Highscore (TODO - the leadeboard exists in memory, it just needs to be displayed)
* Settings  (TODO)
* About     (TODO)

To choose an option, press down on the joystick.

When choosing **Play**, a countdown of 3 seconds will start on the matrix. At the end of the countdown, the game will start and the current score will be shown on the lcd. Once you press the **flap** (aka jump) button, the buzzer emits a sound. After the bird collides with an obstacle, you are prompted with your final score and your position in the leaderboard if you made the top 3. To exit this prompt and return to the main menu, press down on the joystick. The top 3 players and their corresponding names and scores are stored in the EEPROM.

## Setup
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup0.jpeg)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup1.jpeg)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup3.jpeg)