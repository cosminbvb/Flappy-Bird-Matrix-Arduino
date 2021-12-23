# Flappy Bird on a Matrix of LEDs using Arduino

Was flappy bird not frustrating enough already? Have you ever wanted to play flappy bird using an Arduino and 64 LEDs as a map? Me neither. But here it is anyway.

## Content

* [Requirements](#req)
* [Understanding the menu](#menu)
* [Understanding the game](#gameplay)
* [Setup Photos & Wiring Diagrams](#setup)
* [Demo](#demo)
* [Issues and TODO List](#next)

<a name="req"/>

## Requirements

* 8x8 LED Matrix
* MAX7219 Driver
* 16x2 LCD 
* Joystick
* Buzzer (optional)
* Button (optional - you can also use the button on the joystick)
* All the wires available in your country
* Resistors
    * 330Ω for Backlight Anode
    * 100Ω for the Buzzer
    * 10kΩ for driver pin 18
* Capacitors (we use the capacitors to filter circuit noise)
    * 1 electrolytic capacitor of 10 μF
    * 1 ceramic capacitor of 104 pF


<a name="menu"/>

## Understanding the menu

When the game starts, you are asked to enter your name by using the joystick (can be blank if you don't care). Horizontal movement changes the selected position while vertical movement changes the letter on the current position. After pressing down on the joystick to confirm the username, the main menu options are being shown (as plain text on the LCD and as icons on the LED Matrix). These are:
* Play
* Highscore (shows the top 3 players with names and scores)
* Settings 
    * LED Matrix Brightness
    * LCD Contrast
    * LCD Brightness (Backlight)
    * Sound Effects (On / Off)
* About (displays the name of the project, the author and the github username)

**To choose an option or to return to the main menu, press down on the joystick.**

### **Play**
When choosing **Play**, a countdown of 3 seconds will start on the matrix. At the end of the countdown, the game will start and the name together with the current score will be shown on the lcd. Once you press the **flap** (aka jump) button, the buzzer emits a sound. After the bird collides with an obstacle, you are prompted with your final score and your position in the leaderboard if you made the top 3. To exit this prompt and return to the main menu, press down on the joystick.

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/playMenu.jpeg)

### **Highscore**
The top 3 players and their corresponding names and scores are stored in the EEPROM and displayed in this section. **To browse the leadeboard, move the joystick vertically.**

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/highscoreMenu.jpeg)

### **Settings**
This option allows you to adjust the LED Matrix Brightness, the LCD Contrast and the LCD Brightness and to turn the Sound Effect On/Off. The settings are stored in the EEPROM.**To browse between settings, move the joystick vertically. To adjust one of them, move the joystick horizontally**.

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/settingsMenu.jpeg)

### **About**
This section displays the name of the project, the author and the github username using a horizontal scrolling text.

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/aboutMenu.jpeg)

<a name="gameplay"/>

## Understanding the game

The game consists of 2 map phases that are being switched between each other every ```phaseThreshold``` points.

### Phase 1 (Standard flappy bird):
Obstacles are far away from each other (to be precise, they are being spawned in every ```obstacleInterval``` miliseconds) with randomly generated shapes (with a minimum gap size in order to be passable). Difficulty increases every 3 points by speeding up the map movement (```shiftInterval``` - the time interval at which the matrix is being shifted to the left and therefore creating the map movement - decreases).

### Phase 2 (Tunnel):
Obstacles are glued together, creating the image of a tunnel of obstacles. Here, each obstacle is also randomly generated, but it's constrained to have a deviation of maximum 1 cell from the previous obstacle, while also maintaing the same gap size. Here, the difficulty increases in a similar manner, but the speed-up process is slower, due to the fact that the tunnel of obstacles is already significantly more difficult.
 
<a name="setup"/>

## Setup Photos and Wiring Diagrams

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup0.jpeg)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/setup1.jpeg)


Driver <-> Arduino Connection

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/driver_to_arduino.png)

Matrix <-> Driver Connection

![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/matrix_to_driver.png)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/matrix_table_0.png)
![](https://github.com/cosminbvb/Flappy-Bird-Matrix-Arduino/blob/main/images/matrix_table_1.png)

<a name="demo"/>

## Demo
You can watch a quick demo [here](https://www.youtube.com/watch?v=VWjmkaziLCA).

<a name="next"/>

### Issues

* Flap button noise

### TODO

* Produce and add the cinematic masterpiece
* Add a master comment
* Refactor and cleanup