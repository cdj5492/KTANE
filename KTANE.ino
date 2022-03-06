#include <LiquidCrystal.h>
#include <stdlib.h>     /* srand, rand */
#include <Wire.h>

using namespace std;

/// 2nd arduino expansion pins ///
#define PWM_3_REG         0
#define PWM_5_REG         1
#define PWM_6_REG         2
#define PWM_9_REG         3
#define PWM_10_REG        4
#define PWM_11_REG        5
#define PWM_OFF           0
#define DDR               6
#define DR                7
#define ALL_IN            B00111111
#define ANALOGUE_IN_0_LOW 8
#define ANALOGUE_PORT     0  // Change this to read from Analogue ports 0...3
#define SLAVE_ADDR        8  // Change this to the Addr of your device

/// LCD interface pins ///
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


/// rest of the bomb ///
#define lidSensorPin A0
const int lightThreshold = 500;

// shift register
#define shiftData  6
#define shiftClk   7
#define shiftLatch 8

uint16_t out = 0;

#define buzzerPin 9
unsigned long buzzerTimerStart = 0;
long strikeBeepDuration = 300;

#define hardModeSelectPin 6
bool hardMode = false;

int timerStart = 0;
unsigned long timerInitTime = 0;
int timerHigh = 0;
int timerLow = 0;

char serialCode[6];

/*
 * bomb states:
 * 1 - bomb is waiting for the game to start
 * 2 - running init
 * 3 - game is running
 * 4 - game is over
 * 5 - wait for restart (lid close)
*/
int bombState = 1;

int strikes = 0; // how many strikes the player has
uint16_t strikeLedPin1 = 1 << 9;
uint16_t strikeLedPin2 = 1 << 10;

// all leds share these cathodes
uint16_t ledRed = 1 << 0;
uint16_t ledGreen = 1 << 7;
uint16_t ledBlue = 1 << 6;

