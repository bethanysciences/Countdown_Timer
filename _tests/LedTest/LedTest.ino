#include "SevSeg.h"
SevSeg sevseg;

byte digitPins[] = {10,11,12,13};           // digit pin order 1234
byte segmentPins[] = {3,4,5,6,7,8,9,2};     // segment pin order ABCDEFGdp

void setup() {
    sevseg.begin(COMMON_CATHODE,            // COMMON_CATHODE or COMMON_ANODE
                 4,                         // number of digits 
                 digitPins,                         
                 segmentPins,
                 true,                      // resistors onsegments 
                 false,                     // update with delays 
                 false,                     // leading zeros 
                 true);                     // disable decimal point
}

void loop() {
    static unsigned long timer = millis();
    static int deciSeconds = 0;
    if (millis() - timer >= 100) {
        timer += 100;
        deciSeconds++;                      // 100 milliSeconds is equal to 1 deciSecond
        if (deciSeconds == 10000) deciSeconds=0;
        sevseg.setNumber(deciSeconds);
    }
    sevseg.refreshDisplay();                // Must run repeatedly
}
