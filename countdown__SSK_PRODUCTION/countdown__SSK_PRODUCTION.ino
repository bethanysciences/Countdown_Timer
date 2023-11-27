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
  TM1637 based 0.56" high 7-segment LED display
  Passive Buzzer Module
  2 momentary pushbutton switches

  IMPROVEMENTS TO MAKE:
  - reset MCU function (15-min and 30-min press together to 15+seconds?)
  - dim display function
  - vary alarm volume
--------------------------------------------------------------------------------------------------------*/

#include "JC_Button.h"                                  // v2.1.2 https://github.com/JChristensen/JC_Button
#include "Adafruit_seesaw.h"                            // v1.7.0 https://github.com/adafruit/Adafruit_Seesaw
#include "CountDown.h"                                  // v0.3.1 https://github.com/RobTillaart/CountDown
#include "TM1637.h"                                     // v0.3.7 https://github.com/RobTillaart/TM1637_RT

int buzzPin = 4;                                        // Passive Buzzer Module fire pin (GREEN Wire)

/*------------------------------------------------------------------------------------------------------*/
/*                    Instantiate JC Button Library and assign buttons                                  */
/*------------------------------------------------------------------------------------------------------*/

Button set15(15);                                       // PRODUCTION 15-min button to pin 15 BRN wire
Button set30(14);                                       // PRODUCTION 30-min button to pin 14 WHT wire
int32_t time15 = 900;                                   // PRODUCTION 900-secs = 15 mins
int32_t time30 = 1800;                                  // PRODUCTION 1800-secs = 30 mins

// Button set15(10);                                       // PROTOTYPE 5-sec button to pin 10
// Button set30(6);                                        // PROTOTYPE 30-min button to pin 6
// int32_t time15 = 5;                                     // PROTOTYPE 5-secs
// int32_t time30 = 1800;                                  // PROTOTYPE 1800-secs = 30 mins


/*------------------------------------------------------------------------------------------------------*/
/*             Adafruit Encoder breakout board instantiation (uses separate SAMD09 MCU)                 */
/*------------------------------------------------------------------------------------------------------*/
#define SS_SWITCH     24                                // switch pin on SAM09 encoder breakout board
#define SEESAW_ADDR   0x36                              // i2c address (default - change for multiple)
                                                        // VIN (5VDC) - RED wire
                                                        // GND - BLACK wire
                                                        // Serial Clock (SCL) I2C YELLOW wire
                                                        // Serial Data (SDA) I2C BLUE wire
Adafruit_seesaw ss;                                     // instantiate encoder board / functions as 'ss'
int switchState;                                        // current input pin reading
int lastSwitchState = LOW;                              // previous input pin reading
unsigned long lastDebounceTime = 0;                     // last time the output pin toggled
unsigned long debounceDelay = 50;                       // debounce encoder switch delay (ms)
int32_t encoder_position;

/*                Instantiate Rob Tillaart's CountDown Library anb associated variables                 */
CountDown cd(CountDown::SECONDS);                       // instantiate timer to seconds as 'cd'

/*                Instantiate Rob Tillaart's 4-digit TM1637 based 7-segment display library             */
TM1637 tm;                                              // instantiate display as 'tm'
int CLK = 2;                                            // PRODUCTION Clock Input YELLOW wire to pin 2
int DIO = 3;                                            // PRODUCTION Data I/O BLUE wire to pin 3
// int CLK = 3;                                            // PROTOTYPE Clock Input YELLOW wire to pin 3
// int DIO = 2;                                            // PROTOTYPE Data I/O BLUE wire to pin 2
int DIGITS = 4;                                         // this display is 4-digits

/*------------------------------------------------------------------------------------------------------*/
/*                                   Application setup - runs once                                      */
/*------------------------------------------------------------------------------------------------------*/
void setup() {
    pinMode(buzzPin, OUTPUT);                           // fire up buzzer module
    tone(buzzPin, 2000, 500);                           // indicate startup (2000hz for 500ms)

    set15.begin();                                      // fire up 15-minute switch
    set30.begin();                                      // fire up 30-minute switch

    ss.begin(SEESAW_ADDR);
    ss.pinMode(SS_SWITCH, INPUT_PULLUP);                // fire up encoder switch pin
    delay(10);                                          // give MCU a chance to catch up
    ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);  // set switch interrupt
    ss.enableEncoderInterrupt();                        // fire up encoder interrupt
    encoder_position = ss.getEncoderPosition();

    tm.begin(CLK, DIO, DIGITS);                         // fire up and assign pins 4-digit display
    tm.setBrightness(7);                                // set brightness (0 - 7)
    tm.displayClear();                                  // clear display

    cd.setResolution(CountDown::SECONDS);               // set countdown timer to track seconds
}


/*------------------------------------------------------------------------------------------------------*/
/*                                   Application loop - runs forever                                    */
/*------------------------------------------------------------------------------------------------------*/

void loop() {
    if ((PollEncoderSwitch() == true) && (cd.isRunning() == true)) {
        cd.stop();
        tm.displayTime(0, 0, false);
    }       

    int32_t new_position = ss.getEncoderPosition();     // did encoder position change since last loop?
    if ((cd.isStopped() == true) && (encoder_position != new_position)) {
        ss.setEncoderPosition(45);
        while (PollEncoderSwitch() == false) {
            if (ss.getEncoderPosition() < 1) ss.setEncoderPosition(99);
            if (ss.getEncoderPosition() > 99) ss.setEncoderPosition(1);
            tm.displayTime(ss.getEncoderPosition(), 0, true);
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
    
    uint8_t mm = cd.remaining() / 60;
    uint8_t ss = cd.remaining() - mm * 60;
    tm.displayTime(mm, ss, true);

    if ((cd.remaining() <= 2) && (cd.isRunning() == true)) {
        for(int x = 0; x < 3; x++){
            uint8_t mm = cd.remaining() / 60;
            uint8_t ss = cd.remaining() - mm * 60;
            tm.displayTime(mm, ss, true);
            tone(buzzPin, 2000, 500);                   // 2.0khz for 500ms
            delay(500);
            tone(buzzPin, 2100, 500);                   // 2.1khz for 500ms
            delay(500);
        }
        cd.stop();
    }

    delay(10);                                          // let's not overwhelm the MCU
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

