/*--------------------------------------------------------------------------------------------------------------------
  Countdown timer for U.S. Coast Guard Communications Center Console. Arduino based application to count
  down from 15 or 30-minutes, standard patrol communications check-in intervals, or set any time between
  1 and 99 minutes. 15 or 30-minute times selected and count down start on a button press.

  By Bob Smith
  https://github.com/bethanysciences/countdown
  28NOV2023

  Components used (see readme for additional documentation)
  Arduino Nano Every https://content.arduino.cc/assets/Pinout-NANOevery_latest.pdf
    - ATMega48095 Microcontroller 
    - 7-21VDC VIN	
    - 20MHz Clock Speed	
    - ATSAMD11D14A based USB interface
    - GB4943, WEE, RCM, RoHS, CE, FCC, UKCA, REACH Conformities https://docs.arduino.cc/certifications
  Adafruit I2C Rotary Encoder Breakout https://www.adafruit.com/product/4991
  Adafruit Rotary Encoder https://www.adafruit.com/product/377
  Adafruit MicroSD card breakout board+ https://www.adafruit.com/product/254
  Adafruit STEMMA Piezo Driver Amp - PAM8904 https://www.adafruit.com/product/5791
  Passive Buzzer Module https://www.adafruit.com/product/1739
  Adafruit 4-digit 7-segment I2C display RED https://www.adafruit.com/product/878
  2 Sparkfun 4-character 14-segment I2C displays GREEN https://www.sparkfun.com/products/16916
  3 momentary pushbutton switches https://www.amazon.com/WMYCONGCONG-Waterproof-Momentary-Button-Switch/dp/B07S1MNB8C

  Asset Labels come from plain text file assets.txt on standard FAT16 or FAT32 formatted microSD card.
  Labels are 8 alpha-numeric characters maximum separated by commas and no spaces between labels one 1 line
  There is no limiet to the number of loadable labels
  example: CG47211,CG47246,CG29291,CG27111,OTHER, <<---- this last comma signifies end of labels
  Official SD Card Association MACOS/Windows formatter https://www.sdcard.org/downloads/formatter/

---------------------------------------------------------------------------------------------------------------------*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>                             // v1.2.4 https://www.arduino.cc/reference/en/libraries/sd/
#include "JC_Button.h"                      // v2.1.2 https://github.com/JChristensen/JC_Button
#include <SparkFun_Alphanumeric_Display.h>  // v2.2.9 https://github.com/sparkfun/SparkFun_Alphanumeric_Display_Arduino_Library
#include <Adafruit_GFX.h>                   // v1.11.9 https://github.com/adafruit/Adafruit-GFX-Library
#include "Adafruit_LEDBackpack.h"           // v1.4.1 https://github.com/adafruit/Adafruit_LED_Backpack
#include "Adafruit_seesaw.h"                // v1.7.5 https://github.com/adafruit/Adafruit_Seesaw
#include "CountDown.h"                      // v0.3.2 https://github.com/RobTillaart/CountDown


/*------------------------------------------------------------------------------------------------------*/
/*                    Instantiate Piezo Amp & Buzzer Module                                                 */
/*------------------------------------------------------------------------------------------------------*/
int buzzPin = 14;                           // PAM8904 Piezo Amp signal pin


/*------------------------------------------------------------------------------------------------------*/
/*                    Instantiate SD Card                                                                    */
/*------------------------------------------------------------------------------------------------------*/
File myFile;


/*------------------------------------------------------------------------------------------------------*/
/*                    Instantiate JC Button Library and assign pins                                  */
/*------------------------------------------------------------------------------------------------------*/
Button set15(17);                                       // 30-min button - BLUE
Button set30(16);                                       // 15-min button - YELLOW
Button assetCycle(15);                                  // Asset cycle button - GREEN
int32_t time15 = 900;                                   // 900-secs = 15 mins
int32_t time30 = 1800;                                  // 1800-secs = 30 mins
int asset;                                              // assigned asset slot
int tot_assets;
char ASSETS[10][8];

/*------------------------------------------------------------------------------------------------------*/
/*              Instantiate Adafruit Encoder breakout board (uses separate SAMD09 MCU)                  */
/*------------------------------------------------------------------------------------------------------*/
#define SS_SWITCH     24                                // switch pin on SAM09 encoder breakout board
#define SEESAW_ADDR   0x36                              // i2c address
Adafruit_seesaw ss;                                     // instantiate encoder board / functions as 'ss'
int switchState;                                        // current input pin reading
int lastSwitchState = LOW;                              // previous input pin reading
unsigned long lastDebounceTime = 0;                     // last time the output pin toggled
unsigned long debounceDelay = 50;                       // debounce encoder switch delay (ms)
int32_t encoder_position;


/*------------------------------------------------------------------------------------------------------*/
/*                Instantiate Rob Tillaart's CountDown Library and associated variables                 */
/*------------------------------------------------------------------------------------------------------*/
CountDown cd(CountDown::SECONDS);                       // instantiate timer to seconds as 'cd'


/*------------------------------------------------------------------------------------------------------*/
/*                Instantiate Adafruits Seven Segment I2C Backpack Library and associated variables     */
/*------------------------------------------------------------------------------------------------------*/
Adafruit_7segment sevseg0   = Adafruit_7segment();      // instantiate display as 'sevseg0'