int numLeds = 4;
uint16_t ledAnodes[] = {1 << 5, 1 << 4, 1 << 3, 1 << 2}; // each led has its own anode
// red, blue, green channels (on or off)
bool ledStates[][3] = {
    {0, 0, 0},
    {1, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};

/// modules ///
// button module
int buttonPins[] = {7, 8, 9, 10};
bool red[] = {1, 0, 0};
bool green[] = {0, 1, 0};
bool blue[] = {0, 0, 1};
bool purple[] = {1, 0, 1};

int targetOrder[] = {-1, -1, -1, -1};

// wires module
#define wiresReadPin A7
#define tolerance 15
#define wiresVerifyButton A3
bool previousVerifyState = true;

// morse module
//Array of MorseCode for letters of English Language A to Z
String letters[]={

// A to I
".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
// J to R 
".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
// S to Z
"...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.." 
};
#define dotLength 100
#define dashLength 300
#define letterSpacing 300
#define restartSpacing 1000

void setup() {
    pinMode(hardModeSelectPin, INPUT_PULLUP);
    pinMode(lidSensorPin, INPUT_PULLUP);
    pinMode(buzzerPin, OUTPUT);
    pinMode(wiresVerifyButton, INPUT_PULLUP);

    pinMode(shiftData, OUTPUT);
    pinMode(shiftClk, OUTPUT);
    pinMode(shiftLatch, OUTPUT);

    digitalWrite(strikeLedPin1, 0);
    digitalWrite(strikeLedPin2, 0);
    digitalWrite(shiftData, 0);
    digitalWrite(shiftClk, 0);
    digitalWrite(shiftLatch, 0);

    //Wire.begin();
    //Wire.onReceive(receiveEvent);
    //Wire.onRequest(requestEvent);

    lcd.begin(16, 2);
    //lcd.clear();

    srand(analogRead(lidSensorPin));

    Serial.begin(9600);
}

void loop() {
    switch (bombState) {
        case 1: // wait for start
            if (analogRead(lidSensorPin) < lightThreshold) // wait for lid to open
                bombState = 2;
            break;
        case 2: // running init
            // setup timer
            if(hardMode) {
                timerStart = 60*2 + 30; // 2:30
            } else {
                timerStart = 60*5; // 5:00
            }
            timerInitTime = millis();

            strikes = 0;

            // generate a random serial code
            randomString(6).toCharArray(serialCode, 7);
            for(int i = 0; i < 6; i++) {
                Serial.print(serialCode[i]);
            }
            Serial.println();

            bombState = 3; // start the game
            break;
        case 3: // game is running
            updateTimer();
            showTimer();
            showSerialCode();
            updateBuzzer();
            showStrikes();

            doButtonModule();
            doWiresModule();

            if(timerLow == 0 && timerHigh == 0 || strikes > 2) { // timer ran out of too many strikes
                explode();
            }

            break;
        case 4: // game is over
            lcd.setCursor(0, 0);

            lcd.println("You exploded!");
            Serial.println("You exploded!");

            for(int j = 0; j < 300; j++) {
                explosionSound();
            }
            
            bombState = 5;

            break;
        case 5: // wait for start
            if (analogRead(lidSensorPin) > lightThreshold) // wait for lid to close
                bombState = 1;

            delay(1000);
            break;
    }
}

void receiveEvent() {

}

void requestEvent() {

}

void buzzerOn() {
    analogWrite(buzzerPin, 230);
}

void buzzerOff() {
    analogWrite(buzzerPin, 0);
}

void explosionSound() {
    for(int i = 0; i < 255; i++) {
        analogWrite(buzzerPin, i);
    }
    for(int i = 255; i > 0; i--) {
        analogWrite(buzzerPin, i);
    }
}

void updateBuzzer() {
    if(millis() - buzzerTimerStart < strikeBeepDuration) {
        buzzerOn();
    } else {
        buzzerOff();
    }
}

void addStrike() {
    strikes++;
    buzzerTimerStart = millis();
}

void updateTimer() {
    unsigned long currentTime = millis();
    int elapsedSeconds = (currentTime - timerInitTime)/1000;
    int secondsLeft = timerStart - elapsedSeconds;

    timerLow = secondsLeft % 60;
    timerHigh = secondsLeft / 60;
}

void showTimer() {
    lcd.setCursor(0, 0);

    if (timerHigh < 10) {
        lcd.print("0");
    }
    lcd.print(timerHigh);
    lcd.print(":");
    if (timerLow < 10) { // pad with zero
        lcd.print("0");
    }
    lcd.print(timerLow);
}

void showSerialCode() {
    lcd.setCursor(6, 0);

    for(int i = 0; i < 6; i++) {
        lcd.print(serialCode[i]);
    }
}

void showStrikes() {
    digitalWrite(strikeLedPin1, strikes >= 1);
    digitalWrite(strikeLedPin2, strikes >= 2);
}

void explode() {
    bombState = 4;
}

void doWiresModule() {
    bool verify = digitalRead(wiresVerifyButton);   
    if(previousVerifyState && !verify) {
        // do the check on what the wires should be
        bool failCheck = false;
        if(failCheck) {
            addStrike();
        }
    }
    previousVerifyState = verify;
}

void doButtonModule() {
    

    multiplexLeds();
}

void multiplexLeds() {
    out = ledAnodes[0];
    updateShiftReg();
    //out &= ~ledRed;
    //out &= ~ledBlue;
    //out &= ~ledGreen;
    // for(int i = 0; i < numLeds; i++) {
    //     out |= ledAnodes[i];
    //     out &= ~ledRed;
    //     out &= ~ledGreen;
    //     out &= ~ledBlue;
    //     if (ledStates[i][0]) out |= ledRed;
    //     if (ledStates[i][1]) out |= ledGreen;
    //     if (ledStates[i][2]) out |= ledBlue;
    //     updateShiftReg();
    //     out &= ~ledAnodes[i];
    //     delay(1000);
    // }
}

void updateShiftReg() {
    shiftOut(shiftData, shiftClk, MSBFIRST, out >> 8);
    shiftOut(shiftData, shiftClk, MSBFIRST, out & 0xff);
    digitalWrite(shiftLatch, 1);
    digitalWrite(shiftLatch, 0);
}

String randomString(int len)
{
   char str[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
   String newstr;
   int pos;
   while(newstr.length() != len) {
    pos = ((rand() % (62 - 1)));
    newstr += str[pos];
   }
   return newstr;
}