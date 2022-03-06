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
#define ANALOGUE_IN_0_HIGH 9
#define ANALOGUE_IN_1_LOW 10
#define ANALOGUE_IN_1_HIGH 11
#define ANALOGUE_IN_2_LOW 12
#define ANALOGUE_IN_2_HIGH 13
#define ANALOGUE_IN_3_LOW 14
#define ANALOGUE_IN_3_HIGH 15
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
#define shiftClk   8
#define shiftLatch 7

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
//uint16_t strikeLedPin1 = 1 << 9;
//uint16_t strikeLedPin2 = 1 << 10;

// all leds share these cathodes
// uint16_t ledRed = 1 << 0;
// uint16_t ledGreen = 1 << 7;
// uint16_t ledBlue = 1 << 6;

uint16_t ledRed = 0;
uint16_t ledGreen = 7;
uint16_t ledBlue = 6;

int numLeds = 10;
//uint16_t ledAnodes[] = {1 << 5, 1 << 4, 1 << 3, 1 << 2}; // each led has its own anode
uint16_t ledAnodes[] = {5, 4, 3, 2, 1, 14, 13, 12, 11, 10};
// red, blue, green channels (on or off)
bool ledStates[][3] = {
    {1, 0, 0},
    {0, 0, 1},
    {0, 1, 0},
    {1, 0, 1},
    {0, 0, 0},
    {1, 0, 0},
    {0, 0, 0},
    {1, 0, 0},
    {0, 0, 0},
    {1, 0, 0}
};

int binaryNumber = 0;

/// modules ///
// button module
int buttonSequence[] = {-1, -1, -1, -1, -1, -1};
int positionInSequence = 0;
bool buttonModuleCompleted = false;

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
char words[][5] = {
    "weird",
    "ghoul",
    "crane",
    "arise",
    "cries",
    "where",
    "empty",
    "began",
    "mouse",
    "grout",
    "quick",
    "zebra",
    "grind",
    "proud"
};
int rotations[] {
    5-2,
    6-4,
    (4*2)-(3*1),
    1,
    4,
    6,
    0,
    10,
    9,
    7,
    2,
    5,
    3,
    7
};
char secretMessage[5];
int secretMessageRotations = 0;
#define dotLength 100
#define dashLength 300
#define letterSpacing 300
#define restartSpacing 1000
#define morseCodeLedIndex 4

// switch module
byte arduinoDigitalPins = 0;
byte previousArduinoDigitalPins = 0;
byte nextButtonExpected = 0;
#define button1 0
#define button2 1
#define button3 2
#define button4 3

void setup() {
    pinMode(hardModeSelectPin, INPUT_PULLUP);
    pinMode(lidSensorPin, INPUT_PULLUP);
    pinMode(buzzerPin, OUTPUT);
    pinMode(wiresVerifyButton, INPUT_PULLUP);

    pinMode(shiftData, OUTPUT);
    pinMode(shiftClk, OUTPUT);
    pinMode(shiftLatch, OUTPUT);

    //digitalWrite(strikeLedPin1, 0);
    //digitalWrite(strikeLedPin2, 0);
    digitalWrite(shiftData, 0);
    digitalWrite(shiftClk, 0);
    digitalWrite(shiftLatch, 0);

    Wire.begin();
    
    Wire.beginTransmission(SLAVE_ADDR); // Configure Digital Ports 4, 7, 8, 12, 2 and 13 as inputs
    Wire.write(DDR);
    Wire.write(B00111111);    // X, X, P13=I/P, P12=I/P, P8=I/P, P7=I/P, P4=I/P, P2=I/P
    Wire.endTransmission();

    lcd.begin(16, 2);
    //lcd.clear();

    srand(analogRead(lidSensorPin));
    randomSeed(analogRead(lidSensorPin));

    Serial.begin(9600);
}

