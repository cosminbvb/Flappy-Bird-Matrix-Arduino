# Flappy Bird on a Matrix of LEDs using Arduino

When the game starts, you are asked to enter your name by using the joystick (can be blank if you don't care). After pressing down on the joystick to confirm the username, the main menu options are being shown. These are:
* Play
* Highscore (shows the top 3 players with names and scores)
* Settings 
    * LED Matrix Brightness
    * LCD Contrast
    * LCD Brightness (Backlight)
* About (displays the name of the project, the author and the github username)

**To choose an option or to return to the main menu, press down on the joystick.**

## **Play**
When choosing **Play**, a countdown of 3 seconds will start on the matrix. At the end of the countdown, the game will start and the current score will be shown on the lcd. Once you press the **flap** (aka jump) button, the buzzer emits a sound. After the bird collides with an obstacle, you are prompted with your final score and your position in the leaderboard if you made the top 3. To exit this prompt and return to the main menu, press down on the joystick.

## **Highscore**
The top 3 players and their corresponding names and scores are stored in the EEPROM and displayed in this section. **To browse the leadeboard, move the joystick vertically.**

## **Settings**
This option allows you to adjust the LED Matrix Brightness, LCD Contrast and Brightness. **To browse between settings, move the joystick vertically. To adjust one of them, move the joystick horizontally**.

## **About**
This section displays the name of the project, the author and the github username using a horizontal scrolling text.

## Setup Pictures

### Week 1

<details>

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup0.jpeg)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup1.jpeg)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup3.jpeg)

</details>

### Week 2

<details>

To do.

</details>