# Flappy Bird on a Matrix of LEDs using Arduino

When the game starts, you are asked to enter your name by using the joystick (can be blank if you don't care). After pressing down on the joystick to confirm the username, the main menu options are being shown. These are:
* Play
* Highscore (shows the top 3 players with names and scores)
* Settings 
    * LED Matrix Brightness
    * LCD Contrast
    * LCD Brightness (Backlight)
    * Sound Effects (On / Off)
* About (displays the name of the project, the author and the github username)

**To choose an option or to return to the main menu, press down on the joystick.**

## **Play**
When choosing **Play**, a countdown of 3 seconds will start on the matrix. At the end of the countdown, the game will start and the current score will be shown on the lcd. Once you press the **flap** (aka jump) button, the buzzer emits a sound. After the bird collides with an obstacle, you are prompted with your final score and your position in the leaderboard if you made the top 3. To exit this prompt and return to the main menu, press down on the joystick.

## **Highscore**
The top 3 players and their corresponding names and scores are stored in the EEPROM and displayed in this section. **To browse the leadeboard, move the joystick vertically.**

## **Settings**
This option allows you to adjust the LED Matrix Brightness, the LCD Contrast and the LCD Brightness and to turn the Sound Effect On/Off. The settings are stored in the EEPROM.**To browse between settings, move the joystick vertically. To adjust one of them, move the joystick horizontally**.

## **About**
This section displays the name of the project, the author and the github username using a horizontal scrolling text.

## Requirements

<details>

* 8x8 LED Matrix
* MAX7219 Driver
* 16x2 LCD 
* Joystick
* Buzzer (optional)
* Button (optional - you can also use the button on the joystick)
* All the wires available in your country
* Resistors (330Ω for Backlight Anode and 10kΩ for driver pin 18)
* Capacitors (we use the capacitors to filter circuit noise)
    * 1 electrolytic capacitor of 10 μF
    * 1 ceramic capacitor of 104 pF


</details>

## Setup Pictures

### Week 1

<details>

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup0week1.jpeg)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup1week1.jpeg)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup2week1.jpeg)

</details>

### Week 2

<details>

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup0week2.jpeg)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup1week2.jpeg)

</details>


TODO:
* obstacle tunnel
* modify increase step and limit settings values
* master comment

