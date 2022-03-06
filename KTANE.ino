#include <LiquidCrystal.h>
#include <stdlib.h>     /* srand, rand */

using namespace std;

/// LCD interface pins ///
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


/// rest of the bomb ///
#define lidSensorPin A0
const int lightThreshold = 500;

#define buzzerPin 9

#define hardModeSelectPin 6
bool hardMode = false;

int timerStart = 0;
unsigned long timerInitTime = 0;
int timerHigh = 0;
int timerLow = 0;

char serialCode[] = "";

/*
 * bomb states:
 * 1 - bomb is waiting for the game to start
 * 2 - running init
 * 3 - game is running
 * 4 - game is over
*/
int bombState = 1;

int strikes = 0; // how many strikes the player has
#define strikeLedPin1 4
#define strikeLedPin2 5

void setup() {
    pinMode(hardModeSelectPin, INPUT);
    pinMode(strikeLedPin1, OUTPUT);
    pinMode(strikeLedPin2, OUTPUT);
    pinMode(lidSensorPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);

    digitalWrite(strikeLedPin1, 0);
    digitalWrite(strikeLedPin2, 0);

    lcd.begin(16, 2);

    randomSeed(analogRead(A1));

    Serial.begin(9600);
}

void loop() {
    switch (bombState) {
        case 1: // wait for start
            if (analogRead(lidSensorPin) < lightThreshold)
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
            randomString(6).toCharArray(serialCode, 6);
            Serial.println(serialCode);

            bombState = 3; // start the game
            break;
        case 3: // game is running
            updateTimer();
            showTimer();
            showStrikes();
            if(timerLow == 0 && timerHigh == 0 || strikes >= 2) { // timer ran out
                explode();
            }
            break;
        case 4: // game is over
            break;
        case 5: // wait for start
            break;
    }
}

void buzzerOn() {
    analogWrite(buzzerPin, 230);
}

void buzzerOff() {
    analogWrite(buzzerPin, 0);
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
    lcd.println(timerLow);
}

void showStrikes() {
    digitalWrite(strikeLedPin1, strikes >= 1);
    digitalWrite(strikeLedPin2, strikes >= 2);
}

void explode() {
    lcd.println("You exploded!");
    bombState = 4;
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