/*------------------------------------------------------------------------------------------------------*/
/*     Instantiate Sparkfun's 14-segment I2C Alph-numberic display Library and associated variables     */
/*------------------------------------------------------------------------------------------------------*/
HT16K33 display;                                         // instantiate combining 2 displays as 'sdisplay'


/*------------------------------------------------------------------------------------------------------*/
/*                                                                                                      */
/*                                   Application setup - runs once                                      */
/*                                                                                                      */
/*------------------------------------------------------------------------------------------------------*/
void setup() {
    Wire.begin();
    pinMode(buzzPin, OUTPUT);                           // fire up buzzer module
    tone(buzzPin, 330, 250);                            // sound startup (330hz for 250ms)

    SD.begin(10);                                       // fire up SPI SD card w/ CS pin 10
    readSDFile();                                       // branch off to read asset lebels on microSD

    ss.begin(SEESAW_ADDR);                              // fire up encoder
    ss.pinMode(SS_SWITCH, INPUT_PULLUP);                // fire up encoder switch pin
    delay(10);                                          // give MCU a chance to catch up
    ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);  // set switch interrupt
    ss.enableEncoderInterrupt();                        // fire up encoder interrupt
    encoder_position = ss.getEncoderPosition();

    sevseg0.begin(0x77);                                // Adafruit 7-segment display on I2C addr 0x77
    display.begin(0x71, 0x70);                          // Sparkfun displays on i2C addrs left 0x71 / right 0x70

    set15.begin();                                      // start 15-minute switch
    set30.begin();                                      // start 30-minute switch
    assetCycle.begin();                                 // start up asset cycle switch
    display.print("<-ASSET");                           // Inintiate 'display' with "Asset"

    cd.setResolution(CountDown::SECONDS);               // set countdown timer to track seconds
    cd.stop();                                          // double-check timer is stopped
}



/*------------------------------------------------------------------------------------------------------*/
/*                              Read Assets from SD Card - runs once                                    */
/*        File Name assets.txt                                                                          */
/*        entries comma separated example CG27321,CG47450,CG29117,CG27189,                              */
/*------------------------------------------------------------------------------------------------------*/
void readSDFile() {
    char line[8];
    myFile = SD.open("assets.txt");
    if (myFile) {
        while (myFile.available()) {
            memset(line, '\0', sizeof(line));         // clear entry
            myFile.readBytesUntil(',', line, sizeof(line));
            strcpy(ASSETS[asset],line);
            asset++;
        }
        myFile.close();
    }
    else display.print("error");
    tot_assets = asset -1;                            // drop last trash entry
}



/*------------------------------------------------------------------------------------------------------*/
/*                                                                                                      */
/*                                   Application loop - runs forever                                    */
/*                                                                                                      */
/*------------------------------------------------------------------------------------------------------*/

void loop() {
    assetCycle.read();
    if(assetCycle.wasReleased()) {
      asset ++;
      if (asset >= tot_assets) asset = 0;
        display.print(ASSETS[asset]);
    }

    if ((PollEncoderSwitch() == true) && (cd.isRunning() == true)) {
        cd.stop();
        sevseg0.print("STOP");
        sevseg0.drawColon(false);
        sevseg0.writeDisplay();
    }       

    int32_t new_position = ss.getEncoderPosition();     // did encoder position change since last loop?
    if ((cd.isStopped() == true) && (encoder_position != new_position)) {
        ss.setEncoderPosition(45);
        while (PollEncoderSwitch() == false) {
            if (ss.getEncoderPosition() < 1) ss.setEncoderPosition(99);
            if (ss.getEncoderPosition() > 99) ss.setEncoderPosition(1);
            sevseg0.print(ss.getEncoderPosition() * 100);
            sevseg0.drawColon(true);
            sevseg0.writeDisplay();    
            
            set15.read();
            if(set15.wasReleased()) {
                cd.start(time15);
                encoder_position = new_position;
                return;
            }

            set30.read();
            if(set30.wasReleased()) {
                cd.start(time30);
                encoder_position = new_position;
                return;
            }

            cd.start(ss.getEncoderPosition() * 60);
            encoder_position = new_position;
        }
    }

    set15.read();
    if(set15.wasReleased()) cd.start(time15);

    set30.read();
    if(set30.wasReleased()) cd.start(time30);
    
    int mm = cd.remaining() / 60;
    int ss = cd.remaining() - mm * 60;
    int displayValue = (mm * 100) + ss;
    sevseg0.print(displayValue);
    sevseg0.drawColon(true);
    sevseg0.writeDisplay();

    if ((cd.remaining() <= 2) && (cd.isRunning() == true)) {
        for(int x = 0; x < 3; x++){
          int mm = cd.remaining() / 60;
          int ss = cd.remaining() - mm * 60;
          int displayValue = (mm * 100) + ss;
          sevseg0.print(displayValue);
          sevseg0.drawColon(true);
          sevseg0.writeDisplay();            
          tone(buzzPin, 330, 500);                   // 2.0khz for 500ms
          delay(500);
          tone(buzzPin, 523, 500);                   // 2.1khz for 500ms
          delay(500);
        }
        cd.stop();
    }

    delay(10);                                        // let's not overwhelm the MCU
}

bool PollEncoderSwitch() {
    int encoderSW = ss.digitalRead(SS_SWITCH);
    if (encoderSW != lastSwitchState) lastDebounceTime = millis();
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (encoderSW != switchState) {
            switchState = encoderSW;
            if (switchState == HIGH) return true;
        }
    }
    lastSwitchState = encoderSW;
    return false;
}