void loop() {
    if(bombState == 1) { // wait for start
        //if (analogRead(lidSensorPin) < lightThreshold) // wait for lid to open
        //    bombState = 2;
        bombState = 2;
    } else if (bombState == 2) { // running init
        // setup timer
        timerStart = 60*2 + 30; // 2:30
        timerInitTime = millis();

        strikes = 0;
        buttonModuleCompleted = false;
        generateButtonSequence();
        positionInSequence = 0;

        // generate a random serial code
        randomString(6).toCharArray(serialCode, 7);
        for(int i = 0; i < 6; i++) {
            Serial.print(serialCode[i]);
        }
        Serial.println();

        // at the start of each game, a random number is put in binary on leds
        binaryNumber = random(0, 32);
        Serial.print("binaryNumber: "); Serial.println(binaryNumber, BIN);

        readButtons();
        previousArduinoDigitalPins = arduinoDigitalPins;

        int wordIndex = 5;
        for(int i = 0; i < 5; i++) {
            secretMessage[i] = words[wordIndex][i];
        }
        secretMessageRotations = rotations[wordIndex];
        Serial.print("SecretMessage: ");
        for(int i = 0; i < 5; i++) {
            Serial.print(secretMessage[i]);
        }
        Serial.print(" SecretMessageRotations: "); Serial.println(secretMessageRotations);

        bombState = 3; // start the game
        
    } else if(bombState == 3) { // game is running
        updateTimer();
        showTimer();
        showSerialCode();
        updateBuzzer();
        showStrikes();
        showBinaryNumber();

        doButtonModule();
        doWiresModule();
        doMorseCodeModule();

        multiplexLeds();

        if(timerLow == 0 && timerHigh == 0 || strikes > 2) { // timer ran out of too many strikes
            explode();
        }

    } else if(bombState == 4) { // game is over
        lcd.setCursor(0, 0);

        lcd.print("You exploded!");
        Serial.println("You exploded!");

        for(int j = 0; j < 300; j++) {
            explosionSound();
        }
        
        bombState = 5;

    } else if (bombState == 5) { // wait for start
        if (analogRead(lidSensorPin) > lightThreshold) // wait for lid to close
            bombState = 1;

        updateBuzzer();

        //delay(1000);
    }
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

void showBinaryNumber() {
    ledStates[5][0] = bitRead(binaryNumber, 4);
    ledStates[6][0] = bitRead(binaryNumber, 3);
    ledStates[7][0] = bitRead(binaryNumber, 2);
    ledStates[8][0] = bitRead(binaryNumber, 1);
    ledStates[9][0] = bitRead(binaryNumber, 0);
}

void showSerialCode() {
    lcd.setCursor(6, 0);

    for(int i = 0; i < 6; i++) {
        lcd.print(serialCode[i]);
    }
}

void showStrikes() {
    lcd.setCursor(0, 1);
    lcd.print("Strikes:");
    lcd.print(strikes);
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

void doMorseCodeModule() {
    
}

void doButtonModule() {
    if(buttonModuleCompleted) {
        for(int i = 0; i < 4; i++) {
            ledStates[i][0] = 0;
            ledStates[i][1] = 1;
            ledStates[i][2] = 0;
        }
    }
    readButtons();

    if(previousArduinoDigitalPins < arduinoDigitalPins) {
        byte buttonPressed = previousArduinoDigitalPins ^ arduinoDigitalPins;
        
        Serial.println(buttonPressed, BIN);

        switch(buttonSequence[positionInSequence]) {
            case 1:
                if(buttonPressed == 0b00000001) positionInSequence++;
                else {
                    positionInSequence = 0;
                    addStrike();
                }
                break;
            case 2:
                if(buttonPressed == 0b00000010) positionInSequence++;
                else {
                    positionInSequence = 0;
                    addStrike();
                }
                break;
            case 3:
                if(buttonPressed == 0b00000100) positionInSequence++;
                else {
                    positionInSequence = 0;
                    addStrike();
                }
                break;
            case 4:
                if(buttonPressed == 0b00010000) positionInSequence++;
                else {
                    positionInSequence = 0;
                    addStrike();
                }
                break;
        }
    }

    if(buttonSequence[positionInSequence] == -1) {
        buttonModuleCompleted = true;
    }

    previousArduinoDigitalPins = arduinoDigitalPins;

    //Serial.println(arduinoDigitalPins, BIN);
}

/**
 * read button inputs from 2nd arduino
 * 
 */
void readButtons() {
    Wire.beginTransmission(SLAVE_ADDR); // Point to Digital Data Register
    Wire.write(DR);
    Wire.endTransmission();
    Wire.requestFrom((int)SLAVE_ADDR, (int)1);
    arduinoDigitalPins = ~Wire.read();

    //Serial.println(arduinoDigitalPins, BIN);
}

void generateButtonSequence() {
    if(ledStates[0][0] && !ledStates[0][1] && !ledStates[0][2]) { // if led1 is red
        if(!ledStates[2][0] && ledStates[2][1] && !ledStates[2][2]) {
            buttonSequence[0] = 3;
            if(ledStates[1][0] && !ledStates[1][1] && ledStates[1][2]) {
                buttonSequence[1] = 4;
                buttonSequence[2] = 2;
                buttonSequence[3] = 1;
            } else {
                buttonSequence[1] = 1;
                buttonSequence[2] = 2;
                buttonSequence[3] = 4;
            }
        } else {
            buttonSequence[0] = 2;
            if(ledStates[1][0] && !ledStates[1][1] && !ledStates[1][2]) {
                buttonSequence[1] = 3;
                buttonSequence[2] = 1;
                buttonSequence[3] = 4;
            } else {
                buttonSequence[1] = 4;
                buttonSequence[2] = 1;
                buttonSequence[3] = 3;
            }
        }
    } else if(!ledStates[1][0] && !ledStates[1][1] && ledStates[1][2]) { // if led 2 is blue
        buttonSequence[0] = 2;
        if(!ledStates[2][0] && ledStates[2][1] && !ledStates[2][2]) {
            buttonSequence[1] = 2;
            buttonSequence[2] = 3;
            if((ledStates[1][0] && !ledStates[1][1] && !ledStates[1][2]) || (ledStates[3][0] && !ledStates[3][1] && !ledStates[3][2])) {
                buttonSequence[3] = 4;
                buttonSequence[4] = 1;
            } else {
                buttonSequence[3] = 1;
                buttonSequence[4] = 4;
            }
        }
    } else if(ledStates[3][0] && !ledStates[3][1] && ledStates[3][2]) { // if led 4 is purple
        buttonSequence[0] = 1;
        buttonSequence[1] = 2;
        buttonSequence[2] = 3;
        buttonSequence[3] = 4;
    } else {
        buttonSequence[0] = 4;
        buttonSequence[1] = 2;
        buttonSequence[2] = 3;
        buttonSequence[3] = 1;
    }
}

void multiplexLeds() {
    for(int i = 0; i < numLeds; i++) {
        //out |= ledAnodes[i];
        bitWrite(out, ledAnodes[i], 1);
        // out &= ~ledRed;
        // out &= ~ledGreen;
        // out &= ~ledBlue;
        bitWrite(out, ledRed, !ledStates[i][0]);
        bitWrite(out, ledGreen, !ledStates[i][1]);
        bitWrite(out, ledBlue, !ledStates[i][2]);
        updateShiftReg();
        bitWrite(out, ledAnodes[i], 0);
    }
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