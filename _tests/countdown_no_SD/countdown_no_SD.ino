/*------------------------------------------------------------------------------------------------------
  Countdown timer for U.S. Coast Guard Communications Center Console. Arduino based application to count
  down from 15 or 30-minutes, standard patrol communications check-in intervals, or set any time between
  1 and 99 minutes. 15 or 30-minute times selected and count down start on a button press.

  By Bob Smith
  https://github.com/bethanysciences/countdown

  Components used (see readme for additional documentation)
  Arduino Nano Every https://content.arduino.cc/assets/Pinout-NANOevery_latest.pdf
    - ATMega48095 Microcontroller 
    - 7-21VDC VIN	
    - 20MHz Clock Speed	
    - ATSAMD11D14A based USB interface
    - GB4943, WEE, RCM, RoHS, CE, FCC, UKCA, REACH Conformities https://docs.arduino.cc/certifications
  Adafruit I2C Rotary Encoder Breakout
  Adafruit Rotary Encoder
  Passive Buzzer Module
  Adafruit 4-digit 7-segment I2C display
  2 Sparkfun 4-character 14-segment I2C displays
  3 momentary pushbutton switches

--------------------------------------------------------------------------------------------------------*/

#include <Wire.h>
#include "JC_Button.h"                                  // v2.1.2 https://github.com/JChristensen/JC_Button
#include <SparkFun_Alphanumeric_Display.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_seesaw.h"                            // v1.7.0 https://github.com/adafruit/Adafruit_Seesaw
#include "CountDown.h"                                  // v0.3.1 https://github.com/RobTillaart/CountDown


/*                    Instantiate Passive Buzzer Module                                                 */
int buzzPin = 14;                                        // Passive Buzzer Module fire pin (WHITE Wire)
/*
This module is ideally suited to adding noise to your project while functioning with microcontrollers such 
as the Arduino. This module lets you respond to programmatic changes with a lovely annoying buzzer that can
be altered over a range of frequencies to ensure maximum irritation. To operate the I/O pin must receive a 
square wave to trigger the buzzer. This can be produced within all popular microcontrollers.
Passive internal sources do not have oscillating sources, so if the DC signal cannot be made to sing.
It has to be driven by a 2K-5K square wave (PWM).
So these are active buzzers, but with GND and VCC hooked up.. a logic low on I/O sounds the buzzer, and 
a high (or disconnect) silences. So it does not work well with tone() (unless you recode tone to be silent 
with a logic-HIGH). If you want the opposite buzzer logic (high makes tone), then hook GND and I/O together
to GND, and sound buzzer with a HIGH to the Vcc line.
If you are using the Arduino IDE to program the microcontroller used with this, there are libraries of 
"melodies" that you can use to make the sound less obnoxious (so your spouse doesn't use their meat tenderize
mallet to silence the alarm). Pleased with the product.
*/

/*------------------------------------------------------------------------------------------------------*/
/*                    Instantiate JC Button Library and assign buttons                                  */
/*------------------------------------------------------------------------------------------------------*/

Button set30(15);                                       // 15-min button - GREEN
Button set15(16);                                       // 30-min button - ORANGE
Button assetCycle(17);                                  // Asset cycle button - WHITE
int32_t time15 = 900;                                   // 900-secs = 15 mins
int32_t time30 = 1800;                                  // 1800-secs = 30 mins
int asset;                                              // assigned asset slot
int tot_assets = 6;
char ASSETS[6][9] = {"CG47246",
                     "CG47227",
                     "CG47311",
                     "CG29291",
                     "CG27114",
                     "Other",
                     };

/*------------------------------------------------------------------------------------------------------*/
/*             Adafruit Encoder breakout board instantiation (uses separate SAMD09 MCU)                 */
/*------------------------------------------------------------------------------------------------------*/
#define SS_SWITCH     24                                // switch pin on SAM09 encoder breakout board
#define SEESAW_ADDR   0x36                              // i2c address
Adafruit_seesaw ss;                                     // instantiate encoder board / functions as 'ss'
int switchState;                                        // current input pin reading
int lastSwitchState = LOW;                              // previous input pin reading
unsigned long lastDebounceTime = 0;                     // last time the output pin toggled
unsigned long debounceDelay = 50;                       // debounce encoder switch delay (ms)
int32_t encoder_position;

/*                Instantiate Rob Tillaart's CountDown Library anb associated variables                 */
CountDown cd(CountDown::SECONDS);                       // instantiate timer to seconds as 'cd'

/*                Instantiate Adafruits Seven Segment I2C Backpack Library and associated variables     */
Adafruit_7segment sevseg0   = Adafruit_7segment();

/*     Instantiate Sparkfun's 14-segment I2C Alph-numberic display Library and associated variables     */
HT16K33 display;


/*------------------------------------------------------------------------------------------------------*/
/*                                   Application setup - runs once                                      */
/*------------------------------------------------------------------------------------------------------*/
void setup() {
    Wire.begin();
    pinMode(buzzPin, OUTPUT);                           // fire up buzzer module
    tone(buzzPin, 2000, 500);                           // indicate startup (2000hz for 500ms)

    ss.begin(SEESAW_ADDR);
    ss.pinMode(SS_SWITCH, INPUT_PULLUP);                // fire up encoder switch pin
    delay(10);                                          // give MCU a chance to catch up
    ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);  // set switch interrupt
    ss.enableEncoderInterrupt();                        // fire up encoder interrupt
    encoder_position = ss.getEncoderPosition();
    //ss.setEncoderPosition(45);

    sevseg0.begin(0x77);
    display.begin(0x70, 0x71);

    set15.begin();                                      // fire up 15-minute switch
    set30.begin();                                      // fire up 30-minute switch
    assetCycle.begin();                                 // fire up asset cycle switch
    display.print("<-ASSET");                           // start diaplay
    //sevseg0.print("4500");                              // start diaplay
    //sevseg0.drawColon(true);                            // With
    //sevseg0.writeDisplay();

    cd.setResolution(CountDown::SECONDS);               // set countdown timer to track seconds
    cd.stop();
}


/*------------------------------------------------------------------------------------------------------*/
/*                                   Application loop - runs forever                                    */
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
          tone(buzzPin, 2000, 500);                   // 2.0khz for 500ms
          delay(500);
          tone(buzzPin, 2100, 500);                   // 2.1khz for 500ms
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

