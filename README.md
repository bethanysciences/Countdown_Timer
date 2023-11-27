# U.S. Coast Guard Countdown Timer

By [Bob Smith](rpsmithii@mac.com)
[repository](https://github.com/bethanysciences/countdown)

Countdown timer for U.S. Coast Guard Communications Center Console. Arduino based application to count down from 15 or 30-minutes, standard patrol communications check-in intervals, or set any time between 1 and 99 minutes. 15 or 30-minute times selected and count down start on a button press.

## Features

- Set and start 15 or 30-minute countdown using single button press
- Dial in 1 to 99-minutes using rotary encoder
- Outputs minutes and seconds to 4-digit 7-segment LED display

## Components Used

- [Arduino Nano Every MCU](https://content.arduino.cc/assets/Pinout-NANOevery_latest.pdf)
  - ATMega48095 Microcontroller
  - 7-21VDC VIN
  - 20MHz Clock Speed
  - ATSAMD11D14A based USB interface
  - [GB4943, WEE, RCM, RoHS, CE, FCC, UKCA, REACH Conformities](https://docs.arduino.cc/certifications)
- [Adafruit I2C Rotary Encoder Breakout](https://www.adafruit.com/product/4991)
- [Adafruit MicroSD card breakout board+](https://www.adafruit.com/product/254)
- [Adafruit Rotary Encoder](https://www.adafruit.com/product/377)
- [Adafruit 4-digit 7-segment I2C display RED](https://www.adafruit.com/product/878)
- [Adafruit STEMMA Piezo Driver Amp - PAM8904](https://www.adafruit.com/product/5791)
- [Adafruit passive Buzzer Module](https://www.adafruit.com/product/1739)
- [2 Sparkfun 4-character 14-segment I2C displays GREEN](https://www.sparkfun.com/products/16916)
- [3 momentary pushbutton switches](https://www.amazon.com/WMYCONGCONG-Waterproof-Momentary-Button-Switch/dp/B07S1MNB8C)

## Compiler

- [Using Arduino 2.1.0 IDE](https://github.com/arduino/arduino-ide)
- [Arduino platform-specification](https://arduino.github.io/arduino-cli/latest/platform-specification/)

## Supported Core(s)

- [Arduino megaAVG Boards 1.8.8](https://github.com/arduino/ArduinoCore-megaavr)

## Libraries used

- [Arduino's SD card manager v1.2.4](https://www.arduino.cc/reference/en/libraries/sd/)
- [J. Christensen's button management v2.1.2](https://github.com/JChristensen/JC_Button)
- [SparkFun's alphanumeric display v2.2.9](https://github.com/sparkfun/SparkFun_Alphanumeric_Display_Arduino_Library)
- [Adafruit's GFX v1.11.9](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit's LED backpack v1.4.1](https://github.com/adafruit/Adafruit_LED_Backpack)
- [Adafruit's seesaw v1.7.5](https://github.com/adafruit/Adafruit_Seesaw)
- [Rob Tillaart's countdown timer v0.3.2](https://github.com/RobTillaart/CountDown)

Asset Labels come from plain text file assets.txt on standard FAT16 or FAT32 formatted microSD card. Labels are 8 alpha-numeric characters maximum separated by commas and no spaces between labels one 1 line There is no limit to the number of loadable labels
example: CG47211,CG47246,CG29291,CG27111,OTHER, <<---- this last comma signifies end of labels

[Official SD Card Association MACOS/Windows formatter](https://www.sdcard.org/downloads/formatter/)
